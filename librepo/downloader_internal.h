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

#include "handle.h"

typedef struct {
    LrProgressCb cb; /*!<
        User callback */

    LrMirrorFailureCb mfcb; /*!<
        Mirror failure callback */

    GSList *singlecbdata; /*!<
        List of LrCallbackData */

} LrSharedCallbackData;

typedef struct {
    double downloaded;  /*!< Currently downloaded bytes of target */
    double total;       /*!< Total size of the target */
    void *userdata;     /*!< User data related to the target */
    LrSharedCallbackData *sharedcbdata; /*!< Shared cb data */
} LrCallbackData;

#endif //LIBREPO_DOWNLOADER_INTERNAL_H
