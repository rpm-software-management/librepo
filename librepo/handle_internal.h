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

#ifndef LR_HANDLE_INTERNAL_H
#define LR_HANDLE_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <curl/curl.h>

#include "types.h"

struct _lr_Handle {
    CURL            *curl_handle;   /*!< CURL handle */
    int             update;         /*!< Just update existing repo */
    char            *baseurl;       /*!< Base URL of repo */
    char            *mirrorlist;    /*!< Mirrorlist or URL */
    int             local;          /*!< Do not duplicate local data */
    char            *used_mirror;   /*!< Finally used mirror (if any) */
    int             retries;        /*!< Number of maximum retries */
    char            *destdir;       /*!< Destination directory */
    lr_Repotype     repotype;       /*!< Type of repository */
    lr_Checks       checks;         /*!< Which check sould be applied */
    long            status_code;    /*!< Last HTTP or FTP status code */
    CURLcode        last_curl_error;/*!< Last curl error code */
    CURLMcode       last_curlm_error;/*!< Last curl multi handle error code */
    lr_YumRepoFlags yumflags;       /*!< Flags for downloading of yum repo */
    lr_ProgressCb   user_cb;        /*!< User progress callback */
    void            *user_data;     /*!< User data for callback */
};

#ifdef __cplusplus
}
#endif

#endif
