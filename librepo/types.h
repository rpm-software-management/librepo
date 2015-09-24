/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2012  Tomas Mlcoch
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __LR_TYPES_H__
#define __LR_TYPES_H__

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
    LR_RPMMDREPO        = LR_YUMREPO,
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

/** IpResolve types */
typedef enum {
    LR_IPRESOLVE_WHATEVER,  /*!< Default - resolves addresses to all IP versions */
    LR_IPRESOLVE_V4,        /*!< Resolve to IPv4 addresses */
    LR_IPRESOLVE_V6,        /*!< Resolve to IPv6 addresses */
} LrIpResolveType;

/** LrAuth methods */
typedef enum {
    LR_AUTH_NONE        = 0,       /*!< None auth method */
    LR_AUTH_BASIC       = (1<<0),  /*!< HTTP Basic authentication (Default) */
    LR_AUTH_DIGEST      = (1<<1),  /*!< HTTP Digest authentication */
    LR_AUTH_NEGOTIATE   = (1<<2),  /*!< HTTP Negotiate (SPNEGO) authentication */
    LR_AUTH_NTLM        = (1<<3),  /*!< HTTP NTLM authentication */
    LR_AUTH_DIGEST_IE   = (1<<4),  /*!< HTTP Digest authentication with an IE flavor */
    LR_AUTH_NTLM_WB     = (1<<5),  /*!< NTLM delegating to winbind helper */
    LR_AUTH_ONLY        = (1<<31), /*!< This is a meta symbol. OR this value
                                        together with a single specific auth
                                        value to force libcurl to probe for
                                        un-restricted auth and if not, only
                                        that single auth algorithm is
                                        acceptable.  */
    LR_AUTH_ANY         = (~LR_AUTH_DIGEST_IE), /*!< All suitable methods */
} LrAuth;

/* Some common used arrays for LRO_YUMDLIST */

/** Predefined value for LRO_YUMDLIST option - Download whole repo. */
#define LR_YUM_FULL         NULL
#define LR_RPMMD_FULL       NULL

/** Predefined value for LRO_YUMDLIST option - Download only repomd.xml. */
#define LR_YUM_REPOMDONLY   {NULL}
#define LR_RPMMD_REPOMDONLY {NULL}

/** Predefined value for LRO_YUMDLIST option - Download only base xml files. */
#define LR_YUM_BASEXML      {"primary", "filelists", "other", NULL}
#define LR_RPMMD_BASEXML    {"primary", "filelists", "other", NULL}

/** Predefined value for LRO_YUMDLIST option - Download only base db files. */
#define LR_YUM_BASEDB       {"primary_db", "filelists_db", "other_db", NULL}
#define LR_RPMMD_BASEDB     {"primary_db", "filelists_db", "other_db", NULL}

/** Predefined value for LRO_YUMDLIST option - Download only primary,
 * filelists and prestodelta.
 */
#define LR_YUM_HAWKEY       {"primary", "filelists", "prestodelta", NULL}
#define LR_RPMMD_HAWKEY     {"primary", "filelists", "prestodelta", NULL}

typedef enum LrCbReturnCode_e {
    LR_CB_OK = 0,   /*!< All fine */
    LR_CB_ABORT,    /*!< Abort the transfer - if no failfast is set,
                         then, it just abort the current download */
    LR_CB_ERROR,    /*!< Error - Fatal error, abort all downloading
                         and return from the download function (e.g.
                         lr_download_packages, ...) */
} LrCbReturnCode;

/** Progress callback prototype
 * @param clientp           Pointer to user data.
 * @param total_to_download Total number of bytes to download
 * @param now_downloaded    Number of bytes currently downloaded
 * @return                  See LrCbReturnCode codes
 */
typedef int (*LrProgressCb)(void *clientp,
                            double total_to_download,
                            double now_downloaded);

/** Transfer status codes */
typedef enum {
    LR_TRANSFER_SUCCESSFUL,
    LR_TRANSFER_ALREADYEXISTS,
    LR_TRANSFER_ALREDYEXISTS = LR_TRANSFER_ALREADYEXISTS, // Deprecated - Backward comp. - remove in future release
    LR_TRANSFER_ERROR,
} LrTransferStatus;

/** Called when a transfer is done (use transfer status to check
 * if successful or failed).
 * @param clientp           Pointer to user data.
 * @param status            Transfer status
 * @param msg               Error message or NULL.
 * @return                  See LrCbReturnCode codes
 */
typedef int (*LrEndCb)(void *clientp,
                       LrTransferStatus status,
                       const char *msg);

/** MirrorFailure callback prototype
 * @param clientp           Pointer to user data.
 * @param msg               Error message.
 * @param url               Mirror URL
 * @return                  See LrCbReturnCode codes
 */
typedef int (*LrMirrorFailureCb)(void *clientp,
                                 const char *msg,
                                 const char *url);

/** MirrorFailure callback
 * @param clientp           Pointer to user data.
 * @param msg               Error message.
 * @param url               Mirror URL
 * @param metadata          Metadata type "primary", etc.
 * @return                  See LrCbReturnCode codes
 */
typedef int (*LrHandleMirrorFailureCb)(void *clientp,
                                       const char *msg,
                                       const char *url,
                                       const char *metadata);

typedef enum {
    LR_FMSTAGE_INIT, /*!<
        Fastest mirror detection just started.
        ptr is NULL*/

    LR_FMSTAGE_CACHELOADING, /*!<
        ptr is (char *) pointer to string with path to the cache file.
        (Do not modify or free the string). */

    LR_FMSTAGE_CACHELOADINGSTATUS, /*!<
        if cache was loaded successfully, ptr is NULL, otherwise
        ptr is (char *) string with error message.
        (Do not modify or free the string) */

    LR_FMSTAGE_DETECTION, /*!<
        Detection (pinging) in progress.
        If all data was loaded from cache, this stage is skiped.
        ptr is pointer to long. This is the number of how much
        mirrors have to be "pinged" */

    LR_FMSTAGE_FINISHING, /*!<
        Detection is done, sorting mirrors, updating cache, etc.
        ptr is NULL */

    LR_FMSTAGE_STATUS, /*!<
        The very last invocation of fastest mirror callback.
        If fastest mirror detection was successful ptr is NULL,
        otherwise ptr contain (char *) string with error message.
        (Do not modify or free the string) */
} LrFastestMirrorStages;

/** Fastest mirror status callback
 * @param clientp   Pointer to user data.
 * @param stage     Stage of fastest mirror detection.
 * @param ptr       Value specific for each stage of detection.
 */
typedef void (*LrFastestMirrorCb)(void *clientp,
                                  LrFastestMirrorStages stage,
                                  void *ptr);

/** @} */

G_END_DECLS

#endif
