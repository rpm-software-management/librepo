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

#ifndef __LR_HANDLE_H__
#define __LR_HANDLE_H__

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

/** LRO_FASTESTMIRRORMAXAGE default value */
#define LRO_FASTESTMIRRORMAXAGE_DEFAULT     2592000L // 30 days

/** LRO_FASTESTMIRRORMAXAGE minimal allowed value */
#define LRO_FASTESTMIRRORMAXAGE_MIN         0L

/** LRO_PROXYPORT default value */
#define LRO_PROXYPORT_DEFAULT               1080L

/** LRO_PROXYTYPE default value */
#define LRO_PROXYTYPE_DEFAULT               LR_PROXY_HTTP

/** LRO_MAXSPEED default value (0 == unlimited speed) */
#define LRO_MAXSPEED_DEFAULT                G_GINT64_CONSTANT(0)

/** LRO_CONNECTTIMEOUT default value */
#define LRO_CONNECTTIMEOUT_DEFAULT          120L

/** LRO_MAXMIRRORTRIES default value */
#define LRO_MAXMIRRORTRIES_DEFAULT          0L

/** LRO_MAXMIRRORTRIES minimal allowed value */
#define LRO_MAXMIRRORTRIES_MIN              0L

/** LRO_MAXPARALLELDOWNLOADS default value */
#define LRO_MAXPARALLELDOWNLOADS_DEFAULT    3L

/** LRO_MAXPARALLELDOWNLOADS minimal allowed value */
#define LRO_MAXPARALLELDOWNLOADS_MIN        1L

/** LRO_MAXPARALLELDOWNLOADS maximal allowed value */
#define LRO_MAXPARALLELDOWNLOADS_MAX        20L

/** LRO_MAXDOWNLOADSPERMIRROR default value */
#define LRO_MAXDOWNLOADSPERMIRROR_DEFAULT   3L

/** LRO_MAXDOWNLOADSPERMIRROR minimal allowed value */
#define LRO_MAXDOWNLOADSPERMIRROR_MIN       1L

/** LRO_LOWSPEEDTIME minimal allowed value */
#define LRO_LOWSPEEDTIME_MIN                0L

/** LRO_LOWSPEEDTIME default value */
#define LRO_LOWSPEEDTIME_DEFAULT            120L

/** LRO_LOWSPEEDLIMIT minimal allowed value */
#define LRO_LOWSPEEDLIMIT_MIN               0L

/** LRO_LOWSPEEDLIMIT default value */
#define LRO_LOWSPEEDLIMIT_DEFAULT           1000L

/** LRO_IPRESOLVE default value */
#define LRO_IPRESOLVE_DEFAULT               LR_IPRESOLVE_WHATEVER

/** LRO_ALLOWEDMIRRORFAILURES default value */
#define LRO_ALLOWEDMIRRORFAILURES_DEFAULT   4L

/** LRO_ADAPTIVEMIRRORSORTING default value */
#define LRO_ADAPTIVEMIRRORSORTING_DEFAULT   1L

/** LRO_GNUPGHOMEDIR default value */
#define LRO_GNUPGHOMEDIR_DEFAULT            NULL

/** LRO_FASTESTMIRRORTIMEOUT default value */
#define LRO_FASTESTMIRRORTIMEOUT_DEFAULT    2.0

/** LRO_OFFLINE default value */
#define LRO_OFFLINE_DEFAULT                 0L

/** LRO_HTTPAUTHMETHODS default value*/
#define LRO_HTTPAUTHMETHODS_DEFAULT         LR_AUTH_BASIC

/** LRO_PROXYAUTHMETHODS default value*/
#define LRO_PROXYAUTHMETHODS_DEFAULT        LR_AUTH_BASIC

/** LRO_FTPUSEEPSV default value */
#define LRO_FTPUSEEPSV_DEFAULT              1L


