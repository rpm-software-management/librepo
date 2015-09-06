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

#ifndef __LR_PACKAGE_DOWNLOADER_H__
#define __LR_PACKAGE_DOWNLOADER_H__

#include <glib.h>

#include "rcodes.h"
#include "handle.h"
#include "checksum.h"

G_BEGIN_DECLS

/** \defgroup   package_downloader    Package downloading
 *  \addtogroup package_downloader
 *  @{
 */

/** Download package from repository.
 * @param handle            Librepo handle.
 * @param relative_url      Relative part of url.
 * @param err               GError **
 * @return                  See ::lr_download_package
 */
#define lr_download_simple(handle, relative_url, err) \
                    lr_download_package((handle), (relative_url), NULL, 0, \
                                        NULL, 0, NULL, 0, (err))

/** Download package from repository or base_url.
 * Note: If resume, checksum and checksum_type are specified and
 * the downloaded package alredy exists and checksum matches, then
 * no downloading is done and LRE_ALREADYDOWNLOADED return code is returned.
 * @param handle            Librepo handle.
 * @param relative_url      Relative part of url.
 * @param dest              Destination file, directory
 *                          or NULL (current working dir is used).
 * @param checksum_type     Type of checksum.
 * @param checksum          Checksum value or NULL.
 * @param expectedsize      Expected size of target. If >0 and server reports
 *                          different size, then no download is performed.
 * @param base_url          If specified, mirrors from handle are ignored
 *                          and this base_url is used for downloading.
 * @param resume            If TRUE try to resume downloading if dest file
 *                          already exists.
 * @param err               GError **
 * @return                  TRUE if everything is ok, FALSE if err is set.
 */
gboolean
lr_download_package(LrHandle *handle,
                    const char *relative_url,
                    const char *dest,
                    LrChecksumType checksum_type,
                    const char *checksum,
                    gint64 expectedsize,
                    const char *base_url,
                    gboolean resume,
                    GError **err);

/** LrPackageTarget structure */
typedef struct {

    LrHandle *handle; /*!<
        Related handle */

    char *relative_url; /*!<
        Relative part of URL */

    char *dest; /*!<
        Destination: filename, dirname or NULL */

    char *base_url; /*!<
        Base URL for this target */

    LrChecksumType checksum_type; /*!<
        Checksum type */

    char *checksum; /*!<
        Expected checksum value */

    gint64 expectedsize; /*!<
        Expected size of the target */

    gboolean resume; /*!<
        Indicate if resume is enabled */

    LrProgressCb progresscb; /*!<
        Progress callback */

    void *cbdata; /*!<
        Callback data */

    LrEndCb endcb; /*!<
        Callback called when target transfer is done.
        (Use status to check if successfully or unsuccessfully) */

    LrMirrorFailureCb mirrorfailurecb; /*!<
        Callen when download from a mirror failed. */

    gint64 byterangestart; /*!<
        Download only specified range of bytes. */

    gint64 byterangeend; /*!<
        Download only specified range of bytes. */

    // Will be filled by ::lr_download_packages()

    char *local_path; /*!<
        Local path */

    char *err; /*!<
        Error message or NULL. NULL means no error. */

    GStringChunk *chunk; /*!<
        String chunk */

} LrPackageTarget;

/** Create new LrPackageTarget object.
 * @param handle            Handle related to this download or NULL.
 * @param relative_url      Relative part of URL to download.
 *                          First part of URL will be picked from the LrHandle
 *                          (LRO_URL or mirror) during download process or
 *                          base_url will be used if it is specified.
 * @param dest              Destination filename or just directory (filename
 *                          itself will be derived from the relative_url) or
 *                          NULL (current working directory + filename derived
 *                          from relative_url will be used).
 * @param checksum_type     Type of checksum or LR_CHECKSUM_UNKNOWN.
 * @param checksum          Expected checksum value or NULL.
 * @param base_url          Base URL or NULL
 * @param expectedsize      Expected size of the target. If server reports
 *                          different size, then download won't be performed.
 * @param resume            If TRUE, then downloader will try to resume download
 *                          if the destination file exists. If the file doesn't
 *                          exist it will be downloaded.
 * @param progresscb        Progress callback for this transfer.
 * @param cbdata            User data for the callback
 * @param err               GError **
 * @return                  Newly allocated LrPackageTarget or NULL on error
 */
LrPackageTarget *
lr_packagetarget_new(LrHandle *handle,
                     const char *relative_url,
                     const char *dest,
                     LrChecksumType checksum_type,
                     const char *checksum,
                     gint64 expectedsize,
                     const char *base_url,
                     gboolean resume,
                     LrProgressCb progresscb,
                     void *cbdata,
                     GError **err);

