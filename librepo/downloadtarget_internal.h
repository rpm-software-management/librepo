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

#ifndef LR_DOWNLOADTARGET_INTERNAL_H
#define LR_DOWNLOADTARGET_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <glib.h>
#include "downloadtarget.h"
#include "types.h"

/** Helper function to comfortable setting of error to the ::LrDownloadTarget.
 */
void
lr_downloadtarget_set_error(LrDownloadTarget *target,
                            LrRc code,
                            const char *format,
                            ...);

/** Helper function to comfortable setting usedmirror attribute
 * of ::LrDownloadTarget.
 */
void
lr_downloadtarget_set_usedmirror(LrDownloadTarget *target, const char *url);

/** Helper function to comfortable setting effectiveurl attribute
 * of ::ld_DownloadTarget
 */
void
lr_downloadtarget_set_effectiveurl(LrDownloadTarget *target, const char *url);

#ifdef __cplusplus
}
#endif

#endif
