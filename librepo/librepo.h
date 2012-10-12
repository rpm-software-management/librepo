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

#ifndef LR_LIBREPO_H
#define LR_LIBREPO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <curl/curl.h>

#include "types.h"

/* Return/Error codes */
typedef enum {
    LRE_OK,                         /*!< everything is ok */
    LRE_BAD_FUNCTION_ARGUMENT,      /*!< bad function argument */

    /* lr_setopt specific */
    LRE_BAD_OPTION_ARGUMENT,        /*!< bad argument of the option */
    LRE_UNKNOWN_OPTION,             /*!< library doesn't know the option */
    LRE_CURL_SETOPT,                /*!< cURL doesn't know the option.
                                         Too old curl version? */
    LRE_ALREADYUSEDRESULT,          /*!< lr_Result object is not clea.r */
    LRE_INCOMPLETERESULT,           /*!< lr_Result doesn't contain all what
                                         is needed */
    LRE_CURL_DUP,                   /*!< cannot duplicate curl handle */
    LRE_CURL,                       /*!< cURL error. Use the
                                         lr_last_curl_error to get CURLcode */
    LRE_CURLM,                      /*!< cULR multi handle error. Use the
                                         lr_last_mculr_error to get CURLMcode */
    LRE_BAD_STATUS,                 /*!< HTTP or FTP returned status code which
                                         do not represent success
                                         (file doesn't exists, etc.) */
    LRE_NOTLOCAL,                   /*!< URL is not a local address */
    LRE_SELECT,                     /*!< error while call select() on set
                                         of sockets */
    LRE_CANNOT_CREATE_DIR,          /*!< cannot create a directory in output
                                         dir (the directory already exists?) */
    LRE_IO,                         /*!< input output error */
    LRE_ML_BAD,                     /*!< bad metalink file (metalink doesn't
                                         contain needed file) */
    LRE_ML_XML,                     /*!< metalink XML parse error */
    LRE_BAD_CHECKSUM,               /*!< bad checksum */
    LRE_REPOMD_XML,                 /*!< repomd XML parse error */
    LRE_NOURL,                      /*!< no usable URL found */
    LRE_CANNOT_CREATE_TMP,          /*!< cannot create tmp directory */
    LRE_UNKNOWN_CHECKSUM,           /*!< unknown type of checksum is need to
                                         calculate to verify one or more file */
    LRE_UNKNOWN_ERROR,              /*!< unknown error - last element in
                                         error codes enum */
} lr_Rc; /*!< Return codes */

/* Handle info options */
typedef enum {
    LRI_URL,
} lr_HandleInfoOption;

/* Handle options */
typedef enum {
    LRO_UPDATE,      /*!< Update existing repo in lr_Result */
    LRO_URL,         /*!< Base repo URL */
    LRO_MIRRORLIST,  /*!< Mirrorlist or metalink url */
    LRO_LOCAL,       /*!< Do not duplicate local metadata, just locate
                          the old one */
    LRO_HTTPAUTH,    /*!< Enable all supported method of HTTP
                          authentification. */
    LRO_USERPWD,     /*!< User and password for http authetification in format
                          "user:password" */
    LRO_PROXY,       /*!< Address of proxy server eg. "proxy-host.com:8080" */
    LRO_PROXYPORT,   /*!< Set port number for proxy separately */
    LRO_PROXYSOCK,   /*!< Set type of proxy to SOCK (default is assumed
                          HTTP proxy) */
    LRO_PROXYAUTH,   /*!< Enable all supported method for proxy
                          authentification */
    LRO_PROXYUSERPWD,/*!< User and password for proxy */
    LRO_PROGRESSCB,  /*!< Progress callback */
    LRO_PROGRESSDATA,/*!< Progress callback user data */
    LRO_RETRIES,     /*!< Number of maximum retries for each file - TODO */
    LRO_MAXSPEED,    /*!< Maximum download speed in bytes per second */
    LRO_DESTDIR,     /*!< Where to save downloaded files */

    LRO_REPOTYPE,    /*!< Type of downloaded repo, currently only supported
                          is LR_YUMREPO. */

    /* Repo common options */
    LRO_GPGCHECK,    /*!< Check GPG signature if available - TODO */
    LRO_CHECKSUM,    /*!< Check files checksum if available */

    /* LR_YUMREPO specific options */
    LRO_YUMREPOFLAGS,/*!< Download only specified files */

} lr_HandleOption; /*!< Handle config options */

/* Result option */
typedef enum {
    LRR_YUM_REPO,
    LRR_YUM_REPOMD,
} lr_ResultInfoOption;

void lr_global_init();
void lr_global_cleanup();

lr_Result lr_result_init();
void lr_result_clear(lr_Result);
void lr_result_free(lr_Result);
int lr_result_getinfo(lr_Result result, lr_ResultInfoOption option, ...);

lr_Handle lr_handle_init();
void lr_handle_free(lr_Handle handle);

/* look at: url.c - Curl_setopt() */
int lr_setopt(lr_Handle handle, lr_HandleOption option, ...);
int lr_perform(lr_Handle handle, lr_Result result);
int lr_last_curl_error(lr_Handle);
int lr_last_curlm_error(lr_Handle);

/* Yum repo */

lr_YumRepo lr_yum_repo_init();
void lr_yum_repo_clear(lr_YumRepo repo);
void lr_yum_repo_free(lr_YumRepo repo);

#ifdef __cplusplus
}
#endif

#endif
