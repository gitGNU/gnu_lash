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

#ifndef __LASH_CONTROL_INTERFACE_H__
#define __LASH_CONTROL_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif

lash_client_t *
lash_client_open_controller(void);

/**
 * Set the controller callback function.
 */
bool
lash_set_control_callback(lash_client_t       *client,
                          LashControlCallback  callback,
                          void                *user_data);

void
lash_control_load_project_path(lash_client_t *client,
                               const char    *project_path);

void
lash_control_name_project(lash_client_t *client,
                          const char    *project_name,
                          const char    *new_name);

void
lash_control_move_project(lash_client_t *client,
                          const char    *project_name,
                          const char    *new_path);

void
lash_control_save_project(lash_client_t *client,
                          const char    *project_name);

void
lash_control_close_project(lash_client_t *client,
                           const char    *project_name);

#ifdef __cplusplus
}
#endif

#endif /* __LASH_CONTROL_INTERFACE_H__ */
