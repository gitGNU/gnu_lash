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

#ifndef __LIBLASH_DBUS_IFACE_CLIENT_H__
#define __LIBLASH_DBUS_IFACE_CLIENT_H__

#include <stdbool.h>

#include "dbus/interface.h"

#include "lash/types.h"

extern const interface_t g_liblash_interface_client;

// TODO: These three belong somewhere else; client.h perhaps?

/** Run the client callback for @ref LashEvent @a task_event. Commit any data
 * written inside the callback with @ref lash_write() to the server, and send
 * a success or failure message if necessary.<br />
 * This function is meant to be used only for save-type tasks.
 * @param client Pointer to client.
 * @param task_event Task event type.
 */
void
lash_new_save_task(lash_client_t  *client,
                   enum LashEvent  task_event);

void
lash_new_quit_task(lash_client_t *client);

bool
check_client_cb(lash_client_t *client,
                method_call_t *call);

#endif /* __LIBLASH_DBUS_IFACE_CLIENT_H__ */
