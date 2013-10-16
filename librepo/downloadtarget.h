/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2013  Tomas Mlcoch
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

#ifndef LR_DOWNLOADTARGET_H
#define LR_DOWNLOADTARGET_H

#include <glib.h>

#include "handle.h"
#include "rcodes.h"
#include "checksum.h"
#include "types.h"

G_BEGIN_DECLS

typedef struct {
    LrChecksumType type;
    gchar *value;
} LrDownloadTargetChecksum;

/** Create new LrDownloadTargetChecksum object.
 * @param type      Checksum type
 * @param value     Checksum value. This value will be stduped.
 */
LrDownloadTargetChecksum *
lr_downloadtargetchecksum_new(LrChecksumType type, const gchar *value);

/** Free LrDownloadTargetChecksum object.
 * @param dtch      LrDownloadTargetChecksum object
 */
void
lr_downloadtargetchecksum_free(LrDownloadTargetChecksum *dtch);

/** Single download target
 */
typedef struct {

    LrHandle *handle; /*!<
        Handle */

    char *path; /*!<
        Relative path for URL (URL: "http://foo.bar/stuff",
        path: "somestuff.xml") */

    char *baseurl; /*!<
        Base URL for this target. If used, then mirrorlist will be ignored. */

    int fd; /*!<
        Opened file descriptor where data will be written or -1.
        Note: Only one, fd or fn, is set simultaneously. */

    char *fn; /*!<
        Filename where data will be written or NULL.
        Note: Only one, fd or fn, is set simultaneously. */

    GSList *checksums; /*!<
        NULL or GSList with pointers to LrDownloadTargetChecksum
        structures. With possible checksums of the file.
        Checksum check is stoped right after first match.
        Useful in situation when the file could has one from set
        of available checksums.
        E.g. <mm0:alternates> element in metalink could contain alternate
        checksums for the repomd.xml, because metalink and mirrors
        could be out of sync for a while. */

    gint64 expectedsize; /*!<
        Expected size of the target */

    gboolean resume; /*!<
        Resume:
         0  - no resume, download whole file,
         otherwise - autodetect offset for resume */

    LrProgressCb progresscb; /*!<
        Progression callback for the target */

    void *cbdata; /*!<
        Callback user data */

    LrEndCb endcb; /*!<
        Callback called when target transfer is done.
        (Use status to check if successfully or unsuccessfully) */

    LrMirrorFailureCb mirrorfailurecb; /*!<
        Callen when download from a mirror failed. */

    GStringChunk *chunk; /*!<
        Chunk for strings used in this structure. */

    // Items filled by downloader

    char *usedmirror; /*!<
        Used mirror. Filled only if transfer was successfull. */

    char *effectiveurl; /*!<
        Effective url. Could be only concatenation of used_mirror and
        path or completely different url (if there were some
        redirects, etc.). Filled only if transfer was successfull. */

    LrRc rcode; /*!<
        Return code */

    char *err; /*!<
        NULL or error message */

    // Other items

    void *userdata; /*!<
        User data - This data are not used by lr_downloader or touched
        by lr_downloadtarget_free. */

} LrDownloadTarget;

/** Create new empty ::LrDownloadTarget.
 * @param handle            Handle
 * @param path              Absolute or relative URL path
 * @param baseurl           Base URL for relative path specified in path param
 * @param fd                Opened file descriptor where data will be written
 *                          or -1.
 *                          Note: Set this or fn, no both!
 * @param fn                Filename where data will be written or NULL.
 *                          Note: Set this or fd, no both!
 * @param possiblechecksums NULL or GSList with pointers to
 *                          LrDownloadTargetChecksum structures. With possible
 *                          checksums of the file. Checksum check is stoped
 *                          right after first match. Useful in situation when
 *                          the file could has one from set of available
 *                          checksums.
 *                          E.g. <mm0:alternates> element in metalink could
 *                          contain alternate checksums for the repomd.xml,
 *                          because metalink and mirrors could be out of
 *                          sync for a while.
 *                          Note: LrDownloadTarget takes responsibility
 *                          for destroing this list! So the list must not
 *                          be destroyed by the user, since it is passed
 *                          to this function.
 * @param expectedsize      Expected size of the target. If mirror reports
 *                          different size, then no download is performed.
 *                          If 0 then size of downloaded target is not checked.
 * @param resume            If FALSE, then no resume is done and whole file is
 *                          downloaded again. Otherwise offset will be
 *                          automaticaly detected from opened file descriptor
 *                          by seek to the end.
 * @param progresscb        Progression callback or NULL
 * @param cbdata            Callback data or NULL
 * @param endcb             Callback called when target transfer is done.
 *                          (Use status to check if successfully
 *                          or unsuccessfully)
 * @param mirrorfailurecb   Called when download from a mirror failed.
 * @param userdata          This variable could be used to store some user
 *                          data. This data are not used/touched by
 *                          lr_download* functions or by
 *                          lr_downloadtarget_free().
 * @return                  New allocated target
 */
LrDownloadTarget *
lr_downloadtarget_new(LrHandle *handle,
                      const char *path,
                      const char *baseurl,
                      int fd,
                      const char *fn,
                      GSList *possiblechecksums,
                      gint64 expectedsize,
                      gboolean resume,
                      LrProgressCb progresscb,
                      void *cbdata,
                      LrEndCb endcb,
                      LrMirrorFailureCb mirrorfailurecb,
                      void *userdata);

/** Free a ::LrDownloadTarget element and its content.
 * @param target        Target to free.
 */
void
lr_downloadtarget_free(LrDownloadTarget *target);

G_END_DECLS

#endif
