/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2016  Martin Hatina
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

#ifndef LIBREPO_METADATA_DOWNLOADER_H
#define LIBREPO_METADATA_DOWNLOADER_H

#include <glib.h>

#include "yum.h"
#include "handle.h"
#include "repomd.h"
#include "downloadtarget.h"

/** LrMetadataTarget structure */
typedef struct {

    LrHandle *handle; /*!<
        Related handle */

    LrYumRepo *repo; /*!<
        Related repo */

    LrYumRepoMd *repomd; /*!<
        Related repomd */

    int repomd_records_to_download;
    int repomd_records_downloaded;/*!<
        Necessary for endcb, which would be otherwise called prematurely */

    void *cbdata; /*!<
        Callback data */

    LrProgressCb progresscb; /*!<
        Progress callback */

    LrMirrorFailureCb mirrorfailurecb;

    LrEndCb endcb; /*!<
        Callback called when target transfer is done.
        (Use status to check if successfully or unsuccessfully) */

    GList *err; /*!<
        Array of error messages or NULL. NULL means no error. */

    GStringChunk *chunk; /*!<
        String chunk */

    LrDownloadTarget *download_target; /*!<
        Download target for repomd.xml */

    gchar *gnupghomedir; /*!<
        GNUPG home dir. */

} LrMetadataTarget;

/**
 * Create new LrMetadataTarget object.
 * @param handle Handle related to this download or NULL.
 * @param repo Related repo.
 * @param repomd Related repomd.
 * @param cbdata User data for the callback
 * @param err GError **
 * @return Newly allocated LrMetadataTarget or NULL on error
 */
LrMetadataTarget *
lr_metadatatarget_new(LrHandle *handle, LrYumRepo *repo, LrYumRepoMd *repomd, void *cbdata, GError **err);

/**
 * Create new LrMetadataTarget object.
 * Almost same as lr_metadatatarget_new() except this function
 * could set more callbacks.
 * @param handle Handle related to this download or NULL.
 * @param cbdata User data for the callback
 * @param progresscb Progress callback for this transfer.
 * @param mirror_failure_cb Called when download from a mirror failed.
 * @param endcb Callback called when target transfer is done.
 * @param err GError **
 * @return Newly allocated LrMetadataTarget or NULL on error
 */
LrMetadataTarget *
lr_metadatatarget_new2(LrHandle *handle,
                       void *cbdata,
                       LrProgressCb progresscb,
                       LrMirrorFailureCb mirror_failure_cb,
                       LrEndCb endcb,
                       const char *gnupghomedir,
                       GError **err);

/** Free ::LrMetadataTarget object.
 * @param target LrMetadataTarget object
 */
void
lr_metadatatarget_free(LrMetadataTarget *target);

/**
 * Download all LrMetadataTargets at the targets GSList.
 * @param targets GSList where each element is a ::LrPackageTarget object
 * @param err GError **
 * @return If FALSE then err is set.
 */
gboolean
lr_download_metadata(GSList *targets, GError **err);

#endif //LIBREPO_METADATA_DOWNLOADER_H