/** Handle options for the ::lr_handle_setopt function. */
typedef enum {

    LRO_UPDATE,  /*!< (long 1 or 0)
        Update existing repo in ::LrResult. Update means download missing
        (previously omitted) metadata file(s). */

    LRO_URLS,  /*!< (char ** NULL-terminated)
        List of base repo URLs */

    LRO_MIRRORLIST,  /*!< (char *)
        Mirrorlist or metalink url.
        This option is DEPRECATED!
        Use LRO_MIRRORLISTURL or LRO_METALINKURL instead. */

    LRO_MIRRORLISTURL, /*!< (char *)
        Mirrorlist url */

    LRO_METALINKURL, /*!< (char *)
        Metalink url */

    LRO_LOCAL,  /*!< (long 1 or 0)
        Do not duplicate local metadata, just locate the old one */

    LRO_HTTPAUTH,  /*!< (long 1 or 0)
        Enable all supported method of HTTP authentification.
        This option is DEPRECATED!
        Use LRO_HTTPAUTHMETHODS */

    LRO_USERPWD,  /*!< (char *)
        User and password for http authetification in format user:password */

    LRO_PROXY,  /*!< (char *)
        Address of proxy server eg. "proxy-host.com:8080" */

    LRO_PROXYPORT,  /*!< (long)
        Set port number for proxy separately. Default port is 1080. */

    LRO_PROXYTYPE,  /*!< (LrProxyType)
        Type of the proxy used. */

    LRO_PROXYAUTH,  /*!< (long 1 or 0)
        Enable all supported method for proxy authentification.
        This option is DEPRECATED!
        Use LRO_PROXYAUTHMETHODS */

    LRO_PROXYUSERPWD,  /*!< (char *)
        User and password for proxy in format user:password */

    LRO_PROGRESSCB,  /*!< (LrProgressCb)
        Progress callback */

    LRO_PROGRESSDATA,  /*!< (void *)
        Progress callback user data */

    LRO_MAXSPEED,  /*!< (gint64)
        Maximum download speed in bytes per second. Default is 0 = unlimited
        download speed. */

    LRO_DESTDIR,  /*!< (char *)
        Where to save downloaded files */

    LRO_REPOTYPE,  /*!< (LrRepotype)
        Type of downloaded repo, currently only supported is LR_YUMREPO. */

    LRO_CONNECTTIMEOUT,  /*!< (long)
        Max time in sec for connection phase. default timeout
        is 300 seconds. */

    LRO_IGNOREMISSING,  /*!< (long 1 or 0)
        If you want to localise (LRO_LOCAL is enabled) a incomplete local
        repository (eg. only primary and filelists are present) you could
        use LRO_YUMDLIST and specify only file that are present, or use this
        option. */

    LRO_INTERRUPTIBLE,  /*!< (long 1 or 0)
        If true, Librepo setups its own signal handler for SIGTERM and stops
        downloading if SIGTERM is catched. In this case current operation
        could return any kind of error code. Handle which operation was
        interrupted shoud never be used again! */

    LRO_USERAGENT,  /*!< (char *)
        String for  User-Agent: header in the http request sent to
        the remote server */

    LRO_FETCHMIRRORS,  /*!< (long 1 or 0)
        If true - do not download anything, except mirrorlist or metalink
        (during lr_handle_perform()).*/

    LRO_MAXMIRRORTRIES,  /*!< (long)
        If download fails try at most the specified number of mirrors.
        0 means try all available mirrors. */

    LRO_MAXPARALLELDOWNLOADS,  /*!< (long)
        Maximum number of parallel downloads. */

    LRO_MAXDOWNLOADSPERMIRROR,  /*!< (long)
        Maximum number of parallel downloads per mirror. */

    LRO_VARSUB,  /*!< (LrUrlVars *)
        Variables and its substitutions for repo URL.
        [{"releasever", "f18"}], ...;
        (e.g.: http://foo/$releasever => http://foo/f18)
        LrUrlVars has to be constructed by the ::lr_urlvars_set() function.
        After set the list to the handle, it has not to be freed! Handle
        itself takes care about freeing the list. */

    LRO_FASTESTMIRROR, /*!< (long 1 or 0)
        Sort the internal mirrorlist, after it is constructed, by the
        determined connection speed.
        Disabled by default. */

    LRO_FASTESTMIRRORCACHE, /*!< (char *)
        Path to the fastestmirror's cache file.
        Used when LRO_FASTESTMIRROR is enabled.
        If it doesn't exists, it will be created. */

    LRO_FASTESTMIRRORMAXAGE, /*< (long)
        Maximum age of a record in cache (seconds).
        Default: 2592000 (30 days). */

    LRO_FASTESTMIRRORCB, /* (LrFastestMirrorCb)
        Fastest mirror status callback */

    LRO_FASTESTMIRRORDATA, /* (void *)
        User data for LRO_FASTESTMIRRORCB */

    LRO_LOWSPEEDTIME, /*< (long)
        The time in seconds that the transfer should be below the
        LRO_LOWSPEEDLIMIT for the library to consider it too slow
        and abort. */

    LRO_LOWSPEEDLIMIT, /*< (long)
        The transfer speed in bytes per second that the transfer
        should be below during LRO_LOWSPEEDTIME seconds for
        the library to consider it too slow and abort. */

    /* Repo common options */

    LRO_GPGCHECK,   /*!< (long 1 or 0)
        Check GPG signature if available */

    LRO_CHECKSUM,  /*!< (long 1 or 0)
        Check files checksum if available */

    /* LR_YUMREPO specific options */

    LRO_YUMDLIST,  /*!< (char ** NULL-terminated)
        Download only specified records from repomd (e.g. ["primary",
        "filelists", NULL]).
        Note: Last element of the list must be NULL! */

    LRO_RPMMDDLIST = LRO_YUMDLIST,

    LRO_YUMBLIST,  /*!< (char ** NULL-terminated)
        Do not download this specified records from repomd (blacklist).
        Note: Last element of the list must be NULL! */

    LRO_RPMMDBLIST = LRO_YUMBLIST,

    LRO_HMFCB, /*!< (LrHandleMirrorFailureCb)
        Handle specific mirror failure callaback.
        Callback called when a repodata download from a mirror fails.
        This callback gets the user data setted by LRO_PROGRESSDATA */

    LRO_SSLVERIFYPEER, /*!< (long 1 or 0)
        This option determines whether librepo verifies the authenticity
        of the peer's certificate.
        This trust is based on a chain of digital signatures,
        rooted in certification authority (CA) certificates. */

    LRO_SSLVERIFYHOST, /*!< (long 1 or 0)
        This option determines whether librepo verifies that
        the server cert is for the server it is known as. */

    LRO_IPRESOLVE, /*!< (LrIpResolveType)
        Sets what kind of IP addresses to use when resolving host names. */

    LRO_ALLOWEDMIRRORFAILURES, /*!< (long)
        If all transfers from a mirror failed (no successful transfer
        from the mirror exists) and the number
        of failed downloads is higher or equal to this value
        the mirror will be skipped (ignored) for all next downloads.
        Note: Number of failed transfers for a single mirror can
        outreach this number! For example, if you set this value to 1
        but you allow 3 parallel downloads it is possible that all
        three downloads start from the mirror,
        before any of them can fail. Then, if all three transfers
        fail, the number of failures for the mirror
        will be 3, even if this option was set to 1.
        Set -1 or 0 to disable this option */

    LRO_ADAPTIVEMIRRORSORTING, /*!< (long 1 or 0)
        If enabled, internal list of mirrors for each handle is
        re-sorted after each finished transfer.
        The the sorting is based on mirror
        error rate etc. */

    LRO_GNUPGHOMEDIR, /*!< (char *)
        Configuration directory for GNUPG (a directory with keyring) */

    LRO_FASTESTMIRRORTIMEOUT, /*!< (double)
        Max length of fastest mirror measurement in seconds.
        Default value is 2sec */

    LRO_HTTPHEADER, /*!< (char ** NULL-terminated)
        List of HTTP header to pass to the server and/or proxy in
        Librepo's HTTP requests. */

    LRO_OFFLINE, /*!< (long 1 or 0)
        Make the handle work only locally, all remote URLs are ignored.
        Remote mirrorlists/metalinks (if they are specified) are ignored.
        Fastest mirror check (if enabled) is skiped. */

    LRO_SSLCLIENTCERT, /*!< (char *)
        Path to the PEM format SSL client certificate librepo should use
        when talking to the server. */

    LRO_SSLCLIENTKEY, /*!< (char *)
        Path to the PEM format SSL client key librepo should use when
        talking to the server, if not included in the client certificate
        file. */

    LRO_SSLCACERT, /*!< (char *)
        Path to a file containing the list of PEM format trusted CA
        certificates. */

    LRO_HTTPAUTHMETHODS, /*!< (LrAuth)
        Bitmask which tell Librepo which auth metods you wan to use. */

    LRO_PROXYAUTHMETHODS, /*!< (LrAuth)
        A long bitmask which tell Librepo which auth methods you want
        to use for proxy auth. */

    LRO_FTPUSEEPSV, /*!< (long 1 or 0)
        Enable/Disable EPSV (Extended Passive mode) for FTP. */

    LRO_SENTINEL,    /*!< Sentinel */

} LrHandleOption; /*!< Handle config options */

