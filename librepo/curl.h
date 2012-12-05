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

#ifndef LR_CURL_H
#define LR_CURL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "checksum.h"
#include "rcodes.h"
#include "internal_mirrorlist.h"
#include "curltargetlist.h"

/* Return codes of the module:
 *  LRE_OK          everything ok
 *  LRE_IO          input/output error
 *  LRE_NOURL       no usable URL
 *  LRE_CURL_DUP    cannot duplicate curl handle
 *  LRE_CURL        curl err
 *  LRE_MCURL       curl multi handle error
 *  LRE_BAD_STATUS  HTTP or FTP returned status code which
 *                  do not represent success
 */

#define lr_curl_single_download(HANDLE, URL, FD) \
            lr_curl_single_download_resume((HANDLE), (URL), (FD), 0)

int lr_curl_single_download_resume(lr_Handle handle, const char *url, int fd, long offset);

#define lr_curl_single_mirrored_download(HANDLE, FILENAME, FD, CHKSUM_T, CHKSUM)\
            lr_curl_single_mirrored_download_resume((HANDLE), \
                                                     (FILENAME), \
                                                     (FD), \
                                                     (CHKSUM_T), \
                                                     (CHKSUM), \
                                                     0)

int lr_curl_single_mirrored_download_resume(lr_Handle handle,
                                            const char *filename,
                                            int fd,
                                            lr_ChecksumType checksum_type,
                                            const char *checksum,
                                            long offset);

int lr_curl_multi_download(lr_Handle handle, lr_CurlTargetList targets);

#ifdef __cplusplus
}
#endif

#endif
