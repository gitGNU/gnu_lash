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

#ifndef __LASH_TYPES_H__
#define __LASH_TYPES_H__

#include <stdbool.h>
#include <stdint.h>
#include <uuid/uuid.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Events that LASH clients can trigger or monitor. */
enum LashEvent
{
	LASH_EVENT_INVALID = 0,

	LASH_EVENT_SAVE,
	LASH_EVENT_LOAD,
	LASH_EVENT_QUIT,
	LASH_EVENT_CHANGE_NAME,
	LASH_EVENT_CHANGE_PROJECT,
	LASH_EVENT_CHANGE_PROJECT_NAME,

	LASH_EVENT_PROJECT_ADDED,
	LASH_EVENT_PROJECT_REMOVED,
	LASH_EVENT_PROJECT_PATH_CHANGED,
	LASH_EVENT_PROJECT_NAME_CHANGED,
	LASH_EVENT_PROJECT_CLIENT_ADDED,
	LASH_EVENT_PROJECT_CLIENT_REMOVED,
	LASH_EVENT_CLIENT_NAME_CHANGED,
	LASH_EVENT_CLIENT_JACK_NAME_CHANGED,
	LASH_EVENT_SERVER_PROGRESS,
};

/** Datatypes that LASH clients can store and retrieve. */
enum LashType
{
	LASH_TYPE_INVALID = 0,   /**< Invalid type. Must never be used as parameter to any LASH function without explicit permission stated in this document. */
	LASH_TYPE_DOUBLE  = 'd', /**< 64-bit double. */
	LASH_TYPE_INTEGER = 'u', /**< 32-bit integer. */
	LASH_TYPE_STRING  = 's', /**< UTF-8 character string. */
	LASH_TYPE_RAW     = '-'  /**< Raw data. */
};

/** Flags for relaying status and configuration information to the LASH server
 * during initial client connection. Use these to construct the <i>flags</i>
 * parameter of \ref lash_client_open by OR-ing your choices together.
 */
enum LashFlags
{
	LashClientIsController  = 0x0001, /**< The client is a controller. */
	LashClientNeedsTerminal = 0x0002  /**< The client needs a terminal to run. */
};

typedef union
{
	double    d;
	int32_t   i;
	uint32_t  u;
	char     *s;
	char     *r;
} lash_value_t;

/** The LASH client handle. Represents the client's connection to LASH.
 * \a lash_client_t is an opaque type and objects must only be accessed using
 * the appropriate LASH API functions. */
typedef struct _lash_client lash_client_t;

/** The client event callback function type. */
typedef bool (*LashClientCallback) (enum LashEvent  type,
                                    void           *user_data);

/** The Controller event callback function type. */
typedef void (*LashControlCallback) (enum LashEvent  type,
                                     const char     *string1,
                                     const char     *string2,
                                     uuid_t          client_id,
                                     void           *user_data);

#ifdef __cplusplus
}
#endif

#endif /* __LASH_TYPES_H__ */