/** Handle options for the ::lr_handle_getinfo function. */
typedef enum {
    LRI_UPDATE,                 /*!< (long *) */
    LRI_URLS,                   /*!< (char *** Malloced)
        NOTE: Returned list must be freed as well as all its items!
        You could use g_strfreev() function. */
    LRI_MIRRORLIST,             /*!< (char **) */
    LRI_MIRRORLISTURL,          /*!< (char **) */
    LRI_METALINKURL,            /*!< (char **) */
    LRI_LOCAL,                  /*!< (long *) */
    LRI_PROGRESSCB,             /*!< (void *) */
    LRI_PROGRESSDATA,           /*!< (LrProgressCb) */
    LRI_DESTDIR,                /*!< (char **) */
    LRI_REPOTYPE,               /*!< (long *) */
    LRI_USERAGENT,              /*!< (char **) */
    LRI_YUMDLIST,               /*!< (char *** Malloced)
        NOTE: Returned list must be freed as well as all its items!
        You could use g_strfreev() function. */
    LRI_RPMMDDLIST = LRI_YUMDLIST,
    LRI_YUMBLIST,               /*!< (char *** Malloced)
        NOTE: Returned list must be freed as well as all its items!
        You could use g_strfreev() function. */
    LRI_RPMMDBLIST = LRI_YUMBLIST,
    LRI_FETCHMIRRORS,           /*!< (long *) */
    LRI_MAXMIRRORTRIES,         /*!< (long *) */
    LRI_VARSUB,                 /*!< (LrUrlVars **) */
    LRI_MIRRORS,                /*!< (char *** Malloced)
        Mirrorlist associated with the repository.

        Mirrors on this list are mirrors parsed from
        mirrorlist/metalink specified by LRO_MIRRORLIST or
        from mirrorlist specified by LRO_MIRROSLISTURL and
        metalink specified by LRO_METALINKURL.

        No URLs specified by LRO_URLS are included in this list.

        NOTE: Returned list must be freed as well as all its items!
        You could use g_strfreev() function. */
    LRI_METALINK,               /*!< (LrMetalink *) */
    LRI_FASTESTMIRROR,          /*!< (long *) */
    LRI_FASTESTMIRRORCACHE,     /*!< (char **) */
    LRI_FASTESTMIRRORMAXAGE,    /*!< (long *) */
    LRI_HMFCB,                  /*!< (LrHandleMirrorFailureCb) */
    LRI_SSLVERIFYPEER,          /*!< (long *) */
    LRI_SSLVERIFYHOST,          /*!< (long *) */
    LRI_IPRESOLVE,              /*!< (LrIpResolveType *) */
    LRI_ALLOWEDMIRRORFAILURES,  /*!< (long *) */
    LRI_ADAPTIVEMIRRORSORTING,  /*!< (long *) */
    LRI_GNUPGHOMEDIR,           /*!< (char **) */
    LRI_FASTESTMIRRORTIMEOUT,   /*!< (double *) */
    LRI_HTTPHEADER,             /*!< (char *** Malloced)
        NOTE: Returned list must be freed as well as all its items!
        You could use g_strfreev() function. */
    LRI_OFFLINE,                /*!< (long *) */
    LRI_SSLCLIENTCERT,          /*!< (char **) */
    LRI_SSLCLIENTKEY,           /*!< (char **) */
    LRI_SSLCACERT,              /*!< (char **) */
    LRI_LOWSPEEDTIME,           /*!< (long) */
    LRI_LOWSPEEDLIMIT,          /*!< (long) */
    LRI_HTTPAUTHMETHODS,        /*!< (LrAuth) */
    LRI_PROXYAUTHMETHODS,       /*!< (LrAuth) */
    LRI_FTPUSEEPSV,             /*!< (long) */
    LRI_SENTINEL,
} LrHandleInfoOption; /*!< Handle info options */

