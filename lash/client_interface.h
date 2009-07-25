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

#ifndef __LASH_CLIENT_INTERFACE_H_
#define __LASH_CLIENT_INTERFACE_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Create a new LASH client handle and connect to the LASH server.
 *
 * The client is not immediately active after its creation, so be sure to
 * register a callback with \ref lash_set_client_callback() (optionally also
 * with \ref lash_set_control_callback()) and finally acticate the client with
 * \ref lash_activate().
 *
 * @param client_class A pointer to the client's class string. This string can
 *        can be thought of as a unique identifier for the client's "make and
 *        model". For instance, a sampler application called UnixSampler 2.0.13
 *        might want to set its \a client_class to something like
 *        "UnixSampler2.0".
 *        It is critical that different versions of the same program only use
 *        the same class string if the data they store with LASH is 100 %
 *        cross-compatible!
 * @param flags Client flags of type enum \ref LashFlags passed as an
 *        integer value by OR'ing them together.
 * @param argc The argc value passed to main().
 * @param argv The argv pointer passed to main().
 * @return Pointer to the newly created client handle, or NULL if a problem
 *         occurred.
 */
lash_client_t *
lash_client_open(const char      *client_class,
                 enum LashFlags   flags,
                 int              argc,
                 char           **argv);

/** Close connection to the LASH server and destroy the client handle.
 * @param client Pointer to the LASH client to close.
 */
void
lash_client_close(lash_client_t *client);

/** Activate a connected LASH client.
 * 
 * @param client Pointer to the LASH client to activate.
 * @return True if succesful, false otherwise.
 */
bool
lash_activate(lash_client_t *client);

/** TODO: Not yet implemented.
 */
bool
lash_deactivate(lash_client_t *client);

/** Get the client's current name.
 * @param client Pointer to the LASH client.
 * @return Pointer to the client's current name.
 * TODO: What if it's NULL?
 */
const char *
lash_get_client_name(lash_client_t *client);

/** Get the name of the LASH project the client currently belongs to.
 * @param client Pointer to the LASH client.
 * @return Pointer to the client's current project name.
 * TODO: What if it's NULL?
 */
const char *
lash_get_project_name(lash_client_t *client);

bool
lash_client_is_being_restored(lash_client_t *client);

/** Set the LASH client callback function.
 * (TrySave, Save, Load, SaveDataSet, LoadDataSet, Quit, NameChange,
 *  ProjectChange, PathChange)
 */
bool
lash_set_client_callback(lash_client_t      *client,
                         LashClientCallback  callback,
                         void               *user_data);

/**
 * Append a key-value pair of data to a Save message.
 *
 * @param client LASH client pointer.
 * @param key The key string pointer.
 * @param value The value pointer. For doubles and integers, must point to
 *              the desired value to append. For strings and raw data, must
 *              point to a pointer to the first element of the array to append.
 *              The simplest way to ensure all this is to store the value in a
 *              \ref lash_value_t variable and pass a pointer to that.
 * @param type The value type. Must be \ref LASH_TYPE_DOUBLE,
 *             \ref LASH_TYPE_INTEGER, \ref LASH_TYPE_STRING, or
 *             \ref LASH_TYPE_RAW. Setting this to \ref LASH_TYPE_RAW
 *             makes the \a size argument mandatory.
 * @param size The length of the value data. Only required when \a type is
 *             \ref LASH_TYPE_RAW, otherwise ignored.
 * @return True if writing data succeeded, false otherwise.
 */
bool
lash_write(lash_client_t       *client,
           const char          *key,
           const void          *value,
           const enum LashType  type,
           const int            size);

/**
 * Read a key-value pair of data from a Load message.
 *
 * @param client LASH client pointer.
 * @param key_ptr A pointer to the memory location in which to
 *        save the key pointer.
 * @param value_ptr A pointer to the memory location in which to
 *        save the value pointer.
 * @param type_ptr A pointer to the memory location in which to
 *        save the value type.
 * @return If reading succeeds the return value will be equal to the
 *         config value's size in bytes (for strings this includes the
 *         terminating NUL). If no data remains in the message the return
 *         value will 0, and -1 if an error occurred during reading.
 */
int
lash_read(lash_client_t  *client,
          const char    **key_ptr,
          void           *value_ptr,
          enum LashType  *type_ptr);

void
lash_notify_progress(lash_client_t *client,
                     uint8_t        percentage);

#ifdef __cplusplus
}
#endif

#endif /* __LASH_CLIENT_INTERFACE_H_ */
