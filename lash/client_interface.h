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
#include <lash/types.h>

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

void
lash_wait(lash_client_t *client);

void
lash_dispatch(lash_client_t *client);

bool
lash_dispatch_once(lash_client_t *client);

void
lash_notify_progress(lash_client_t *client,
                     uint8_t        percentage);

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


#endif /* __LASH_CLIENT_INTERFACE_H_ */