/** Return new handle.
 * @return              New allocated handle.
 */
LrHandle *
lr_handle_init();

/** Frees handle and its content.
 * @param handle        Handle.
 */
void
lr_handle_free(LrHandle *handle);

/** Set option (::LrHandleOption) of the handle.
 * @param handle        Handle.
 * @param err           GError **
 * @param option        Option from ::LrHandleOption enum.
 * @param ...           Value for the option.
 * @return              TRUE if everything is ok, FALSE if err is set.
 */
gboolean
lr_handle_setopt(LrHandle *handle,
                 GError **err,
                 LrHandleOption option,
                 ...);

/** Get information from handle.
 * Most of returned pointers point directly to the handle internal
 * values and therefore you should assume that they are only valid until
 * any manipulation (lr_handle_setopt, lr_handle_perform, ...)
 * with handle occurs.
 * NOTE: You should not free or modify the memory returned by this
 * function unless it is explicitly mentioned!
 * @param handle        Librepo handle.
 * @param err           GError **
 * @param option        Option from ::LrHandleInfoOption enum.
 * @param ...           Appropriate variable for the selected option.
 * @return              TRUE if everything is ok, FALSE if err is set.
 */
gboolean
lr_handle_getinfo(LrHandle *handle,
                  GError **err,
                  LrHandleInfoOption option,
                  ...);

/** Perform repodata download or location.
 * @param handle        Librepo handle.
 * @param result        Librepo result.
 * @param err           GError **
 * @return              TRUE if everything is ok, FALSE if err is set.
 */
gboolean
lr_handle_perform(LrHandle *handle, LrResult *result, GError **err);

/** @} */

G_END_DECLS

#endif
