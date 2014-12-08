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

#ifndef __LR_LRMIRRORLIST_H__
#define __LR_LRMIRRORLIST_H__

#include <glib.h>

#include "url_substitution.h"
#include "mirrorlist.h"
#include "metalink.h"

G_BEGIN_DECLS

typedef enum {
    LR_PROTOCOL_OTHER,
    LR_PROTOCOL_FILE,
    LR_PROTOCOL_HTTP,
    LR_PROTOCOL_FTP,
    LR_PROTOCOL_RSYNC,
} LrProtocol;

/** A internal representation of a mirror */
typedef struct {
    char *url;           /*!< URL of the mirror */
    int preference;      /*!< Integer number 1-100 - higher is better */
    LrProtocol protocol; /*!< Protocol of this mirror */
} LrInternalMirror;

typedef GSList LrInternalMirrorlist;

/** Detect URL protocol.
 * @param url       URL
 * @return          Type of detected protocol
 */
LrProtocol
lr_detect_protocol(const char *url);

 /** Append url to the mirrorlist.
 * @param list          a LrInternalMirrorlist or NULL
 * @param url           the Url
 * @param urlvars       a LrUrlVars or NULL
 * @return              the new start of the LrInternalMirrorlist
 */
LrInternalMirrorlist *
lr_lrmirrorlist_append_url(LrInternalMirrorlist *list,
                           const char *url,
                           LrUrlVars *urlvars);

/** Append mirrors from mirrorlist to the internal mirrorlist.
 * @param iml           Internal mirrorlist or NULL
 * @param mirrorlist    Mirrorlist
 * @param urlvars       a LrUrlVars or NULL
 * @return              the new start of the LrInternalMirrorlist
 */
LrInternalMirrorlist *
lr_lrmirrorlist_append_mirrorlist(LrInternalMirrorlist *list,
                                  LrMirrorlist *mirrorlist,
                                  LrUrlVars *urlvars);

/** Append mirrors from metalink to the internal mirrorlist.
 * @param iml           Internal mirrorlist or NULL
 * @param metalink      Metalink
 * @param suffix        Suffix that shoud be removed from the metalink urls
 * @param urlvars       a LrUrlVars or NULL
 * @return              the new start of the LrInternalMirrorlist
 */
LrInternalMirrorlist *
lr_lrmirrorlist_append_metalink(LrInternalMirrorlist *list,
                                LrMetalink *metalink,
                                const char *suffix,
                                LrUrlVars *urlvars);

/** Append mirrors from another LrInternalMirrorlist.
 * @param iml           Internal mirrorlist
 * @param ml            Other internal mirrorlist
 * @return              the new start of the LrInternalMirrorlist
 */
LrInternalMirrorlist *
lr_lrmirrorlist_append_lrmirrorlist(LrInternalMirrorlist *list,
                                    LrInternalMirrorlist *other);

/** Return mirror on the given position.
 * @param list          a LrInternalMirrorlist
 * @param nth           the position of the mirror
 * @return              the mirror
 */
LrInternalMirror *
lr_lrmirrorlist_nth(LrInternalMirrorlist *list,
                    unsigned int nth);

/** Return url of the mirror on at the given position.
 * @param list          a LrInternalMirrorlist
 * @param nth           the position of the mirror
 * @return              the url
 */
char *
lr_lrmirrorlist_nth_url(LrInternalMirrorlist *list,
                        unsigned int nth);

/** Free LrInternalMirrorlist.
 * @param list          Internal mirrorlist
 */
void
lr_lrmirrorlist_free(LrInternalMirrorlist *list);

G_END_DECLS

#endif
