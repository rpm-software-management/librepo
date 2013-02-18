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

#ifndef LR_HANDLE_H
#define LR_HANDLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "result.h"

/** \defgroup   handle    Handle for downloading data
 *  \addtogroup handle
 *  @{
 */

/** Handle object containing configration for repository metadata and
 * package downloading.
 */
typedef struct _lr_Handle *lr_Handle;

/** Handle options for the ::lr_handle_setopt function. */
typedef enum {
    LRO_UPDATE,      /*!< (long 1 or 0) Update existing repo in ::lr_Result.
                          Update means download missing (previously omitted)
                          metadata file(s). */
    LRO_URL,         /*!< (char *) Base repo URL */
    LRO_MIRRORLIST,  /*!< (char *) Mirrorlist or metalink url */
    LRO_LOCAL,       /*!< (long 1 or 0) Do not duplicate local metadata, just
                          locate the old one */
    LRO_HTTPAUTH,    /*!< (long 1 or 0) Enable all supported method of HTTP
                          authentification. */
    LRO_USERPWD,     /*!< (char *) User and password for http authetification
                          in format user:password */
    LRO_PROXY,       /*!< (char *) Address of proxy server eg.
                          "proxy-host.com:8080" */
    LRO_PROXYPORT,   /*!< (long) Set port number for proxy separately. Default
                          port is 1080. */
    LRO_PROXYTYPE,   /*!< (::lr_ProxyType) Type of the proxy used. */
    LRO_PROXYAUTH,   /*!< (long 1 or 0) Enable all supported method for proxy
                          authentification */
    LRO_PROXYUSERPWD,/*!< (char *) User and password for proxy in format
                          user:password */
    LRO_PROGRESSCB,  /*!< (::lr_ProgressCb) Progress callback */
    LRO_PROGRESSDATA,/*!< (void *) Progress callback user data */
    LRO_RETRIES,     /*!< (long) Number of maximum retries for each file - TODO */
    LRO_MAXSPEED,    /*!< (unsigned long long) Maximum download speed
                          in bytes per second. Default is 0 = unlimited
                          download speed. */
    LRO_DESTDIR,     /*!< (char *) Where to save downloaded files */

    LRO_REPOTYPE,    /*!< (::lr_Repotype) Type of downloaded repo, currently
                          only supported is LR_YUMREPO. */
    LRO_CONNECTTIMEOUT,/*!< (long) Max time in sec for connection phase.
                            default timeout is 300 seconds. */
    LRO_IGNOREMISSING, /*!< (long) If you want to localise (LRO_LOCAL is enabled)
                            a incomplete local repository (eg. only primary
                            and filelists are present) you could use
                            LRO_YUMDLIST and specify only file that are
                            present, or use this option. */

    /* Repo common options */
    LRO_GPGCHECK,    /*!< (long 1 or 0) Check GPG signature if available */
    LRO_CHECKSUM,    /*!< (long 1 or 0) Check files checksum if available */

    /* LR_YUMREPO specific options */
    LRO_YUMDLIST,    /*!< (char **) Download only specified records
                          from repomd (e.g. ["primary", "filelists", NULL]).
                          Note: Last element of the list must be NULL! */
    LRO_YUMBLIST,    /*!< (char **) Do not download this specified records
                          from repomd (blacklist).
                          Note: Last element of the list must be NULL! */
    LRO_SENTINEL,    /*!<  */
} lr_HandleOption; /*!< Handle config options */

/** Handle options for the ::lr_handle_getinfo function. */
typedef enum {
    LRI_UPDATE,                 /* (long *) */
    LRI_URL,                    /* (char **) */
    LRI_MIRRORLIST,             /* (char **) */
    LRI_LOCAL,                  /* (long *) */
    LRI_DESTDIR,                /* (char **) */
    LRI_REPOTYPE,               /* (long *) */
    LRI_YUMDLIST,               /* (char ***) */
    LRI_YUMBLIST,               /* (char ***) */
    LRI_LASTCURLERR,            /* (long *) */
    LRI_LASTCURLMERR,           /* (long *) */
    LRI_LASTCURLSTRERR,         /* (char **) */
    LRI_LASTCURLMSTRERR,        /* (char **) */
    LRI_LASTBADSTATUSCODE,      /* (long *) */
    LRI_SENTINEL,
} lr_HandleInfoOption; /*!< Handle info options */

/** Return new handle.
 * @return              New allocated handle.
 */
lr_Handle lr_handle_init();

/** Frees handle and its content.
 * @param handle        Handle.
 */
void lr_handle_free(lr_Handle handle);

/** Set option (::lr_HandleOption) of the handle.
 * @param handle         Handle.
 * @param option        Option from ::lr_HandleOption enum.
 * @param ...           Value for the option.
 * @return              Librepo return code from ::lr_Rc enum.
 */
int lr_handle_setopt(lr_Handle handle, lr_HandleOption option, ...);

/** Get information from handle.
 * Most of returned pointers point directly to the handle internal
 * values and therefore you should assume that they are only valid until
 * any manipulation (lr_handle_setopt, lr_handle_perform, ...)
 * with handle occurs.
 * NOTE: You should not free or modify the memory returned by this
 * function unless it is explicitly mentioned!
 * @param handle        Librepo handle.
 * @param option        Option from ::lr_HandleInfoOption enum.
 * @param ...           Apropriate variable fro the selected option.
 * @return              Librepo return code ::lr_Rc.
 */
int lr_handle_getinfo(lr_Handle handle, lr_HandleOption option, ...);

/** Perform repodata download or location.
 * @param handle        Librepo handle.
 * @param result        Librepo result.
 * @return              Librepo return code from ::lr_Rc enum.
 */
int lr_handle_perform(lr_Handle handle, lr_Result result);

/** Return last encountered cURL error code from cURL.
 * @param handle        Librepo handle.
 * @return              cURL (CURLcode) return code.
 */
int lr_handle_last_curl_error(lr_Handle handle);

/** Return last encoutered cURL error code from cURL multi handle.
 * @param handle        Librepo handle.
 * @return              cURL multi (CURLMcode) return code.
 */
int lr_handle_last_curlm_error(lr_Handle handle);

/** Return last encountered HTTP/FTP status code (e.g. 404).
 * @param handle        Librepo handle.
 * @return              Last encountered HTTP/FTP status code.
 */
long lr_handle_last_bad_status_code(lr_Handle handle);

/** Return string representation of last encountered cURL error.
 * @param handle        Librepo handle.
 * @return              String with text description of error.
 */
const char *lr_handle_last_curl_strerror(lr_Handle handle);

/** Return string representation of last encountered cURL multi error.
 * @param handle        Librepo handle.
 * @return              String with text description of error.
 */
const char *lr_handle_last_curlm_strerror(lr_Handle handle);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
