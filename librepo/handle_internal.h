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

#ifndef __LR_HANDLE_INTERNAL_H__
#define __LR_HANDLE_INTERNAL_H__

#include <glib.h>
#include <curl/curl.h>

#include "types.h"
#include "handle.h"
#include "lrmirrorlist.h"
#include "url_substitution.h"

G_BEGIN_DECLS

#define TMP_DIR_TEMPLATE    "librepo-XXXXXX"

struct _LrHandle {

    CURL *curl_handle; /*!<
        CURL handle */

    int update; /*!<
        Just update existing repo */

    LrInternalMirrorlist *internal_mirrorlist; /*!<
        Internal list of mirrors (Real used mirrorlist = baseurl + mirrors) */

    char **urls; /*!<
        URLs of repositories */

    LrInternalMirrorlist *urls_mirrors; /*!<
        Mirrors from urls */

    int fastestmirror; /*!<
        Should be internal mirrorlist sorted by connection time */

    char *fastestmirrorcache; /*!<
        Path to the fastestmirror's cache file. */

    long fastestmirrormaxage; /*!<
        Maximum age of a record in cache (seconds). */

    LrFastestMirrorCb fastestmirrorcb; /*!<
        Fastest mirror detection status callback */

    void *fastestmirrordata; /*!<
        User data for fastestmirrorcb. */

    // Mirrorlist related stuff

    char *mirrorlist; /*!<
        XXX: Deprecated!
        List of or metalink */

    char *mirrorlisturl; /*!<
        Mirrorlist URL */

    int mirrorlist_fd; /*!<
        Raw downloaded mirrorlist file */

    LrInternalMirrorlist *mirrorlist_mirrors; /*!<
        Mirrors from mirrorlist */

    // Metalink related stuff

    char * metalinkurl; /*!<
        Metalink URL */

    int metalink_fd; /*!<
        Raw downloaded metalink file */

    LrInternalMirrorlist *metalink_mirrors; /*!<
        Mirrors from metalink */

    LrMetalink *metalink; /*!<
        Parsed metalink for repomd.xml */

    LrInternalMirrorlist *mirrors;  /*!<
        Mirrors from metalink or mirrorlist */

    int local; /*!<
        Do not duplicate local data */

    char *used_mirror; /*!<
        Finally used mirror (if any) */

    char *destdir; /*!<
        Destination directory */

    char *useragent; /*!<
        User agent */

    LrRepotype repotype; /*!<
        Type of repository */

    LrChecks checks; /*!<
        Which check should be applied */

    LrProgressCb user_cb; /*!<
        User progress callback */

    void *user_data; /*!<
        User data for callback */

    int ignoremissing; /*!<
        Ignore missing metadata files */

    int interruptible; /*!<
        Setup own SIGTERM handler*/

    char **yumdlist; /*!<
        Repomd data typenames to download NULL - Download all
        yumdlist[0] = NULL - Only repomd.xml */

    char **yumblist; /*!<
        Repomd data typenames to skip (blacklist). NULL as argument will
        disable blacklist. */

    int fetchmirrors;   /*!<
        Only fetch and parse mirrorlist. */

    int maxmirrortries; /*!<
        Try at most this number of mirrors. */

    long maxparalleldownloads; /* !<
        Maximum number of parallel downloads. */

    long maxdownloadspermirror; /* !<
        Maximum number of parallel downloads per a single mirror. */

    LrUrlVars *urlvars; /*!<
        List with url substitutions */

    long lowspeedtime; /*!<
        The time in seconds that the transfer should be below the
        LRO_LOWSPEEDLIMIT for the library to consider it too slow
        and abort. */

    long lowspeedlimit; /*!<
        The transfer speed in bytes per second that the transfer
        should be below during LRO_LOWSPEEDTIME seconds for
        the library to consider it too slow and abort. */

    LrHandleMirrorFailureCb hmfcb; /*!<
        Callaback called when a repodata download from a mirror fails. */

    gint64 maxspeed; /*!<
        Max speed in bytes per sec */

    long sslverifypeer; /*!<
        Determines whether verify the autenticity of the peer's certificate */

    long sslverifyhost; /*!<
        Determines whether the server name should be checked against the name
        in the certificate */

    char *sslclientcert; /*!<
        Client certificate filename. */

    char *sslclientkey; /*!<
        Client certificate key. */

    char *sslcacert; /*!<
        CA certificate path. */

    LrIpResolveType ipresolve; /*!<
        What kind of IP addresses to use when resolving host names. */

    long allowed_mirror_failures; /*!<
        Number of allowed failed transfers, when there are no
        successful ones, before a mirror gets ignored. */

    long adaptivemirrorsorting; /*!<
        See: LRO_ADAPTIVEMIRRORSORTING */

    gchar *gnupghomedir; /*!<
        GNUPG home dir. */

    gdouble fastestmirrortimeout; /*!<
        Max length of fastest mirror measurement in seconds. */

    gchar **httpheader; /*!<
        List of HTTP headers.
        Curl doesn't copy HTTP header values from curl_slist.
        We need to keep them around. */

    gboolean offline; /*!<
        If TRUE, librepo should work offline - ignore all
        non local URLs, etc. */

    LrAuth httpauthmethods; /*!<
        Bitmask with auth methods */

    LrAuth proxyauthmethods; /*!<
        Bitmask with auth methods */

    long ftpuseepsv; /*!<
        Use FTP EPSV (extended passive mode) mode */
};

/** Return new CURL easy handle with some default options setted.
 */
CURL *
lr_get_curl_handle();

/**
 * Create (if do not exists) internal mirrorlist. Insert baseurl (if
 * specified) and download, parse and insert mirrors from mirrorlist url.
 * @param handle            Librepo handle.
 * @param usefastestmirror  Sort internal mirrorlist by speed of mirrors
 * @param err               GError **
 * @return                  returns TRUE if error is not set and FALSE if it is.
 */
gboolean
lr_handle_prepare_internal_mirrorlist(LrHandle *handle,
                                      gboolean usefastestmirror,
                                      GError **err);


G_END_DECLS

#endif
