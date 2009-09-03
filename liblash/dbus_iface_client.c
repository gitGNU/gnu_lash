/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *   Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
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
#include "dbus_iface_client.h"

#include "common/safety.h"
#include "common/debug.h"

#include "dbus/error.h"
#include "dbus/object_path.h"
#include "dbus/interface.h"
#include "dbus/method.h"

#include "lash/types.h"

#include "client.h"

#define client_ptr ((lash_client_t *)(((object_path_t *)call->context)->context))

/** This handler parses the reply sent by the server to a WaitSnapshot call.
 * The message is verified and a boolean value @a snapshot_ok is extracted,
 * based on which the client may initiate a new snapshot task.
 * @param pending Pointer to the D-Bus pending call object containing the reply.
 * @param data Pointer to user data, in this case a @ref lash_client_t object.
 */
static void
lash_dbus_wait_snapshot_handler(DBusPendingCall *pending,
                                void            *data)
{
	lash_client_t *client = data;
	DBusMessage *msg = dbus_pending_call_steal_reply(pending);
	DBusError err;
	dbus_bool_t snapshot_ok;
	const char *err_str;

	if (!msg) {
		lash_error("Cannot get method return from pending call");
		goto end;
	}

	if (!method_return_verify(msg, &err_str)) {
		lash_error("Server returned an error: %s", err_str);
		goto end_unref_msg;
	}

	dbus_error_init(&err);

	/* Read the server's response to whether the client can proceed with
	   the snapshot. */
	if (!dbus_message_get_args(msg, &err,
	                           DBUS_TYPE_BOOLEAN, &snapshot_ok,
	                           DBUS_TYPE_INVALID)) {
		lash_error("Cannot get message argument: %s", err.message);
		dbus_error_free(&err);
		goto end_unref_msg;
	}

	if (snapshot_ok)
		lash_new_save_task(client, LASH_EVENT_SNAPSHOT);
	else
		client->pending_task = 0;

end_unref_msg:
	dbus_message_unref(msg);
end:
	dbus_pending_call_unref(pending);
}

bool
check_client_cb(lash_client_t *client,
                method_call_t *call)
{
	if (client->client_cb)
		return true;

	if (call)
		lash_dbus_error(call, LASH_DBUS_ERROR_GENERIC,
		                "Client callback not registered");
	else
		lash_error("Client callback not registered");

	return false;
}

/* Report task completion or failure to the LASH server */
static void
report_success_or_failure(lash_client_t *client,
                          bool           success)
{
	if (!client->pending_task) {
		lash_error("Client has no pending task");
		return;
	}

	uint8_t x = (uint8_t) (success ? 255 : 0);

	/* Send a success or failure report */
	method_call_new_valist(client->dbus_service, NULL,
	                       method_default_handler, false, false,
	                       "org.nongnu.LASH",
	                       "/",
	                       "org.nongnu.LASH.Server",
	                       "Progress",
	                       DBUS_TYPE_UINT64, &client->pending_task,
	                       DBUS_TYPE_BYTE, &x,
	                       DBUS_TYPE_INVALID);
}

void
lash_new_save_task(lash_client_t *client,
                   enum LashEvent  task_event)
{
	bool retval;

	if (!(retval = check_client_cb(client, NULL)))
		goto report;

	client->task_event = (uint8_t) task_event;
	client->task_progress = 0;

	/* Call the client callback */
	if (!(retval = client->client_cb(task_event, client->client_data)))
		lash_error("Callback failed");

	client->task_event = LASH_EVENT_INVALID;

	/* Check if the client wrote any data during the callback, in which
	   case we need to send a CommitData message instead of a simple
	   success/failure message. */
	if (client->task_msg.message) {
		if (retval) {
			if (!(retval = dbus_message_iter_close_container(&client->task_msg_iter,
			                                                 &client->task_msg_array_iter))) {
				lash_error("Failed to close array container");
				goto unref;
			}

			if ((retval = method_send(&client->task_msg, false, false)))
				goto end;

			lash_error("Failed to send CommitData method call");
		} else {
		unref:
			dbus_message_unref(client->task_msg.message);
			client->task_msg.message = NULL;
		}
	}

report:
	report_success_or_failure(client, retval);
end:
	client->pending_task = 0;
}

static bool
get_task_id(method_call_t   *call,
            dbus_uint64_t   *task_id,
            DBusMessageIter *iter)
{
	DBusMessageIter xiter;

	if (!iter)
		iter = &xiter;

	if (!dbus_message_iter_init(call->message, iter)
	    || dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_UINT64) {
		lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_ARGS,
		                "Invalid arguments to method \"%s\": "
		                "Cannot find task ID in message",
		                call->method_name);
		return false;
	}

	dbus_message_iter_get_basic(iter, task_id);
	dbus_message_iter_next(iter);

	if (client_ptr->pending_task) {
		lash_dbus_error(call, LASH_DBUS_ERROR_UNFINISHED_TASK,
		                "Task %llu is unfinished, bouncing task %llu",
		                client_ptr->pending_task, *task_id);
		return false;
	}

	return true;
}

