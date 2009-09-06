/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
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

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "common/safety.h"
#include "common/debug.h"
#include "common/types.h"

#include "dbus/object_path.h"
#include "dbus/service.h"

#include "dbus_service.h"
#include "dbus_iface_client.h"
#include "client.h"

service_t *
lash_dbus_service_new(lash_client_t *client)
{
	if (!client) {
		lash_error("NULL client parameter");
		return NULL;
	}

	return service_new(NULL, (bool *) &client->quit,
	                   1,
	                   object_path_new("/org/nongnu/LASH/Client",
	                                   (void *) client, 1,
	                                   &g_liblash_interface_client, NULL),
	                   NULL);
}

static void
lash_dbus_service_connect_handler(DBusPendingCall *pending,
                                  void            *data)
{
	lash_client_t *client = data;
	DBusMessage *msg;
	const char *err_str;
	DBusError err;
	dbus_uint32_t flags;

	if (!(msg = dbus_pending_call_steal_reply(pending))) {
		lash_error("Cannot get method return from pending call");
		goto end;
	}

	if (!method_return_verify(msg, &err_str)) {
		lash_error("Failed to connect to LASH server: %s", err_str);
		goto end_unref_msg;
	}

	dbus_error_init(&err);

	if (!dbus_message_get_args(msg, &err,
	                           DBUS_TYPE_UINT32, &flags,
	                           DBUS_TYPE_INVALID)) {
		lash_error("Cannot get message arguments: %s", err.message);
		dbus_error_free(&err);
		goto end_unref_msg;
	}

	client->flags = flags|LashClientIsConnected;

	lash_debug("Connected to LASH server with client flags %u", flags);

end_unref_msg:
	dbus_message_unref(msg);
end:
	dbus_pending_call_unref(pending);
}

static void
lash_dbus_service_activate_handler(DBusPendingCall *pending,
                                   void            *data)
{
	lash_client_t *client = data;
	DBusMessage *msg;
	DBusError err;
	const char *id_str, *client_name, *project_name, *wd, *err_str;

	if (!(msg = dbus_pending_call_steal_reply(pending))) {
		lash_error("Cannot get method return from pending call");
		goto end;
	}

	if (!method_return_verify(msg, &err_str)) {
		lash_error("LASH server failed to activate client: %s", err_str);
		goto end_unref_msg;
	}

	dbus_error_init(&err);

	if (!dbus_message_get_args(msg, &err,
	                           DBUS_TYPE_STRING, &id_str,
	                           DBUS_TYPE_STRING, &client_name,
	                           DBUS_TYPE_STRING, &project_name,
	                           DBUS_TYPE_STRING, &wd,
	                           DBUS_TYPE_INVALID)) {
		lash_error("Cannot get message arguments: %s", err.message);
		dbus_error_free(&err);
		goto end_unref_msg;
	}

	if (uuid_parse(id_str, client->id) != 0) {
		lash_error("Cannot parse client id");
		goto end_unref_msg;
	}

	if (!client_name[0])
		client_name = NULL;
	if (!project_name[0])
		project_name = NULL;

	lash_strset(&client->name, client_name);
	lash_strset(&client->project_name, project_name);

	/* Change working directory if the server so demands */
	if (strcmp(wd, client->working_dir) != 0) {
		lash_strset(&client->working_dir, wd);
		lash_info("Changing working directory to '%s'", wd);
		if (chdir(wd) == -1)
			lash_error("Cannot change directory: %s",
			           strerror(errno));
	}

	lash_debug("Activated LASH client with ID %s, name '%s', "
	           "project '%s', working directory '%s'",
	           id_str, client_name, project_name, wd);

end_unref_msg:
	dbus_message_unref(msg);
end:
	dbus_pending_call_unref(pending);
}


bool
lash_dbus_service_connect(lash_client_t *client)
{
	method_msg_t call;
	dbus_int32_t pid;
	DBusMessageIter iter, array_iter;
	int i;

	if (!client) {
		lash_error("NULL client parameter");
		return false;
	} else if (client->flags & LashClientIsConnected) {
		lash_warn("Client is already connected");
		return true;
	}

	if (!method_call_init(&call, client->dbus_service,
	                      (void *) client,
	                      lash_dbus_service_connect_handler,
	                      "org.nongnu.LASH",
	                      "/",
	                      "org.nongnu.LASH.Server",
	                      "Connect")) {
		lash_error("Failed to initialize Connect message");
		return false;
	}

	pid = (dbus_int32_t) getpid();

	if (!dbus_message_append_args(call.message,
	                              DBUS_TYPE_INT32, &pid,
	                              DBUS_TYPE_STRING, &client->class,
	                              DBUS_TYPE_UINT32, &client->flags,
	                              DBUS_TYPE_STRING, &client->working_dir,
	                              DBUS_TYPE_INVALID))
		goto oom;

	dbus_message_iter_init_append(call.message, &iter);

	if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s",
	                                      &array_iter))
		goto oom;

	for (i = 0; i < client->argc; ++i) {
		if (!dbus_message_iter_append_basic(&array_iter,
		                                    DBUS_TYPE_STRING,
		                                    (const void *) &client->argv[i])) {
			dbus_message_iter_close_container(&iter, &array_iter);
			goto oom;
		}
	}

	if (!dbus_message_iter_close_container(&iter, &array_iter))
		goto oom;

	lash_debug("Sending connection request to LASH server");

	if (!method_send(&call, true, true)) {
		lash_error("Failed to send Connect message");
		return false;
	}

	return true;

oom:
	lash_error("Ran out of memory while creating Connect message");
	dbus_message_unref(call.message);
	call.message = NULL;
	return false;;
}

bool
lash_dbus_service_activate(lash_client_t *client)
{
	method_msg_t call;

	if (!client) {
		lash_error("NULL client parameter");
		return false;
	}

	if (!method_call_init(&call, client->dbus_service,
	                      (void *) client,
	                      lash_dbus_service_activate_handler,
	                      "org.nongnu.LASH",
	                      "/",
	                      "org.nongnu.LASH.Server",
	                      "Activate")) {
		lash_error("Failed to initialize Activate message");
		return false;
	}

	if (!method_send(&call, true, true)) {
		lash_error("Failed to send Activate message");
		return false;
	}

	return true;
}

/* The appropriate destructor is service_destroy() in dbus/service.c */

/* EOF */
