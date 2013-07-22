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

#ifndef LR_LRMIRRORLIST_H
#define LR_LRMIRRORLIST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "url_substitution.h"
#include "mirrorlist.h"
#include "metalink.h"

/** A internal representation of a mirror */
typedef struct {
    char *url;      /*!< URL of the mirror */
    int preference; /*!< Integer number 1-100 - higher is better */
    int fails;      /*!< Number of failed downloads from this mirror */
} lr_LrMirror;

typedef GSList lr_LrMirrorlist;

 /** Append url to the mirrorlist.
 * @param list          a lr_LrMirrorlist or NULL
 * @param url           the Url
 * @param urlvars       a lr_UrlVars or NULL
 * @return              the new start of the lr_LrMirrorlist
 */
lr_LrMirrorlist *
lr_lrmirrorlist_append_url(lr_LrMirrorlist *list,
                           const char *url,
                           lr_UrlVars *urlvars);

/** Append mirrors from mirrorlist to the internal mirrorlist.
 * @param iml           Internal mirrorlist or NULL
 * @param mirrorlist    Mirrorlist
 * @param urlvars       a lr_UrlVars or NULL
 * @return              the new start of the lr_LrMirrorlist
 */
lr_LrMirrorlist *
lr_lrmirrorlist_append_mirrorlist(lr_LrMirrorlist *list,
                                  lr_Mirrorlist mirrorlist,
                                  lr_UrlVars *urlvars);

/** Append mirrors from metalink to the internal mirrorlist.
 * @param iml           Internal mirrorlist or NULL
 * @param metalink      Metalink
 * @param suffix        Suffix that shoud be removed from the metalink urls
 * @param urlvars       a lr_UrlVars or NULL
 * @return              the new start of the lr_LrMirrorlist
 */
lr_LrMirrorlist *
lr_lrmirrorlist_append_metalink(lr_LrMirrorlist *list,
                                lr_Metalink *metalink,
                                const char *suffix,
                                lr_UrlVars *urlvars);

/** Append mirrors from another lr_LrMirrorlist.
 * @param iml           Internal mirrorlist
 * @param ml            Other internal mirrorlist
 * @return              the new start of the lr_LrMirrorlist
 */
lr_LrMirrorlist *
lr_lrmirrorlist_append_lrmirrorlist(lr_LrMirrorlist *list,
                                    lr_LrMirrorlist *other);

/** Return mirror on the given position.
 * @param list          a lr_LrMirrorlist
 * @param nth           the position of the mirror
 * @return              the mirror
 */
lr_LrMirror *
lr_lrmirrorlist_nth(lr_LrMirrorlist *list,
                    unsigned int nth);

/** Return url of the mirror on at the given position.
 * @param list          a lr_LrMirrorlist
 * @param nth           the position of the mirror
 * @return              the url
 */
char *
lr_lrmirrorlist_nth_url(lr_LrMirrorlist *list,
                        unsigned int nth);

/** Free lr_LrMirrorlist.
 * @param list          Internal mirrorlist
 */
void
lr_lrmirrorlist_free(lr_LrMirrorlist *list);

#ifdef __cplusplus
}
#endif

#endif
