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

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup   types   Basic types and constants
 *  \addtogroup types
 *  @{
 */

/** Handle object containing configration for repository metadata and
 * package downloading.
 */
//typedef struct _lr_Handle *lr_Handle;

/** Result object containing information about downloaded/located repository. */
typedef struct _lr_Result *lr_Result;

/** Flags for available checks. */
typedef enum {
    LR_CHECK_GPG        = (1<<0),   /*!< GPG check */
    LR_CHECK_CHECKSUM   = (1<<1),   /*!< Checksum check */
} lr_Checks;

/** Repo types flags. */
typedef enum {
    LR_YUMREPO          = (1<<1),   /*!< Yum repository */
    LR_SUSEREPO         = (1<<2),   /*!< YaST2 repository - Not implemented yet */
    LR_DEBREPO          = (1<<3),   /*!< Debian repository - Not implemented yet */
} lr_Repotype;

/** Proxy types. */
typedef enum {
    LR_PROXY_HTTP,              /*!< HTTP proxy (Default) */
    LR_PROXY_HTTP_1_0,          /*!< HTTP 1.0 proxy */
    LR_PROXY_SOCKS4,            /*!< SOCKS4 proxy */
    LR_PROXY_SOCKS5,            /*!< SOCKS5 proxy */
    LR_PROXY_SOCKS4A,           /*!< SOCKS4A proxy */
    LR_PROXY_SOCKS5_HOSTNAME,   /*!< SOCKS5 proxy */
} lr_ProxyType;

/* Some common used arrays for LRO_YUMDLIST */

/** Predefined value for LRO_YUMDLIST option - Download whole repo. */
#define LR_YUM_FULL         NULL

/** Predefined value for LRO_YUMDLIST option - Download only repomd.xml. */
#define LR_YUM_REPOMDONLY   [NULL]

/** Predefined value for LRO_YUMDLIST option - Download only base xml files. */
#define LR_YUM_BASEXML      ["primary", "filelists", "other", NULL]

/** Predefined value for LRO_YUMDLIST option - Download only base db files. */
#define LR_YUM_BASEDB       ["primary_db", "filelists_db", "other_db", NULL]

/** Predefined value for LRO_YUMDLIST option - Download only primary,
 * filelists and prestodelta.
 */
#define LR_YUM_HAWKEY       ["primary", "filelists", "prestodelta", NULL]

/** Progress callback prototype */
typedef int (*lr_ProgressCb)(void *clientp,
                              double total_to_download,
                              double now_downloaded);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
