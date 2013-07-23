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

#ifndef LR_SETUP_H
#define LR_SETUP_H

#ifdef __cplusplus
extern "C" {
#endif

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN    "librepo"

#define TMP_DIR_TEMPLATE    "librepo-XXXXXX"

#ifdef DEBUG
#define DEBUGF(x) x
#else
#define DEBUGF(x) do {} while(0)  /* Just to force write ';' after DEBUGF() */
#endif

/* DEBUGASSERT is only for debuging.
 * For assertion which shoud be always valid, assert() is used directly.
 */
#ifdef DEBUG
#include <assert.h>
#define DEBUGASSERT(x) assert(x)
#else
#define DEBUGASSERT(x) do {} while(0)
#endif

#ifdef __cplusplus
}
#endif

#endif
