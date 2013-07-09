/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2013  Tomas Mlcoch
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

#ifndef LR_DOWNLOADTARGET_H
#define LR_DOWNLOADTARGET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <glib.h>
#include "checksum.h"

/** Single download target
 */
typedef struct {
    char *path; /*!<
        Relative path for URL (URL: "http://foo.bar/stuff",
        path: "somestuff.xml") */

    char *baseurl; /*!<
        Base URL for this target. If used, then mirrorlist will be ignored. */

    int fd; /*!<
        Opened file descriptor where data will be written */

    lr_ChecksumType checksumtype; /*!<
        Checksum type */

    char *checksum; /*!<
        Expected checksum value or NULL */

    int resume; /*!<
        Resume:
         0  - no resume, download whole file,
        -1 - autodetect offset for resume */

    GStringChunk *chunk; /*!<
        Chunk for strings used in this structure. */

} lr_DownloadTarget;

/** Create new empty ::lr_DownloadTarget.
 * @return              New allocated target.
 */
lr_DownloadTarget *
lr_downloadtarget_new(const char *path,
                      const char *baseurl,
                      int fd,
                      lr_ChecksumType checksumtype,
                      const char *checksum,
                      int resume);

/** Free a ::lr_DownloadTarget element and its content.
 * @param target        Target to free.
 */
void
lr_downloadtarget_free(lr_DownloadTarget *target);

#ifdef __cplusplus
}
#endif

#endif
