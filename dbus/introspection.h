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

#ifndef __LASH_DBUS_INTROSPECTION_H__
#define __LASH_DBUS_INTROSPECTION_H__

#include "dbus/types.h"
#include "dbus/interface.h"
#include "dbus/object_path.h"

DBusMessage *
introspection_new(object_path_t *path);

void
introspection_destroy(object_path_t *path);

extern const interface_t g_dbus_interface_dtor_introspectable;

#endif /* __LASH_DBUS_INTROSPECTION_H__ */
