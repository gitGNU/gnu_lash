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

#ifndef __LASH_DBUS_SIGNAL_H__
#define __LASH_DBUS_SIGNAL_H__

#include <dbus/dbus.h>

#include "dbus/types.h"

struct _signal_msg
{
	DBusConnection *connection;
	DBusMessage    *message;
};

struct _signal_arg
{
	const char *name;
	const char *type;
};

struct _signal
{
	const char         *name;
	const signal_arg_t *args;
};

void
signal_new_single(service_t  *service,
                  const char *path,
                  const char *interface,
                  const char *name,
                  int         type,
                  const void *arg);

void
signal_new_valist(service_t  *service,
                  const char *path,
                  const char *interface,
                  const char *name,
                  int         type,
                              ...);

#define SIGNAL_ARGS_BEGIN(signal_name)                         \
static const struct _signal_arg signal_name ## _args_dtor[] =  \
{

#define SIGNAL_ARG_DESCRIBE(arg_name, arg_type)                \
        {                                                      \
                .name = arg_name,                              \
                .type = arg_type                               \
        },

#define SIGNAL_ARGS_END                                        \
        {                                                      \
                .name = NULL,                                  \
                .type = NULL                                   \
        }                                                      \
};

#define SIGNALS_BEGIN                                          \
static const struct _signal signals_dtor[] =                   \
{

#define SIGNAL_DESCRIBE(signal_name)                           \
        {                                                      \
                .name = # signal_name,                         \
                .args = signal_name ## _args_dtor              \
        },

#define SIGNALS_END                                            \
        {                                                      \
                .name = NULL,                                  \
                .args = NULL                                   \
        }                                                      \
};

#endif /* __LASH_DBUS_SIGNAL_H__ */
