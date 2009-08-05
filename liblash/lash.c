/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *   Copyright (C) 2002, 2003 Robert Ham <rah@bash.sh>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "../config.h"

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <dbus/dbus.h>

#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/param.h>
#include <rpc/xdr.h>

#include "common/safety.h"
#include "common/debug.h"
#include "common/types.h"

#include "lash/lash.h"

#include "dbus/service.h"

#include "dbus_service.h"
#include "client.h"

#include "dbus_iface_client.h"

static void *
lash_client_thread(void *arg)
{
	lash_client_t *client = arg;

	while (!client->quit) {
		dbus_connection_read_write(client->dbus_service->connection, 250);
		while (dbus_connection_get_dispatch_status(client->dbus_service->connection)
		       == DBUS_DISPATCH_DATA_REMAINS) {
			dbus_connection_dispatch(client->dbus_service->connection);
		}
	}

	if (client->flags & LashClientIsOrdinary)
		method_call_new_void(client->dbus_service, NULL,
		                     method_default_handler, false, false,
		                     "org.nongnu.LASH",
		                     "/",
		                     "org.nongnu.LASH.Server",
		                     "Disconnect");

	pthread_exit(NULL);
}

#if 0
static void
ping_handler(DBusPendingCall *pending,
             void            *data)
{
	DBusMessage *msg = dbus_pending_call_steal_reply(pending);
	if (msg)
		dbus_message_unref(msg);
	dbus_pending_call_unref(pending);
	fprintf(stderr, "Server replied: Pong!\n");
}
#endif

static lash_client_t *
lash_client_new_with_service(void)
{
	lash_client_t *client = lash_client_new();
	if (!client) {
		lash_error("Failed to allocate memory for client");
		return NULL;
	}

	/* Connect to the D-Bus daemon */
	client->dbus_service = lash_dbus_service_new(client);
	if (!client->dbus_service) {
		lash_error("Failed to start client D-Bus service");
		lash_client_destroy(client);
		client = NULL;
	}

	return client;
}

/* The client's signal handler function for signals
   originating from org.nongnu.LASH.Server */
static void
lash_server_signal_handler(lash_client_t *client,
                           const char    *member,
                           DBusMessage   *message)
{
	const char *project_name;
	dbus_uint64_t task_id;
	DBusError err;

	dbus_error_init(&err);

	if (strcmp(member, "Save") == 0) {
		if (!dbus_message_get_args(message, &err,
		                           DBUS_TYPE_STRING, &project_name,
		                           DBUS_TYPE_UINT64, &task_id,
		                           DBUS_TYPE_INVALID)) {
			lash_error("Cannot get signal arguments: %s", err.message);
			dbus_error_free(&err);
			return;
		}

		//lash_info("Save signal for project '%s' received.", project_name);

		/* Silently return if this signal doesn't concern our project */
		if (!client->project_name
		    || strcmp(client->project_name, project_name) != 0)
			return;

		if (!client->pending_task)
			lash_new_save_task(client, task_id);
		else
			lash_error("Task %llu is unfinished", client->pending_task);

		//lash_info("Save signal for project '%s' processed.", project_name); fflush(stdout);

	} else if (strcmp(member, "Quit") == 0) {
		if (!dbus_message_get_args(message, &err,
		                           DBUS_TYPE_STRING, &project_name,
		                           DBUS_TYPE_INVALID)) {
			lash_error("Cannot get signal arguments: %s", err.message);
			dbus_error_free(&err);
			return;
		}

		/* Silently return if this signal doesn't concern our project */
		if (client->project_name
		    && strcmp(client->project_name, project_name) != 0)
			return;

		lash_new_quit_task(client);

	} else {
		lash_error("Received unknown signal '%s'", member);
	}
}

