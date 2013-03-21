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

#include <signal.h>

#include "types.h"
#include "handle.h"
#include "checksum.h"
#include "rcodes.h"
#include "internal_mirrorlist.h"
#include "curltargetlist.h"

/** \defgroup   curl    Set of function for downloading via curl
 */

extern volatile sig_atomic_t lr_interrupt;
void lr_sigint_handler(int sig);

/** \ingroup curl
 * Simplified version lr_curl_single_download_resume. Whole file is
 * downloaded without try to resume and user callback in handle is not used.
 * For more information look at lr_curl_single_download_resume.
 * @param handle        Librepo handle
 * @param url           Full URL
 * @param fd            Opened file descriptor.
 * @return              ::lr_Rc value.
 */
#define lr_curl_single_download(handle, url, fd) \
            lr_curl_single_download_resume((handle), (url), (fd), 0, 0)

/** \ingroup curl
 * Download one single file.
 * @param handle        Librepo handle
 * @param url           Full URL
 * @param fd            Opened file descriptor where downloaded data
 *                      will be written. Data writing starts on current
 *                      descriptor position, no seek or truncation
 *                      is performed (except situation when offset is -1).
 * @param offset        Offset where to start downloading. If 0, do not
 *                      resume and download whole file. If offset > 0, try
 *                      start download from this offset (no seek performed!).
 *                      If offset == -1 offset autodetection is used
 *                      (seek to the SEEK_END, obtain offset by ftell())
 *                      and start writing from the current (SEEK_END)
 *                      position..
 * @param use_cb        Use user callback from librepo handle? 0 == No
 * @return              ::lr_Rc value.
 */
int lr_curl_single_download_resume(lr_Handle handle,
                                   const char *url,
                                   int fd,
                                   long long offset,
                                   int use_cb);

/** \ingroup curl
 * Simplified version of lr_curl_single_mirrored_download_resume.
 * Whole file is downloaded without try to resume and user callback in handle
 * is not used.
 * @param handle        Librepo handle
 * @param path          Relative part of URL. Mirror URL will be prepended
 *                      to this path.
 * @param fd            Openede file descriptor.
 * @param chksum_t      CHecksum type.
 * @param chksum        Expected checksum value or NULL. If NULL, checksum
 *                      will not be checked.
 * @return              ::lr_Rc value.
 */
#define lr_curl_single_mirrored_download(handle, path, fd, chksum_t, chksum)\
            lr_curl_single_mirrored_download_resume((handle), \
                                                     (path), \
                                                     (fd), \
                                                     (chksum_t), \
                                                     (chksum), \
                                                     0, \
                                                     0)

/** \ingroup curl
 * Download single file from a first working mirror.
 * @param handle        Librepo handle.
 * @param path          Relative part of URL. Mirror URL will be prepended
 *                      to this path.
 * @param fd            Opened file descriptor.
 * @param checksum_type Checksum type.
 * @param checksum      Expected checksum of file or NULL. If NULL, checksum
 *                      will not be checked.
 * @param offset        Offset (same as ::lr_curl_single_download_resume).
 * @param use_cb        Use user callback from librepo handle? 0 == No.
 * @return              ::lr_Rc value.
 */
int lr_curl_single_mirrored_download_resume(lr_Handle handle,
                                            const char *path,
                                            int fd,
                                            lr_ChecksumType checksum_type,
                                            const char *checksum,
                                            long long offset,
                                            int use_cb);

/** \ingroup curl
 * Download several files at once.
 * @param handle        Librepo handle.
 * @param targets       List of targets to download.
 * @return              ::lr_Rc value.
 */
int lr_curl_multi_download(lr_Handle handle, lr_CurlTargetList targets);

#ifdef __cplusplus
}
#endif

#endif
