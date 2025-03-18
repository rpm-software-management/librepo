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

#ifndef LIBREPO_DOWNLOADER_INTERNAL_H
#define LIBREPO_DOWNLOADER_INTERNAL_H

#include "librepo/metadata_downloader.h"
#include <librepo/handle.h>

G_BEGIN_DECLS

typedef struct {
    LrProgressCb cb; /*!<
        User callback */

    LrMirrorFailureCb mfcb; /*!<
        Mirror failure callback */

    LrEndCb endcb; /*!<
        End callback */

    GSList *singlecbdata; /*!<
        List of LrCallbackData */

    LrMetadataTarget * target; /*!<
        Metadata target for which are these callback data shared.
        It is currently only used in lr_multi_end_func to determine
        if endcb should be called. */
} LrSharedCallbackData;

typedef struct {
    double downloaded;  /*!< Currently downloaded bytes of target */
    double total;       /*!< Total size of the target */
    void *userdata;     /*!< User data related to the target */
    LrSharedCallbackData *sharedcbdata; /*!< Shared cb data */
} LrCallbackData;

int
lr_multi_progress_func(void* ptr,
                       double total_to_download,
                       double now_downloaded);

int
lr_multi_mf_func(void *ptr, const char *msg, const char *url);

int
lr_metadata_target_end_func(void *ptr, LrTransferStatus status, const char *msg);

G_END_DECLS

#endif //LIBREPO_DOWNLOADER_INTERNAL_H