static void
lash_project_name_changed_handler(lash_client_t *client,
                                  DBusMessage   *message)
{
	DBusError err;
	const char *old_name, *new_name;

	dbus_error_init(&err);

	if (!dbus_message_get_args(message, &err,
	                           DBUS_TYPE_STRING, &old_name,
	                           DBUS_TYPE_STRING, &new_name,
	                           DBUS_TYPE_INVALID)) {
		lash_error("Cannot get signal arguments: %s", err.message);
		dbus_error_free(&err);
		return;
	}

	if (client->project_name) {
		if (strcmp(client->project_name, old_name) != 0)
			return;
	} else if (old_name[0])
		return;

	if (!new_name[0])
		new_name = NULL;

	lash_strset(&client->project_name, new_name);

	/* Call client's ProjectChanged callback */
	if (check_client_cb(client, NULL))
		client->client_cb(LASH_EVENT_PROJECT_NAME_CHANGED, client->client_data);
}

static void
lash_control_signal_handler(lash_client_t *client,
                            const char    *member,
                            DBusMessage   *message)
{
	if (!client->control_cb) {
		lash_error("Controller callback not registered");
		return;
	}

	uint8_t sig_id;
	dbus_bool_t ret;
	DBusError err;
	const char *str1, *str2;
	unsigned char byte_var[2];
	uuid_t id;
	enum LashEvent type;

	if (strcmp(member, "ProjectDisappeared") == 0) {
		sig_id = 1;
		type = LASH_EVENT_PROJECT_REMOVED;
	} else if (strcmp(member, "ProjectAppeared") == 0) {
		sig_id = 2;
		type = LASH_EVENT_PROJECT_ADDED;
	} else if (strcmp(member, "ProjectNameChanged") == 0) {
		sig_id = 3;
		type = LASH_EVENT_PROJECT_NAME_CHANGED;
	} else if (strcmp(member, "ProjectPathChanged") == 0) {
		sig_id = 4;
		type = LASH_EVENT_PROJECT_PATH_CHANGED;
	} else if (strcmp(member, "ClientAppeared") == 0) {
		sig_id = 5;
		type = LASH_EVENT_PROJECT_CLIENT_ADDED;
	} else if (strcmp(member, "ClientDisappeared") == 0) {
		sig_id = 6;
		type = LASH_EVENT_PROJECT_CLIENT_REMOVED;
	} else if (strcmp(member, "ClientNameChanged") == 0) {
		sig_id = 7;
		type = 	LASH_EVENT_CLIENT_NAME_CHANGED;
	} else if (strcmp(member, "ClientJackNameChanged") == 0) {
		sig_id = 8;
		type = LASH_EVENT_CLIENT_JACK_NAME_CHANGED;
	} else if (strcmp(member, "Progress") == 0) {
		sig_id = 9;
		type = LASH_EVENT_SERVER_PROGRESS;
	} else {
		lash_error("Received unknown signal '%s'", member);
		return;
	}

	dbus_error_init(&err);

	if (sig_id == 1) {
		ret = dbus_message_get_args(message, &err,
		                            DBUS_TYPE_STRING, &str1,
		                            DBUS_TYPE_INVALID);
		str2 = "";
	} else if (sig_id < 9) {
		ret = dbus_message_get_args(message, &err,
		                            DBUS_TYPE_STRING, &str1,
		                            DBUS_TYPE_STRING, &str2,
		                            DBUS_TYPE_INVALID);
	} else {
		ret = dbus_message_get_args(message, &err,
		                            DBUS_TYPE_BYTE, &byte_var[0],
		                            DBUS_TYPE_INVALID);
		byte_var[1] = '\0';
		str1 = (const char *) byte_var;
		str2 = "";
	}

	if (!ret) {
		lash_error("Cannot get signal arguments: %s", err.message);
		dbus_error_free(&err);
		return;
	}

	if (sig_id < 5 || sig_id > 8)
		uuid_clear(id);
	else if (uuid_parse(str1, id) == 0) {
		str1 = str2;
	} else {
		lash_error("Cannot parse client id");
		return;
	}

	/* Call the control callback */
	client->control_cb(type, str1, str2, id, client->control_data);
}

