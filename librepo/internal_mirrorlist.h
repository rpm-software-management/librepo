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

#ifndef LR_INTERNAL_MIRRORLIST_H
#define LR_INTERNAL_MIRRORLIST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mirrorlist.h"
#include "metalink.h"

struct _lr_InternalMirror {
    char *url;
    int preference;  /*!< Integer number 1-100 - higher is better */
    int fails;       /*!< Number of failed downloads from this mirror */
};
typedef struct _lr_InternalMirror * lr_InternalMirror;

struct _lr_InternalMirrorlist {
    struct _lr_InternalMirror **mirrors;
    int nom;  /*!< Number of mirrors */
};
typedef struct _lr_InternalMirrorlist * lr_InternalMirrorlist;

lr_InternalMirrorlist lr_internalmirrorlist_new();
void lr_internalmirrorlist_free(lr_InternalMirrorlist);
void lr_internalmirrorlist_append_url(lr_InternalMirrorlist iml, const char *url);
void lr_internalmirrorlist_append_mirrorlist(lr_InternalMirrorlist iml,
                                             lr_Mirrorlist mirrorlist);
void lr_internalmirrorlist_append_metalink(lr_InternalMirrorlist iml,
                                           lr_Metalink metalink,
                                           const char *suffix);
lr_InternalMirror lr_internalmirrorlist_get(lr_InternalMirrorlist, int);
char *lr_internalmirrorlist_get_url(lr_InternalMirrorlist, int);
int lr_internalmirrorlist_len(lr_InternalMirrorlist);

#ifdef __cplusplus
}
#endif

#endif
