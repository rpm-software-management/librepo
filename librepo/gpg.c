/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2012  Tomas Mlcoch
 * Copyright (C) 2022  Jaroslav Rohel <jrohel@redhat.com>
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

#include "gpg.h"
#include "gpg_internal.h"

const LrGpgKey *
lr_gpg_key_get_next(const LrGpgKey *key) {
    return key->has_next ? ++key : NULL;
}

char * const *
lr_gpg_key_get_userids(const LrGpgKey *key) {
    return key->uids;
}

const char *
lr_gpg_key_get_raw_key(const LrGpgKey *key) {
    return key->raw_key;
}

const LrGpgSubkey *
lr_gpg_key_get_subkeys(const LrGpgKey *key) {
    return key->subkeys;
}

static void
lr_gpg_subkeys_free(LrGpgSubkey *subkeys) {
    for (LrGpgSubkey *item = subkeys; item; ++item) {
        g_free(item->fingerprint);
        g_free(item->id);
        if (!item->has_next) {
            break;
        }
    }
    g_free(subkeys);
}

void
lr_gpg_keys_free(LrGpgKey *keys) {
    for (LrGpgKey *item = keys; item; ++item) {
        g_free(item->raw_key);
        lr_gpg_subkeys_free(item->subkeys);
        g_strfreev(item->uids);
        if (!item->has_next) {
            break;
        }
    }
    g_free(keys);
}

const LrGpgSubkey *
lr_gpg_subkey_get_next(const LrGpgSubkey *subkey) {
    return subkey->has_next ? ++subkey : NULL;
}

const char *
lr_gpg_subkey_get_id(const LrGpgSubkey *subkey) {
    return subkey->id;
}

const char *
lr_gpg_subkey_get_fingerprint(const LrGpgSubkey *subkey) {
    return subkey->fingerprint;
}

long int
lr_gpg_subkey_get_timestamp(const LrGpgSubkey *subkey) {
    return subkey->timestamp;
}

gboolean
lr_gpg_subkey_get_can_sign(const LrGpgSubkey *subkey) {
    return subkey->can_sign;
}
