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

#include <glib.h>

#include "result.h"

G_BEGIN_DECLS

/** \defgroup   handle    Librepo Handle
 *  \addtogroup handle
 *  @{
 */

/** Handle object containing configration for repository metadata and
 * package downloading.
 */
typedef struct _LrHandle LrHandle;

/** Handle options for the ::lr_handle_setopt function. */
typedef enum {
    LRO_UPDATE,      /*!< (long 1 or 0) Update existing repo in ::LrResult.
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
    LRO_PROXYTYPE,   /*!< (::LrProxyType) Type of the proxy used. */
    LRO_PROXYAUTH,   /*!< (long 1 or 0) Enable all supported method for proxy
                          authentification */
    LRO_PROXYUSERPWD,/*!< (char *) User and password for proxy in format
                          user:password */
    LRO_PROGRESSCB,  /*!< (::LrProgressCb) Progress callback */
    LRO_PROGRESSDATA,/*!< (void *) Progress callback user data */
    LRO_RETRIES,     /*!< (long) Number of maximum retries for each file - TODO */
    LRO_MAXSPEED,    /*!< (unsigned long long) Maximum download speed
                          in bytes per second. Default is 0 = unlimited
                          download speed. */
    LRO_DESTDIR,     /*!< (char *) Where to save downloaded files */

    LRO_REPOTYPE,    /*!< (::LrRepotype) Type of downloaded repo, currently
                          only supported is LR_YUMREPO. */
    LRO_CONNECTTIMEOUT,/*!< (long) Max time in sec for connection phase.
                            default timeout is 300 seconds. */
    LRO_IGNOREMISSING, /*!< (long) If you want to localise (LRO_LOCAL is enabled)
                            a incomplete local repository (eg. only primary
                            and filelists are present) you could use
                            LRO_YUMDLIST and specify only file that are
                            present, or use this option. */
    LRO_INTERRUPTIBLE, /*!< (long 1 or 0) If true, Librepo setups
                            its own signal handler for SIGTERM and
                            stops downloading if SIGTERM is catched.
                            In this case current operation could return any
                            kind of error code.
                            Handle which operation was interrupted
                            shoud never be used again! */
    LRO_USERAGENT,     /*!< (char *) String for  User-Agent: header in the
                            http request sent to the remote server */
    LRO_FETCHMIRRORS,  /*!< (long 1 or 0) If true - do not download anything,
                            except mirrorlist or metalink
                            (during lr_handle_perform()).*/
    LRO_MAXMIRRORTRIES,/*!< (long) If download fails try at most the specified
                            number of mirrors. 0 means try all available
                            mirrors. */
    LRO_VARSUB,        /*!< (LrUrlVars *) Variables and its substitutions for
                            repo URL. [{"releasever", "f18"}], ...;
                            (e.g.: http://foo/$releasever => http://foo/f18)
                            LrUrlVars has to be constructed by
                            lr_urlvars_set() function. After set the list
                            to the handle, it has not to be freed! Handle
                            itself takes care about freeing the list. */

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
} LrHandleOption; /*!< Handle config options */

/** Handle options for the ::lr_handle_getinfo function. */
typedef enum {
    LRI_UPDATE,                 /*!< (long *) */
    LRI_URL,                    /*!< (char **) */
    LRI_MIRRORLIST,             /*!< (char **) */
    LRI_LOCAL,                  /*!< (long *) */
    LRI_PROGRESSCB,             /*!< (void *) */
    LRI_PROGRESSDATA,           /*!< (LrProgressCb) */
    LRI_DESTDIR,                /*!< (char **) */
    LRI_REPOTYPE,               /*!< (long *) */
    LRI_USERAGENT,              /*!< (char **) */
    LRI_YUMDLIST,               /*!< (char ***)
        NOTE: Returned list must be freed as well as all its items!
        You could use g_strfreev() function. */
    LRI_YUMBLIST,               /*!< (char ***)
        NOTE: Returned list must be freed as well as all its items!
        You could use g_strfreev() function. */
    LRI_FETCHMIRRORS,           /*!< (long *) */
    LRI_MAXMIRRORTRIES,         /*!< (long *) */
    LRI_VARSUB,                 /*!< (LrUrlVars **) */
    LRI_MIRRORS,                /*!< (char ***)
        Mirrorlist associated with the repository.

        If LRO_MIRRORLIST was specified, then content of this list is
        created from the specified mirrorlist.
        If no LRO_MIRRORLIST was specified and repository is on a local
        filesystem and contains a mirrorlist then the mirrorlist is
        automatically loaded.

        Mirrors from this list are used for downloading only if the
        mirrorlist was specified by LRO_MIRRORLIST option! Automatically
        loaded mirrorlist from a local repository is not implicitly
        used for downloading!

        NOTE: Returned list must be freed as well as all its items!
        You could use g_strfreev() function. */
    LRI_METALINK,               /*!< (LrMetalink *) */
    LRI_SENTINEL,
} LrHandleInfoOption; /*!< Handle info options */

/** Return new handle.
 * @return              New allocated handle.
 */
LrHandle *lr_handle_init();

/** Frees handle and its content.
 * @param handle        Handle.
 */
void lr_handle_free(LrHandle *handle);

/** Set option (::LrHandleOption) of the handle.
 * @param handle         Handle.
 * @param option        Option from ::LrHandleOption enum.
 * @param ...           Value for the option.
 * @return              Librepo return code from ::LrRc enum.
 */
int lr_handle_setopt(LrHandle *handle, LrHandleOption option, ...);

/** Get information from handle.
 * Most of returned pointers point directly to the handle internal
 * values and therefore you should assume that they are only valid until
 * any manipulation (lr_handle_setopt, lr_handle_perform, ...)
 * with handle occurs.
 * NOTE: You should not free or modify the memory returned by this
 * function unless it is explicitly mentioned!
 * @param handle        Librepo handle.
 * @param option        Option from ::LrHandleInfoOption enum.
 * @param ...           Apropriate variable fro the selected option.
 * @return              Librepo return code ::LrRc.
 */
int lr_handle_getinfo(LrHandle *handle, LrHandleOption option, ...);

/** Perform repodata download or location.
 * @param handle        Librepo handle.
 * @param result        Librepo result.
 * @param err           GError **
 * @return              Librepo return code from ::LrRc enum.
 */
int lr_handle_perform(LrHandle *handle, LrResult *result, GError **err);

/** @} */

G_END_DECLS

#endif
