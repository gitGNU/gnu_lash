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

#ifndef __LIBLASH_CLIENT_H__
#define __LIBLASH_CLIENT_H__

#include <stdint.h>
#include <pthread.h>
#include <uuid/uuid.h>
#include <dbus/dbus.h>

#include "config.h"

#include "dbus/service.h"
#include "dbus/method.h"

#include "lash/types.h"

struct _lash_client
{
	char             *class;
	uuid_t            id;
	char             *name; // TODO: Get name from server
	char             *project_name;
	int               argc;
	char            **argv;
	char             *working_dir;
	uint32_t          flags;

	service_t        *dbus_service;
	pthread_t         thread_id;
	volatile bool     quit;
	uint64_t          pending_task;
	uint8_t           task_event;
	method_msg_t      task_msg;
	DBusMessageIter   task_msg_iter;
	DBusMessageIter   task_msg_array_iter;
	uint8_t           task_progress;

	LashClientCallback  client_cb;
	void               *client_data;

	LashControlCallback  control_cb;
	void                *control_data;
};

lash_client_t *
lash_client_new(void);

void
lash_client_destroy(lash_client_t *client);

bool
lash_client_init_task_msg(lash_client_t *client);

#endif /* __LIBLASH_CLIENT_H__ */
