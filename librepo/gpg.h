/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2012  Tomas Mlcoch
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

/** @} */

G_END_DECLS

#endif
