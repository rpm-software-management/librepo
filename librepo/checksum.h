/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2012  Tomas Mlcoch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#ifndef LR_CHECKSUM_H
#define LR_CHECKSUM_H

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup checksum Functions for checksum calculating and checking.
 */

/** \ingroup checksum
 * Enum of supported checksum types.
 * NOTE! This enum guarantee to be sorted by "hash quality"
 */
typedef enum {
    LR_CHECKSUM_UNKNOWN,
    LR_CHECKSUM_MD2,        /*    The most weakest hash */
    LR_CHECKSUM_MD5,        /*  |                       */
    LR_CHECKSUM_SHA,        /*  |                       */
    LR_CHECKSUM_SHA1,       /*  |                       */
    LR_CHECKSUM_SHA224,     /*  |                       */
    LR_CHECKSUM_SHA256,     /*  |                       */
    LR_CHECKSUM_SHA384,     /* \|/                      */
    LR_CHECKSUM_SHA512,     /*    The most secure hash  */
} lr_ChecksumType;

/** \ingroup checksum
 * Convert checksum name (string) to lr_ChecksumType.
 * @param type      String with a checksum name (e.g. "sha1", "SHA256", ...)
 * @return          lr_ChecksumType value representing the checksum
 *                  or LR_CHECKSUM_UNKNOWN
 */
lr_ChecksumType lr_checksum_type(const char *type);

/** \ingroup checksum
 * Convert lr_ChecksumType to string
 * @param type      Checksum type
 * @return          Constant string with name of the checksum
 */
const char *lr_checksum_type_to_str(lr_ChecksumType type);

/** \ingroup checksum
 * Calculate checksum for data pointed by file descriptor.
 * @param type      Checksum type
 * @param fd        Opened file descriptor. Note: Function call only read()
 *                  on the descriptor and do not perform close() or seek()
 *                  to the beginning of file.
 */
char *lr_checksum_fd(lr_ChecksumType type, int fd);

/** \ingroup checksum
 * Calculate checksum for data pointed by file descriptor and
 * compare it to the expected checksum value.
 * @param type      Checksum type
 * @param fd        File descriptor
 * @param expected  String with expected checksum value
 * @return          0 if calculated checksum == expected checksum
 */
int lr_checksum_fd_cmp(lr_ChecksumType type, int fd, const char *expected);

#ifdef __cplusplus
}
#endif

#endif
