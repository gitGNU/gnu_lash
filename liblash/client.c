/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *   Copyright (C) 2002 Robert Ham <rah@bash.sh>
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

#include "client.h"
#include "common/safety.h"
#include "common/debug.h"

lash_client_t *
lash_client_new(void)
{
	return lash_calloc(1, sizeof(lash_client_t));
}

void
lash_client_destroy(lash_client_t * client)
{
	if (client) {
		lash_free(&client->class);
		lash_free(&client->project_name);

		if (client->argv) {
			int i;
			for (i = 0; i < client->argc; ++i) {
				free(client->argv[i]);
			}
			free(client->argv);
		}

		free(client);
	}
}

void
lash_client_close(lash_client_t *client)
{
	if (client)
		client->quit = true;
}

bool
lash_client_init_task_msg(lash_client_t *client)
{
	method_msg_t *msg;
	DBusMessageIter *iter, *array_iter;

	lash_debug("Initializing task message");

	msg = &client->task_msg;
	iter = &client->task_msg_iter;
	array_iter = &client->task_msg_array_iter;

	if (!method_call_init(msg, client->dbus_service,
	                      NULL,
	                      method_default_handler,
	                      "org.nongnu.LASH",
	                      "/",
	                      "org.nongnu.LASH.Server",
	                      "CommitData")) {
		lash_error("Failed to initialise CommitData method call");
		return false;
	}

	dbus_message_iter_init_append(msg->message, iter);

	if (!dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT64, &client->pending_task)) {
		lash_error("Failed to write task ID");
		goto fail;
	}

	if (!dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "{sv}", array_iter)) {
		lash_error("Failed to open config array container");
		goto fail;
	}

	return true;

fail:
	dbus_message_unref(msg->message);
	msg->message = NULL;

	return false;
}

/* EOF */
