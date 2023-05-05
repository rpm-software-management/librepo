/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2023  Jaroslav Rohel <jrohel@redhat.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __LR_GPG_INTERNAL_H__
#define __LR_GPG_INTERNAL_H__

#include <glib.h>

struct tLrGpgSubkey {
    gboolean has_next;   // FALSE if this is the last subkey in the list
    char *id;            // subkey id
    char *fingerprint;   // fingerprint of the subkey in hex digit form
    long int timestamp;  // creation timestamp, -1 if invalid, 0 if not available
    gboolean can_sign;   // TRUE if subkey can be used for signing
};

struct tLrGpgKey {
    gboolean has_next;     // FALSE if this is the last subkey in the list
    char **uids;           // NULL terminated array of user IDs strings
    LrGpgSubkey *subkeys;  // list of subkeys associated with the key. The first subkey is the primary key
    char *raw_key;         // key in ACII-Armor format
};

#endif