/** Create new LrPackageTarget object.
 * Almost same as lr_packagetarget_new() except this function
 * could set more callbacks.
 * @param handle            Handle related to this download or NULL.
 * @param relative_url      Relative part of URL to download.
 *                          First part of URL will be picked from the LrHandle
 *                          (LRO_URL or mirror) during download process or
 *                          base_url will be used if it is specified.
 * @param dest              Destination filename or just directory (filename
 *                          itself will be derived from the relative_url) or
 *                          NULL (current working directory + filename derived
 *                          from relative_url will be used).
 * @param checksum_type     Type of checksum or LR_CHECKSUM_UNKNOWN.
 * @param checksum          Expected checksum value or NULL.
 * @param base_url          Base URL or NULL
 * @param expectedsize      Expected size of the target. If server reports
 *                          different size, then no download is performed.
 *                          If 0 then size check is ignored.
 * @param resume            If TRUE, then downloader try to resume download
 *                          if destination file exists. If the file doesn't
 *                          exists, it will be normally downloaded again.
 * @param progresscb        Progress callback for this transfer.
 * @param cbdata            User data for the callbacks
 * @param endcb             Callback called when target transfer is done.
 *                          (Use status to check if successfully
 * @param mirrorfailurecb   Called when download from a mirror failed.
 * @param err               GError **
 * @return                  Newly allocated LrPackageTarget or NULL on error
 */
LrPackageTarget *
lr_packagetarget_new_v2(LrHandle *handle,
                        const char *relative_url,
                        const char *dest,
                        LrChecksumType checksum_type,
                        const char *checksum,
                        gint64 expectedsize,
                        const char *base_url,
                        gboolean resume,
                        LrProgressCb progresscb,
                        void *cbdata,
                        LrEndCb endcb,
                        LrMirrorFailureCb mirrorfailurecb,
                        GError **err);

/** Create new LrPackageTarget object.
 * Almost same as lr_packagetarget_new_v2() except this function
 * could set a required byte range of the package.
 * For params see lr_packagetarget_new_v2().
 * @param byterangestart    Download only specified range of bytes. This param
 *                          specifies the begin. 0 is default.
 *                          Note: When this options is != 0 then resume must
 *                          be disabled - resume param must be FALSE.
 * @param byterangeend      Download only specified range of bytes. This param
 *                          specifies the end. 0 is default.
 *                          If this value is less or equal to byterangestart,
 *                          then it is ignored.
 * @return                  Newly allocated LrPackageTarget or NULL on error
 */
LrPackageTarget *
lr_packagetarget_new_v3(LrHandle *handle,
                        const char *relative_url,
                        const char *dest,
                        LrChecksumType checksum_type,
                        const char *checksum,
                        gint64 expectedsize,
                        const char *base_url,
                        gboolean resume,
                        LrProgressCb progresscb,
                        void *cbdata,
                        LrEndCb endcb,
                        LrMirrorFailureCb mirrorfailurecb,
                        gint64 byterangestart,
                        gint64 byterangeend,
                        GError **err);

/** Free ::LrPackageTarget object.
 * @param target        LrPackageTarget object
 */
void
lr_packagetarget_free(LrPackageTarget *target);

/** Available flags for package downloader */
typedef enum {
    LR_PACKAGEDOWNLOAD_FAILFAST    = 1 << 0, /*!<
        If TRUE, then whole downloading is stoped immediately when any
        of download fails (FALSE is returned and err is set). If the failfast
        is FALSE, then this function returns after all downloads finish
        (no matter if successfully or unsuccessfully) and FALSE is returned
        only if a nonrecoverable error related to the function itself is meet
        (Errors related to individual downloads are reported via corresponding
        PackageTarget objects). */
} LrPackageDownloadFlag;

/** Download all LrPackageTargets at the targets GSList.
 * @param targets           GSList where each element is a ::LrPackageTarget
 *                          object
 * @param flags             Bitfield with flags to download
 * @param err               GError **
 * @return                  If FALSE then err is set.
 */
gboolean
lr_download_packages(GSList *targets,
                     LrPackageDownloadFlag flags,
                     GError **err);

typedef enum {
    LR_PACKAGECHECK_FAILFAST    = 1 << 0, /*!<
        If TRUE, then whole check is stoped immediately when any
        of target doesn't exist locally or its checksum doesn't match.
        (FALSE is returned and err is set). If the failfast
        is FALSE, then this function returns after check of all targets
        is finished (no matter if successfully or unsuccessfully) and
        FALSE is returned only if a nonrecoverable error related to the
        function itself is meet (Errors related to individual targets
        are reported via corresponding PackageTarget objects). */
} LrPackageCheckFlag;

/** Check if targets locally exist and checksums match.
 * If target locally exists, then its err is NULL,
 * if it doesn't exists, or checksum is differ. Then target->err is
 * an error message.
 */
gboolean
lr_check_packages(GSList *targets,
                  LrPackageCheckFlag flags,
                  GError **err);

/** @} */

G_END_DECLS

#endif
