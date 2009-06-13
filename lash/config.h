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

#include <stdbool.h>

#include <lash/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Append a key-value pair of typed data to a config message.
 *
 * @param handle An opaque config message handle passed
 *        to the callback function by liblash.
 * @param key The key string pointer.
 * @param value The value pointer.
 * @param type The value type. Must be one out of \ref LASH_TYPE_DOUBLE,
 *        \ref LASH_TYPE_INTEGER, and \ref LASH_TYPE_STRING.
 * @return True if writing succeeded, false otherwise.
 */
bool
lash_config_write(lash_config_handle_t *handle,
                  const char           *key,
                  const void           *value,
                  int                   type);

/**
 * Append a key-value pair of raw data to a config message.
 *
 * @param handle An opaque config message handle passed
 *        to the callback function by liblash.
 * @param key The key string pointer.
 * @param buf The data buffer pointer.
 * @param size The size of the data buffer in bytes.
 * @return True if writing succeeded, false otherwise.
 */
bool
lash_config_write_raw(lash_config_handle_t *handle,
                      const char           *key,
                      const void           *buf,
                      int                   size);

/**
 * Read a key-value pair of data from a config message.
 *
 * @param handle An opaque config message handle passed
 *        to the callback function by liblash.
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
lash_config_read(lash_config_handle_t  *handle,
                 const char           **key_ptr,
                 void                  *value_ptr,
                 int                   *type_ptr);

#ifdef __cplusplus
}
#endif

#endif /* __LASH_CONFIG_H__ */
