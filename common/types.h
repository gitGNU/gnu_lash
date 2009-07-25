/*
 *   LASH
 *
 *   Copyright (C) 2009 Juuso Alasuutari <juuso.alasuutari@gmail.com>
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

#ifndef __COMMON_TYPES_H__
#define __COMMON_TYPES_H__

/** These flags are used internally by lashd and liblash. They begin from the
 * 17th bit (0x10000) as the lowest 16 are reserved for \ref LashClientFlags.
 */
enum LashInternalFlags
{
	LashClientIsRestored   = 0x10000,  /**< The LASH server sets this flag for clients that are launched for restoring to a previously saved state. */
	LashClientIsOrdinary   = 0x20000,  /**< The LASH library sets this flag for ordinary clients, i.e. clients which take part in projects. Notice that an ordinary client may simultaneously be a controller, so this flag can coexist with \ref LashClientIsController. */
	LashClientIsConnected  = 0x40000,  /**< The LASH library sets this flag for connected clients, and unsets it for disconnected ones. */
	LashClientIsActive     = 0x80000   /**< The LASH library sets this flag for active clients, and unsets it for inactive ones. */
};

#endif /* __COMMON_TYPES_H__ */
