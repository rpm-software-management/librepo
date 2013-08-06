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

#ifndef LR_PACKAGE_DOWNLOADER_H
#define LR_PACKAGE_DOWNLOADER_H

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
                                        NULL, NULL, 0, (err))

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
                    const char *base_url,
                    gboolean resume,
                    GError **err);

typedef struct {

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

    gboolean resume; /*!<
        Indicate if resume is enabled */

    LrProgressCb progresscb; /*!<
        Progress callback */

    void *cbdata; /*!<
        Callback data */

    // Will be filled by ::lr_download_packages()

    char *local_path; /*!<
        Local path */

    char *err; /*!<
        Error message or NULL. NULL means no error. */

    GStringChunk *chunk; /*!<
        String chunk */

} LrPackageTarget;

LrPackageTarget *
lr_packagetarget_new(const char *relative_url,
                     const char *dest,
                     LrChecksumType checksum_type,
                     const char *checksum,
                     const char *base_url,
                     gboolean resume,
                     LrProgressCb progresscb,
                     void *cbdata,
                     GError **err);

void
lr_packagetarget_free(LrPackageTarget *target);

gboolean
lr_download_packages(LrHandle *handle,
                     GSList *targets,
                     gboolean failfast,
                     GError **err);

/** @} */

G_END_DECLS

#endif
