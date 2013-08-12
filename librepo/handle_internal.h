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

    char **baseurls; /*!<
        Base URL of repo */

    LrInternalMirrorlist *internal_mirrorlist; /*!<
        Internal list of mirrors (Real used mirrorlist = baseurl + mirrors) */

    // Mirrorlist related stuff

    char *mirrorlist; /*!<
        Mirrorlist or metalink URL */

    LrMetalink *metalink; /*!<
        Parsed metalink for repomd.xml */

    LrInternalMirrorlist *mirrors;  /*!<
        Mirrors from metalink or mirrorlist */

    int mirrorlist_fd;  /*!<
        Raw downloaded file */

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
        Which check sould be applied */

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
};

/** Return new CURL easy handle with some default options setted.
 */
CURL *
lr_get_curl_handle();

/**
 * Create (if do not exists) internal mirrorlist. Insert baseurl (if
 * specified) and download, parse and insert mirrors from mirrorlist url.
 * @param handle            Librepo handle.
 * @param err               GError **
 * @return                  returns TRUE if error is not set and FALSE if it is.
 */
gboolean
lr_handle_prepare_internal_mirrorlist(LrHandle *handle, GError **err);


G_END_DECLS

#endif
