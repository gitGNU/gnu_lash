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

#ifndef __LASH_CONFIG_H__
#define __LASH_CONFIG_H__

#include <lash/types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* __LASH_CONFIG_H__ */
