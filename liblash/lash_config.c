/*
 *   LASH
 *
 *   Copyright (C) 2008-2009 Juuso Alasuutari <juuso.alasuutari@gmail.com>
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

#include <string.h>
#include <arpa/inet.h>
#include <rpc/xdr.h>

#include "client.h"
#include "common/debug.h"
#include "dbus/method.h"

static inline bool
init_write(lash_client_t *client)
{
	if (client->task_event != LASH_EVENT_SAVE) {
		lash_error("Can only write data during a Save operation");
		return false;
	}

	if (!client->task_msg.message && !lash_client_init_task_msg(client)) {
		lash_error("Failed to initialize task message");
		return false;
	}

	return true;
}

bool
lash_write(lash_client_t       *client,
           const char          *key,
           const void          *value,
           const enum LashType  type,
           const int            size)
{
	const void *value_ptr;
	char buf[8];

if (type == LASH_TYPE_STRING) lash_debug("!!!!!!!!!!! WRITING CONFIG %s = '%s'", key, *((const char **)value));
	if (!client || !key || !key[0] || !value) {
		lash_error("Invalid arguments");
		return false;
	}

	if (type == LASH_TYPE_DOUBLE) {
		XDR x;
		xdrmem_create(&x, (char *) buf, 8, XDR_ENCODE);
		if (!xdr_double (&x, (double *) value)) {
			lash_error("Failed to encode floating point value");
			return false;
		}
		value_ptr = buf;
	} else if (type == LASH_TYPE_INTEGER) {
		*((uint32_t *) buf) = htonl(*((uint32_t *) value));
		value_ptr = buf;
	} else if (type == LASH_TYPE_STRING || type == LASH_TYPE_RAW) {
		value_ptr = value;
	} else {
		lash_error("Invalid value type %i", type);
		return false;
	}

	if (!init_write(client))
		return false;

	lash_debug("Writing config \"%s\" of type '%c'", key, (char) type);

	if (!method_iter_append_dict_entry(&client->task_msg_array_iter, type, key, value_ptr, size)) {
		lash_error("Failed to append dict entry");
		return false;
	}

	return true;
}

int
lash_read(lash_client_t  *client,
          const char    **key_ptr,
          void           *value_ptr,
          enum LashType  *type_ptr)
{
	if (!client || !key_ptr || !value_ptr || !type_ptr) {
		lash_error("Invalid arguments");
		return -1;
	}

	int size;

	if (client->task_event != LASH_EVENT_LOAD) {
		lash_error("Can only read data during a Load operation");
		return -1;
	}

	/* No data left in message */
	if (dbus_message_iter_get_arg_type(&client->task_msg_array_iter) == DBUS_TYPE_INVALID)
		return 0;

	if (!method_iter_get_dict_entry(&client->task_msg_array_iter, key_ptr,
	                                value_ptr, (int *) type_ptr, &size)) {
		lash_error("Failed to read config message");
		return -1;
	}

	dbus_message_iter_next(&client->task_msg_array_iter);

	if (*type_ptr == LASH_TYPE_DOUBLE) {
		XDR x;
		double d;
		xdrmem_create(&x, (char *) value_ptr, 8, XDR_DECODE);
		if (!xdr_double (&x, &d)) {
			lash_error("Failed to decode floating point value");
			return -1;
		}
		*((double *) value_ptr) = d;
		size = sizeof(double);
	} else if (*type_ptr == LASH_TYPE_INTEGER) {
		uint32_t u = *((uint32_t *) value_ptr);
		*((uint32_t *) value_ptr) = ntohl(u);
		size = sizeof(uint32_t);
	} else if (*type_ptr == LASH_TYPE_STRING) {
		size = (int) strlen(*((const char **) value_ptr));
		if (size < 1) {
			lash_error("String length is 0");
			return -1;
		}
	} else if (*type_ptr != LASH_TYPE_RAW) {
		lash_error("Unknown value type %i", *type_ptr);
		return -1;
	}

	return size;
}

/* EOF */
