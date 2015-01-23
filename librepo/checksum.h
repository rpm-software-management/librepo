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

#ifndef __LR_CHECKSUM_H__
#define __LR_CHECKSUM_H__

#include <glib.h>

G_BEGIN_DECLS

/** \defgroup   checksum    Checksum calculation and checking
 *  \addtogroup checksum
 *  @{
 */

/** Enum of supported checksum types.
 * NOTE! This enum guarantee to be sorted by "hash quality"
 */
typedef enum {
    LR_CHECKSUM_UNKNOWN,
    LR_CHECKSUM_MD5,        /*    The most weakest hash */
    LR_CHECKSUM_SHA1,       /*  |                       */
    LR_CHECKSUM_SHA224,     /*  |                       */
    LR_CHECKSUM_SHA256,     /*  |                       */
    LR_CHECKSUM_SHA384,     /* \|/                      */
    LR_CHECKSUM_SHA512,     /*    The most secure hash  */
} LrChecksumType;

/** Convert checksum name (string) to ::LrChecksumType.
 * @param type      String with a checksum name (e.g. "sha1", "SHA256", ...)
 * @return          ::LrChecksumType value representing the checksum
 *                  or LR_CHECKSUM_UNKNOWN
 */
LrChecksumType
lr_checksum_type(const char *type);

/** Convert LrChecksumType to string
 * @param type      Checksum type
 * @return          Constant string with name of the checksum
 */
const char *
lr_checksum_type_to_str(LrChecksumType type);

/** Calculate checksum for data pointed by file descriptor.
 * @param type      Checksum type
 * @param fd        Opened file descriptor. Function seeks to the begin
 *                  of the file.
 * @param err       GError **
 * @return          Malloced checksum string or NULL on error.
 */
char *
lr_checksum_fd(LrChecksumType type, int fd, GError **err);

/** Calculate checksum for data pointed by file descriptor and
 * compare it to the expected checksum value.
 * @param type      Checksum type
 * @param fd        File descriptor
 * @param expected  String with expected checksum value
 * @param caching   Cache/Use cached checksum value as extended file attr.
 * @param matches   Set pointed variable to TRUE if checksum matches.
 * @param err       GError **
 * @return          returns TRUE if error is not set and FALSE if it is
 */
gboolean
lr_checksum_fd_cmp(LrChecksumType type,
                   int fd,
                   const char *expected,
                   gboolean caching,
                   gboolean *matches,
                   GError **err);

/** Calculate checksum for data pointed by file descriptor and
 * compare it to the expected checksum value.
 * @param type          Checksum type
 * @param fd            File descriptor
 * @param expected      String with expected checksum value
 * @param caching       Cache/Use cached checksum value as extended file attr.
 * @param matches       Set pointed variable to TRUE if checksum matches.
 * @param calculated    If not NULL, the calculated checksum will be pointed
 *                      here, the pointed string must be freed by caller.
 * @param err           GError **
 * @return              returns TRUE if error is not set and FALSE if it is
 */
gboolean
lr_checksum_fd_compare(LrChecksumType type,
                       int fd,
                       const char *expected,
                       gboolean caching,
                       gboolean *matches,
                       gchar **calculated,
                       GError **err);

/** @} */

G_END_DECLS

#endif
