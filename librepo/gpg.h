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

#ifndef __LR_GPG_H__
#define __LR_GPG_H__

#include <glib.h>

G_BEGIN_DECLS

/** \defgroup   gpg GPG signature verification
 *  \addtogroup gpg
 *  @{
 */

/** A structure containing information about subkey.
 */
typedef struct tLrGpgSubkey LrGpgSubkey;

/** A structure containing information about a key with subkeys.
 */
typedef struct tLrGpgKey LrGpgKey;

/** Check detached signature of data.
 * @param signature_fd  File descriptor of signature file.
 * @param data_fd       File descriptor of data to verify.
 * @param home_dir      Configuration directory of OpenPGP engine
 *                      (e.g. "/home/user/.gnupg/"), if NULL default
 *                      config directory is used.
 * @param err           GError **
 * @return              returns TRUE if error is not set and FALSE if it is.
 */
gboolean
lr_gpg_check_signature_fd(int signature_fd,
                          int data_fd,
                          const char *home_dir,
                          GError **err);

/** Check detached signature of data.
 * @param signature_fn  Filename (path) of signature file.
 * @param data_fn       Filename (path) of data to verify.
 * @param home_dir      Configuration directory of OpenPGP engine
 *                      (e.g. "/home/user/.gnupg/"), if NULL default
 *                      config directory is used.
 * @param err           GError **
 * @return              returns TRUE if error is not set and FALSE if it is.
 */
gboolean
lr_gpg_check_signature(const char *signature_fn,
                       const char *data_fn,
                       const char *home_dir,
                       GError **err);


/** Import key into the keyring.
 * @param key           Pointer to memory buffer with key.
 * @param key_len       Length of the key.
 * @param home_dir      Configuration directory of OpenPGP engine
 *                      (e.g. "/home/user/.gnupg/"), if NULL default
 *                      config directory is used.
 * @param err           GError **
 * @return              returns TRUE if error is not set and FALSE if it is.
 */
gboolean
lr_gpg_import_key_from_memory(const char *key,
                              size_t key_len,
                              const char *home_dir,
                              GError **err);

/** Import key into the keyring.
 * @param key_fd        Filedescriptor of key file.
 * @param home_dir      Configuration directory of OpenPGP engine
 *                      (e.g. "/home/user/.gnupg/"), if NULL default
 *                      config directory is used.
 * @param err           GError **
 * @return              returns TRUE if error is not set and FALSE if it is.
 */
gboolean
lr_gpg_import_key_from_fd(int key_fd,
                          const char *home_dir,
                          GError **err);

/** Import key into the keyring.
 * @param key_fn        Filename (path) of key file.
 * @param home_dir      Configuration directory of OpenPGP engine
 *                      (e.g. "/home/user/.gnupg/"), if NULL default
 *                      config directory is used.
 * @param err           GError **
 * @return              returns TRUE if error is not set and FALSE if it is.
 */
gboolean
lr_gpg_import_key(const char *key_fn,
                  const char *home_dir,
                  GError **err);


/** List/export keys (and subkeys) from the keyring.
 * @param export_keys   If TRUE, the list also contains the exported keys.
 *                      Export is in ASCII-Armor format.
 * @param home_dir      Configuration directory of OpenPGP engine
 *                      (e.g. "/home/user/.gnupg/"), if NULL default
 *                      config directory is used.
 * @param err           GError **
 * @return              returns list of keys (and subkeys), or NULL if keyring is empty or an error occured.
 */
LrGpgKey *
lr_gpg_list_keys(gboolean export_keys,
                 const char *home_dir,
                 GError **err);

/** Get the next key from the list obtained from lr_gpg_list_keys.
 * @param key           Input key.
 * @return              returns next kye.
 */
const LrGpgKey *
lr_gpg_key_get_next(const LrGpgKey *key);

/** Get NULL terminated array of user IDs strings.
 * @param key           Input key.
 * @return              returns NULL terminated array of user IDs strings.
 */
char * const *
lr_gpg_key_get_userids(const LrGpgKey *key);

/** Get key in ASCII-Armor format (only if `key` was obtained from lr_gpg_list_keys with `export = TRUE`).
 * @param key           Input key.
 * @return              returns key in ACII-Armor format.
 */
const char *
lr_gpg_key_get_raw_key(const LrGpgKey *key);

/** Get a list of subkeys associated with the key. The first subkey is the primary key.
 * @param key           Input key.
 * @return              returns list of subkeys associated with the key.
 */
const LrGpgSubkey *
lr_gpg_key_get_subkeys(const LrGpgKey *key);

/** Release the list of keys obtained from lr_gpg_list_keys.
 * @param key           Input array of keys.
 */
void
lr_gpg_keys_free(LrGpgKey *key_array);

/** Get the next subkey from the list obtained from lr_gpg_key_get_subkeys.
 * @param key           Input subkey.
 * @return              returns next subkye.
 */
const LrGpgSubkey *
lr_gpg_subkey_get_next(const LrGpgSubkey *subkey);

/** Get subkey ID.
 * @param key           Input subkey.
 * @return              returns key ID.
 */
const char *
lr_gpg_subkey_get_id(const LrGpgSubkey *subkey);

/** Get fingerprint of the subkey in hex digit form.
 * @param key           Input subkey.
 * @return              returns fingerprint of the subkey in hex digit form.
 */
const char *
lr_gpg_subkey_get_fingerprint(const LrGpgSubkey *subkey);

/** Get the creation timestamp.
 * @param key           Input subkey.
 * @return              returns Get the creation timestamp, -1 if invalid, 0 if not available..
 */
long int
lr_gpg_subkey_get_timestamp(const LrGpgSubkey *subkey);

/** Get information if the subkey can be used for signing.
 * @param key           Input subkey.
 * @return              returns TRUE if subkey can be used for signing.
 */
gboolean
lr_gpg_subkey_get_can_sign(const LrGpgSubkey *subkey);

/** @} */

G_END_DECLS

#endif
