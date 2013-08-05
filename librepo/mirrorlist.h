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

#ifndef LR_MIRRORLIST_H
#define LR_MIRRORLIST_H

#include <glib.h>

G_BEGIN_DECLS

/** \defgroup   mirrorlist    Mirrorlist parser
 *  \addtogroup mirrorlist
 *  @{
 */

/** Mirrorlist */
typedef struct {
    GSList *urls;    /*!< List URLs (char *), could be NULL */
} LrMirrorlist;

/**
 * Create new empty mirrorlist.
 * @return              New empty mirrorlist.
 */
LrMirrorlist *
lr_mirrorlist_init();

/**
 * Parse mirrorlist file.
 * @param mirrorlist    Mirrorlist object.
 * @param fd            Opened file descriptor of mirrorlist file.
 * @param err           GError **
 * @return              TRUE if everything is ok, FALSE if err is set.
 */
gboolean
lr_mirrorlist_parse_file(LrMirrorlist *mirrorlist, int fd, GError **err);

/**
 * Free mirrorlist and all its content.
 * @param mirrorlist    Mirrorlist object.
 */
void
lr_mirrorlist_free(LrMirrorlist *mirrorlist);

/** @} */

G_END_DECLS

#endif
