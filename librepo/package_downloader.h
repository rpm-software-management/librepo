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

#ifdef __cplusplus
extern "C" {
#endif

#include "rcodes.h"
#include "handle.h"
#include "checksum.h"

#define lr_download_simple(HANDLE, URL) \
                        lr_download_package((HANDLE), (URL), NULL, 0, NULL, NULL, 0)

int lr_download_package(lr_Handle handle,
                        const char *relative_url,
                        const char *dest,
                        lr_ChecksumType checksum_type,
                        const char *checksum,
                        const char *base_url,
                        int resume);

#ifdef __cplusplus
}
#endif

#endif
