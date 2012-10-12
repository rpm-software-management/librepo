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

#include "librepo.h"

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

struct _lr_CurlTarget {
    char *url;
    int fd;
};
typedef struct _lr_CurlTarget * lr_CurlTarget;

lr_CurlTarget lr_target_init();
void lr_target_free(lr_CurlTarget target);

int lr_curl_single_download(lr_Handle handle,
                            const char *url,
                            int fd,
                            char *mandatory_suffix);

int lr_curl_multi_download(lr_Handle handle, lr_CurlTarget targets[], int not);

#ifdef __cplusplus
}
#endif

#endif
