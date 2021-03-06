/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *   Copyright (C) 2008 Nedko Arnaudov
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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "dbus/object_path.h"
#include "common/safety.h"
#include "common/debug.h"
#include "dbus/introspection.h"	/* g_dbus_interface_dtor_introspectable */
#include "dbus/error.h" 	/* lash_dbus_error() */

static DBusHandlerResult
object_path_handler(DBusConnection *connection,
                    DBusMessage    *message,
                    void           *data);

static void
object_path_handler_unregister(DBusConnection *conn,
                               void           *data);

object_path_t *
object_path_new(const char *name,
                void       *context,
                int         num_ifaces,
                            ...)
{
	if (!name || !name[0] || num_ifaces < 1) {
		lash_debug("Invalid arguments");
		return NULL;
	}

	lash_debug("Creating object path");

	object_path_t *path;
	va_list argp;
	const interface_t **iface_pptr;

	path = lash_malloc(1, sizeof(object_path_t));
	path->name = lash_strdup(name);
	path->interfaces = lash_malloc(num_ifaces + 2, sizeof(interface_t *));

	va_start(argp, num_ifaces);

	iface_pptr = path->interfaces;
	*iface_pptr = &g_dbus_interface_dtor_introspectable;
	for (++iface_pptr;
	     (*iface_pptr = va_arg(argp, const interface_t *));
	     ++iface_pptr);

	va_end(argp);

	if ((path->introspection = introspection_new(path))) {
		path->context = context;
		return path;
	}

	lash_error("Failed to create object path");
	object_path_destroy(path);

	return NULL;
}

int
object_path_register(DBusConnection *conn,
                     object_path_t  *path)
{
	if (!conn || !path || !path->name || !path->interfaces) {
		lash_debug("Invalid arguments");
		return 0;
	}

	lash_debug("Registering object path");

	DBusObjectPathVTable vtable =
	{
		object_path_handler_unregister,
		object_path_handler,
		NULL, NULL, NULL, NULL
	};

	dbus_connection_register_object_path(conn, path->name,
	                                     &vtable, (void *) path);

	return 1;
}

void
object_path_destroy(object_path_t *path)
{
	lash_debug("Destroying object path");

	if (path) {
		if (path->name) {
			free(path->name);
			path->name = NULL;
		}
		if (path->interfaces) {
			free(path->interfaces);
			path->interfaces = NULL;
		}
		introspection_destroy(path);
		free(path);
		path = NULL;
	}
#ifdef LASH_DEBUG
	else
		lash_debug("Nothing to destroy");
#endif
}


static DBusHandlerResult
object_path_handler(DBusConnection *connection,
                    DBusMessage    *message,
                    void           *data)
{
	const char *interface_name;
	const interface_t **iface_pptr;
	method_call_t call;

	/* Check if the message is a method call. If not, ignore it. */
	if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_METHOD_CALL) {
		goto handled;
	}

	/* Get the invoked method's name and make sure it's non-NULL. */
	if (!(call.method_name = dbus_message_get_member(message))) {
		lash_dbus_error(&call, LASH_DBUS_ERROR_UNKNOWN_METHOD,
		                "Received method call with empty method name");
		goto send_return;
	}

	/* Initialize our data. */
	call.connection = connection;
	call.message = message;
	call.interface = NULL; /* To be set by the default interface handler */
	call.context = data;
	call.reply = NULL;

	/* Check if there's an interface specified for this method call. */
	if ((interface_name = dbus_message_get_interface(message))) {
		for (iface_pptr = (const interface_t **) ((object_path_t *) data)->interfaces;
		     iface_pptr && *iface_pptr;
		     ++iface_pptr) {
			if (strcmp(interface_name, (*iface_pptr)->name) == 0) {
				if ((*iface_pptr)->handler(*iface_pptr, &call)) {
					goto send_return;
				}

				break;
			}
		}
	}
	else
	{
		/* No interface was specified so we have to try them all. This is
		* dictated by the D-Bus specification which states that method calls
		* omitting the interface must never be rejected.
		*/

		for (iface_pptr = (const interface_t **) ((object_path_t *) data)->interfaces;
		     iface_pptr && *iface_pptr;
		     ++iface_pptr) {
			if ((*iface_pptr)->handler(*iface_pptr, &call)) {
				goto send_return;
			}
		}
	}

	lash_dbus_error(&call, LASH_DBUS_ERROR_UNKNOWN_METHOD,
	                "Method \"%s\" with signature \"%s\" on interface \"%s\" doesn't exist",
	                call.method_name, dbus_message_get_signature(message), interface_name);

send_return:
	method_return_send(&call);

handled:
	return DBUS_HANDLER_RESULT_HANDLED;
}

static void
object_path_handler_unregister(DBusConnection *conn,
                               void           *data)
{
#ifdef LASH_DEBUG
	object_path_t *path = data;
	lash_debug("Message handler of object path %s was unregistered",
	           (path && path->name) ? path->name : "<unknown>");
#endif /* LASH_DEBUG */
}

/* EOF */
