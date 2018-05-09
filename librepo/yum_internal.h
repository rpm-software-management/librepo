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

#ifndef __LR_YUM_INTERNAL_H__
#define __LR_YUM_INTERNAL_H__

#include <glib.h>

#include "rcodes.h"
#include "result.h"
#include "handle.h"

G_BEGIN_DECLS

gboolean
lr_yum_perform(LrHandle *handle, LrResult *result, GError **err);
gboolean
lr_yum_download_url(LrHandle *lr_handle, const char *url, int fd,
                    gboolean no_cache, gboolean is_zchunk, GError **err);

G_END_DECLS

#endif