static DBusHandlerResult
lash_dbus_signal_handler(DBusConnection *connection,
                         DBusMessage    *message,
                         void           *data)
{
	/* Let non-signal messages fall through */
	if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	lash_client_t *client = data;
	const char *member, *interface;

	member = dbus_message_get_member(message);

	if (!member) {
		lash_error("Received signal with NULL member");
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	interface = dbus_message_get_interface(message);

	if (!interface) {
		lash_error("Received signal with NULL interface");
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	if (strcmp(interface, "org.nongnu.LASH.Server") == 0) {
		lash_debug("Received Server signal '%s'", member);
		lash_server_signal_handler(client, member, message);

	} else if (strcmp(interface, "org.nongnu.LASH.Control") == 0) {
		lash_debug("Received Control signal '%s'", member);

		if (client->flags & LashClientIsOrdinary)
			lash_project_name_changed_handler(client, message);

		if (client->flags & LashClientIsController)
			lash_control_signal_handler(client, member, message);

	} else if (strcmp(interface, "org.freedesktop.DBus") != 0) {
		lash_error("Received signal from unknown interface '%s'",
		           interface);
	}

	return DBUS_HANDLER_RESULT_HANDLED;
}

static bool
lash_client_register_as_controller(lash_client_t *client)
{
	DBusError err;

	dbus_error_init(&err);

	lash_debug("Registering client as control application");

	/* Listen to the LASH server's frontend-facing beacon */
	dbus_bus_add_match(client->dbus_service->connection,
	                   "type='signal'"
	                   ",sender='org.nongnu.LASH'"
	                   ",path='/'"
	                   ",interface='org.nongnu.LASH.Control'",
	                   &err);

	if (dbus_error_is_set(&err)) {
		lash_error("Failed to add D-Bus match rule: "
		           "%s", err.message);
		dbus_error_free(&err);
		return false;
	}

	return true;
}

static void
lash_client_add_filter(lash_client_t **client)
{
	if (!dbus_connection_add_filter((*client)->dbus_service->connection,
	                                lash_dbus_signal_handler,
	                                *client, NULL)) {
		lash_error("Failed to add D-Bus filter");
		lash_client_destroy(*client);
		*client = NULL;
	}
}

lash_client_t *
lash_client_open(const char      *class,
                 enum LashFlags   flags,
                 int              argc,
                 char           **argv)
{
	lash_client_t *client = NULL;

	if (!class) {
		lash_error("Invalid arguments: class parameter is NULL");
		goto end;
	}

	if (!class[0]) {
		lash_error("Invalid arguments: class parameter is empty");
		goto end;
	}

	if (!argc || !argv || !argv[0] || !argv[0][0]) {
		lash_error("Invalid arguments: no command-line args");
		goto end;
	}

	char *str, wd[MAXPATHLEN];
	DBusError err;

	client = lash_client_new_with_service();
	if (!client) {
		lash_error("Failed to create new client");
		goto end;
	}

	/* Set the client parameters */
	if (!(str = getcwd(wd, MAXPATHLEN))) {
		lash_error("Cannot get working directory: %s",
		           strerror(errno));
		wd[0] = '\0';
		if ((str = getenv("PWD")) || (str = getenv("HOME"))) {
			if (strlen(str) < MAXPATHLEN)
				strcpy(wd, str);
		}
	}
	client->argc = argc;
	client->argv = argv;
	client->working_dir = lash_strdup(wd);
	client->flags = (uint32_t) flags;
	lash_strset(&client->class, class);

	dbus_error_init(&err);

	/* Check whether the server is active */
	if (!dbus_bus_name_has_owner(client->dbus_service->connection,
	                             // TODO: Move service name into public header
	                             "org.nongnu.LASH", &err)) {
		if (dbus_error_is_set(&err)) {
			lash_error("Failed to query LASH service "
			           "availability: %s", err.message);
			dbus_error_free(&err);
			dbus_error_init(&err);
		}

		if (!getenv("LASH_NO_START_SERVER"))
			lash_info("Attempting to auto-start LASH server");
		else {
			lash_info("Not attempting to auto-start LASH server");
			goto fail;
		}
	}

	if (!lash_dbus_service_connect(client)) {
		lash_error("Cannot connect to LASH server");
		goto fail;
	}

	if (client->flags & LashClientIsController
	    && !lash_client_register_as_controller(client)) {
		lash_error("Cannot register as controller");
		goto fail;
	}

	/* Listen to the LASH server's client-facing beacon */
	dbus_bus_add_match(client->dbus_service->connection,
	                   "type='signal'"
	                   ",sender='org.nongnu.LASH'"
	                   ",path='/'"
	                   ",interface='org.nongnu.LASH.Server'",
	                   &err);

	if (!dbus_error_is_set(&err)) {
		dbus_bus_add_match(client->dbus_service->connection,
		                   "type='signal'"
		                   ",sender='org.nongnu.LASH'"
		                   ",path='/'"
		                   ",interface='org.nongnu.LASH.Control'"
		                   ",member='ProjectNameChanged'",
		                   &err);
	}

	if (dbus_error_is_set(&err)) {
		lash_error("Failed to add D-Bus match rule: %s", err.message);
		goto fail;
	}

	client->flags |= LashClientIsOrdinary;
	lash_client_add_filter(&client);

	goto end;

fail:
	dbus_error_free(&err);
	lash_client_destroy(client);
	client = NULL;

end:
	return client;
}

lash_client_t *
lash_client_open_controller(void)
{
	lash_client_t *client;

	client = lash_client_new_with_service();
	if (!client) {
		lash_error("Failed to create new client");
		return NULL;
	}

	if (!lash_client_register_as_controller(client)) {
		lash_error("Cannot register as controller");
		lash_client_destroy(client);
		return NULL;
	}

	client->flags |= LashClientIsController;
	lash_client_add_filter(&client);

	return client;
}

// TODO: Move lash_set_*_callback() args checking to a common function

bool
lash_set_client_callback(lash_client_t      *client,
                         LashClientCallback  callback,
                         void               *user_data)
{
	if (!client || !callback) {
		lash_error("Invalid arguments");
		return false;
	} else if (!(client->flags & LashClientIsConnected)) {
		lash_error("Not connected to server");
		return false;
	}

	client->client_cb = callback;
	client->client_data = user_data;

	return true;
}

bool
lash_set_control_callback(lash_client_t       *client,
                          LashControlCallback  callback,
                          void                *user_data)
{
	if (!client) {
		lash_error("Client pointer is NULL");
		return false;
	} else if (!callback) {
		lash_error("Callback function is NULL");
		return false;
	}

	client->control_cb = callback;
	client->control_data = user_data;

	return true;
}

bool
lash_activate(lash_client_t *client)
{
	int err;

	if (!client) {
		lash_error("Client pointer is NULL");
		return false;
	}

	// TODO: Find some generic place for this
	if (!client->dbus_service) {
		lash_error("D-Bus service not running");
		return false;
	}

	/* Ordinary clients must send an Activate message. */
	if ((client->flags & LashClientIsOrdinary) && !lash_dbus_service_activate(client)) {
		lash_error("Cannot activate LASH client");
		return false;
	}

	if ((err = pthread_create(&client->thread_id, NULL, lash_client_thread, client))) {
		lash_error("Cannot create client thread: %s", strerror(err));
		return false;
	}

	lash_info("LASH client activated");
	return true;
}

void
lash_notify_progress(lash_client_t *client,
                     uint8_t        percentage)
{
	if (!client || !client->dbus_service
	    || !client->pending_task || !percentage)
		return;

	if (percentage > 99)
		percentage = 99;

	method_call_new_valist(client->dbus_service, NULL,
	                       method_default_handler, false, false,
	                       "org.nongnu.LASH",
	                       "/",
	                       "org.nongnu.LASH.Server",
	                       "Progress",
	                       DBUS_TYPE_UINT64, &client->pending_task,
	                       DBUS_TYPE_BYTE, &percentage,
	                       DBUS_TYPE_INVALID);
}

const char *
lash_get_client_name(lash_client_t *client)
{
	return (const char *) ((client && client->dbus_service)
	                       ? client->name : NULL);
}

const char *
lash_get_project_name(lash_client_t *client)
{
	return (const char *) ((client && client->dbus_service)
	                       ? client->project_name : NULL);
}

bool
lash_client_is_being_restored(lash_client_t *client)
{
	return (client && (client->flags & LashClientIsRestored));
}

void
lash_jack_client_name(lash_client_t *client,
                      const char    *name)
{
	if (!client || !name || !name[0]) {
		lash_error("Invalid arguments");
		return;
	}

	// TODO: Find some generic place for this
	if (!client->dbus_service) {
		lash_error("D-Bus service not running");
		return;
	}

	method_call_new_single(client->dbus_service, NULL,
	                       method_default_handler, false, false,
	                       "org.nongnu.LASH",
	                       "/",
	                       "org.nongnu.LASH.Server",
	                       "JackName",
	                       DBUS_TYPE_STRING, &name);

	lash_debug("Sent JACK name");
}

// TODO: Convert to ProjectOpen
void
lash_control_load_project_path(lash_client_t *client,
                               const char    *project_path)
{
	if (!client || !project_path) {
		lash_error("Invalid arguments");
		return;
	}

	// TODO: Find some generic place for this
	if (!client->dbus_service) {
		lash_error("D-Bus service not running");
		return;
	}

	method_call_new_single(client->dbus_service, NULL,
	                       method_default_handler, false, false,
	                       "org.nongnu.LASH",
	                       "/",
	                       "org.nongnu.LASH.Control",
	                       "LoadProjectPath",
	                       DBUS_TYPE_STRING, &project_path);

	lash_debug("Sent LoadProjectPath request");
}

void
lash_control_name_project(lash_client_t *client,
                          const char    *project_name,
                          const char    *new_name)
{
	if (!client || !project_name || !new_name) {
		lash_error("Invalid arguments");
		return;
	}

	// TODO: Find some generic place for this
	if (!client->dbus_service) {
		lash_error("D-Bus service not running");
		return;
	}

	method_call_new_valist(client->dbus_service, NULL,
	                       method_default_handler, false, false,
	                       "org.nongnu.LASH",
	                       "/",
	                       "org.nongnu.LASH.Control",
	                       "ProjectRename",
	                       DBUS_TYPE_STRING, &project_name,
	                       DBUS_TYPE_STRING, &new_name,
	                       DBUS_TYPE_INVALID);

	lash_debug("Sent ProjectRename request");
}

void
lash_control_move_project(lash_client_t *client,
                          const char    *project_name,
                          const char    *new_path)
{
	if (!client || !project_name || !new_path) {
		lash_error("Invalid arguments");
		return;
	}

	// TODO: Find some generic place for this
	if (!client->dbus_service) {
		lash_error("D-Bus service not running");
		return;
	}

	method_call_new_valist(client->dbus_service, NULL,
	                       method_default_handler, false, false,
	                       "org.nongnu.LASH",
	                       "/",
	                       "org.nongnu.LASH.Control",
	                       "ProjectMove",
	                       DBUS_TYPE_STRING, &project_name,
	                       DBUS_TYPE_STRING, &new_path,
	                       DBUS_TYPE_INVALID);

	lash_debug("Sent ProjectMove request");
}

void
lash_control_save_project(lash_client_t *client,
                          const char    *project_name)
{
	if (!client || !project_name) {
		lash_error("Invalid arguments");
		return;
	}

	// TODO: Find some generic place for this
	if (!client->dbus_service) {
		lash_error("D-Bus service not running");
		return;
	}

	method_call_new_single(client->dbus_service, NULL,
	                       method_default_handler, false, false,
	                       "org.nongnu.LASH",
	                       "/",
	                       "org.nongnu.LASH.Control",
	                       "ProjectSave",
	                       DBUS_TYPE_STRING, &project_name);

	lash_debug("Sent ProjectSave request");
}

void
lash_control_close_project(lash_client_t *client,
                           const char    *project_name)
{
	if (!client || !project_name) {
		lash_error("Invalid arguments");
		return;
	}

	// TODO: Find some generic place for this
	if (!client->dbus_service) {
		lash_error("D-Bus service not running");
		return;
	}

	method_call_new_single(client->dbus_service, NULL,
	                       method_default_handler, false, false,
	                       "org.nongnu.LASH",
	                       "/",
	                       "org.nongnu.LASH.Control",
	                       "ProjectClose",
	                       DBUS_TYPE_STRING, &project_name);

	lash_debug("Sent ProjectClose request");
}

int
lash_server_connected(lash_client_t *client)
{
	return (client) ? (client->flags & LashClientIsConnected) : 0;
}

/* EOF */
