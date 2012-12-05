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
    LRO_SENTINEL,    /*!<  */
} lr_HandleOption; /*!< Handle config options */

/* Handle info options */
typedef enum {
    LRI_URL,
} lr_HandleInfoOption;


lr_Handle lr_handle_init();
void lr_handle_free(lr_Handle handle);

/* look at: url.c - Curl_setopt() */
int lr_handle_setopt(lr_Handle handle, lr_HandleOption option, ...);
int lr_handle_perform(lr_Handle handle, lr_Result result);
int lr_handle_last_curl_error(lr_Handle);
int lr_handle_last_curlm_error(lr_Handle);
const char *lr_handle_last_curl_strerror(lr_Handle handle);
const char *lr_handle_last_curlm_strerror(lr_Handle handle);

#ifdef __cplusplus
}
#endif

#endif
