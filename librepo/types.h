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

#ifndef LR_TYPES_H
#define LR_TYPES_H

#include <glib.h>

G_BEGIN_DECLS

/** \defgroup   types   Basic types and constants
 *  \addtogroup types
 *  @{
 */

/** Handle object containing configration for repository metadata and
 * package downloading.
 */

/** Flags for available checks. */
typedef enum {
    LR_CHECK_GPG        = (1<<0),   /*!< GPG check */
    LR_CHECK_CHECKSUM   = (1<<1),   /*!< Checksum check */
} LrChecks;

/** Repo types flags. */
typedef enum {
    LR_YUMREPO          = (1<<1),   /*!< Yum repository */
    LR_SUSEREPO         = (1<<2),   /*!< YaST2 repository - Not implemented yet */
    LR_DEBREPO          = (1<<3),   /*!< Debian repository - Not implemented yet */
} LrRepotype;

/** Proxy types. */
typedef enum {
    LR_PROXY_HTTP,              /*!< HTTP proxy (Default) */
    LR_PROXY_HTTP_1_0,          /*!< HTTP 1.0 proxy */
    LR_PROXY_SOCKS4,            /*!< SOCKS4 proxy */
    LR_PROXY_SOCKS5,            /*!< SOCKS5 proxy */
    LR_PROXY_SOCKS4A,           /*!< SOCKS4A proxy */
    LR_PROXY_SOCKS5_HOSTNAME,   /*!< SOCKS5 proxy */
} LrProxyType;

/* Some common used arrays for LRO_YUMDLIST */

/** Predefined value for LRO_YUMDLIST option - Download whole repo. */
#define LR_YUM_FULL         NULL

/** Predefined value for LRO_YUMDLIST option - Download only repomd.xml. */
#define LR_YUM_REPOMDONLY   {NULL}

/** Predefined value for LRO_YUMDLIST option - Download only base xml files. */
#define LR_YUM_BASEXML      {"primary", "filelists", "other", NULL}

/** Predefined value for LRO_YUMDLIST option - Download only base db files. */
#define LR_YUM_BASEDB       {"primary_db", "filelists_db", "other_db", NULL}

/** Predefined value for LRO_YUMDLIST option - Download only primary,
 * filelists and prestodelta.
 */
#define LR_YUM_HAWKEY       {"primary", "filelists", "prestodelta", NULL}

/** Progress callback prototype
 * @param clientp           Pointer to user data.
 * @param total_to_download Total number of bytes to download
 * @param now_downloaded    Number of bytes currently downloaded
 * @return                  Returning a non-zero value from this callback
 *                          will cause to abort the transfer.
 */
typedef int (*LrProgressCb)(void *clientp,
                            double total_to_download,
                            double now_downloaded);

/** End callback prototype
 * Called when download is finished successfully
 * @param clientp           Pointer to user data.
 */
typedef void (*LrEndCb)(void *clientp);

/** Failure callback prototype
 * @param clientp           Pointer to user data.
 * @param msg               Error message.
 * @return                  Returning a non-zero value from this callback
 *                          will cause to abort the transfer.
 */
typedef void (*LrFailureCb)(void *clientp, const char *msg);

/** MirrorFailure callback prototype
 * @param clientp           Pointer to user data.
 * @param msg               Error message.
 */
typedef int (*LrMirrorFailureCb)(void *clientp, const char *msg);

/** @} */

G_END_DECLS

#endif
