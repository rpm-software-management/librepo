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

#ifndef LR_YUMREPO_H
#define LR_YUMREPO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "librepo.h"

/* Return codes of the module:
 *  LRE_OK      everything ok
 *  LRE_IO      input/output error
 *  LRE_URL     no usable URL (no base URL or mirrorlist URL specified)
 *  + Codes from metalink.h
 *  + Codes from mirrorlist.h
 *  + Codes from repomd.h
 *  + Codes from curl.h
 */

int lr_yum_perform(lr_Handle handle, lr_Result result);

#ifdef __cplusplus
}
#endif

#endif
