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

#ifndef LR_FASTESTMIRROR_INTERNAL_H
#define LR_FASTESTMIRROR_INTERNAL_H

#include <glib.h>

#include "handle.h"

G_BEGIN_DECLS

gboolean
lr_fastestmirror_sort_internalmirrorlist(LrHandle *handle,
                                         GSList **list,
                                         GError **err);

G_END_DECLS

#endif