/** This is the client's TrySnapshot method handler. The server calls it to
 * query whether the client can be interrupted and thus snapshotted.<br />
 * Most often LASH will only do batch snapshots that will commence if and only
 * if none of the clients queried reject it. Only clients which reject the
 * query actually return the @a snapshot_ok parameter, everyone else calls the
 * server back straight from inside the TrySnapshot method handler.
 * @param call Pointer to the method call object. It is handled by liblash and
               must not be freed manually.
 */
static void
lash_dbus_try_snapshot(method_call_t *call)
{
	dbus_uint64_t task_id;
	dbus_bool_t retval;

	lash_debug("TrySnapshot");

	if (!get_task_id(call, &task_id, NULL)
	    || !check_client_cb(client_ptr, call))
		return;

	/* If it's not OK to interrupt the client reply with 'snapshot_ok=false' */
	if (!(retval = client_ptr->client_cb(LASH_EVENT_INTERRUPT, client_ptr->client_data))) {
		method_return_new_single(call, DBUS_TYPE_BOOLEAN, &retval);
		return;
	}

	/* The client can snapshot, call the server and wait for it to acknowledge */
	client_ptr->pending_task = task_id;
	method_call_new_void(client_ptr->dbus_service, client_ptr,
	                     lash_dbus_wait_snapshot_handler, true, true,
	                     "org.nongnu.LASH",
	                     "/",
	                     "org.nongnu.LASH.Server",
	                     "WaitSnapshot");
}

static void
lash_dbus_save(method_call_t *call)
{
	dbus_uint64_t task_id;

	lash_debug("Save");

	if (!get_task_id(call, &task_id, NULL))
		return;

	client_ptr->pending_task = task_id;
	lash_new_save_task(client_ptr, LASH_EVENT_SAVE);
}

static void
lash_dbus_load(method_call_t *call)
{
	DBusMessageIter iter, array_iter;
	dbus_uint64_t task_id;
	bool retval;

	lash_debug("Load");

	if (!check_client_cb(client_ptr, call)
	    || !get_task_id(call, &task_id, &iter))
		return;

	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
		lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_ARGS,
		                "Invalid arguments to method \"%s\": "
		                "Cannot find data set array in message",
		                call->method_name);
		return;
	}

	dbus_message_iter_recurse(&iter, &client_ptr->task_msg_array_iter);

	client_ptr->pending_task = task_id;
	client_ptr->task_event = LASH_EVENT_LOAD;
	client_ptr->task_progress = 0;

	/* Call the client callback; its return value dictates whether
	   to report success or failure back to the server */
	if (!(retval = client_ptr->client_cb(LASH_EVENT_LOAD, client_ptr->client_data)))
		lash_error("Client failed to load data");

	report_success_or_failure(client_ptr, retval);

	client_ptr->pending_task = 0;
	client_ptr->task_event = LASH_EVENT_INVALID;
}

void
lash_new_quit_task(lash_client_t *client)
{
	if (client->pending_task)
		lash_error("Warning: Task %llu is unfinished, quitting anyway", client->pending_task);

	if (check_client_cb(client, NULL))
		client->client_cb(LASH_EVENT_QUIT, client->client_data);
}

static void
lash_dbus_quit(method_call_t *call)
{
	lash_new_quit_task(client_ptr);
}

static void
lash_dbus_client_name_changed(method_call_t *call)
{
	DBusError err;
	const char *new_name;

	dbus_error_init(&err);

	if (!dbus_message_get_args(call->message, &err,
	                           DBUS_TYPE_STRING, &new_name,
	                           DBUS_TYPE_INVALID)) {
		lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_ARGS,
		                "Invalid arguments to method \"%s\": %s",
		                call->method_name, err.message);
		dbus_error_free(&err);
		return;
	}

	lash_debug("Setting new client name to '%s'", new_name);

	if (!new_name[0])
		new_name = NULL;

	lash_strset(&client_ptr->name, new_name);

	if (check_client_cb(client_ptr, call))
		client_ptr->client_cb(LASH_EVENT_CLIENT_NAME_CHANGED, client_ptr->client_data);
}

#undef client_ptr

/*
 * Interface methods.
 */

METHOD_ARGS_BEGIN(Save)
  METHOD_ARG_DESCRIBE("task_id", "t", DIRECTION_IN)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Load)
  METHOD_ARG_DESCRIBE("task_id", "t", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("configs", "a{sv}", DIRECTION_IN)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Quit)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(TrySnapshot)
  METHOD_ARG_DESCRIBE("task_id", "t", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("snapshot_ok", "b", DIRECTION_OUT)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(ClientNameChanged)
  METHOD_ARG_DESCRIBE("new_name", "s", DIRECTION_IN)
METHOD_ARGS_END

METHODS_BEGIN
  METHOD_DESCRIBE(Save, lash_dbus_save)
  METHOD_DESCRIBE(Load, lash_dbus_load)
  METHOD_DESCRIBE(Quit, lash_dbus_quit)
  METHOD_DESCRIBE(TrySnapshot, lash_dbus_try_snapshot)
  METHOD_DESCRIBE(ClientNameChanged, lash_dbus_client_name_changed)
METHODS_END

/*
 * Interface description.
 */

INTERFACE_BEGIN(g_liblash_interface_client, "org.nongnu.LASH.Client")
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
INTERFACE_END
