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

/** A mirror of internal mirrorlist */
struct _lr_InternalMirror {
    char *url;      /*!< URL of the mirror */
    int preference; /*!< Integer number 1-100 - higher is better */
    int fails;      /*!< Number of failed downloads from this mirror */
};

/** Pointer to ::_lr_InternalMirror */
typedef struct _lr_InternalMirror * lr_InternalMirror;

/** Internal mirrorlist used internaly in handle */
struct _lr_InternalMirrorlist {
    struct _lr_InternalMirror **mirrors;    /*!< Mirrorlist */
    int nom;                                /*!< Number of mirrors */
};

/** Pointer to _lr_InternalMirrorlist */
typedef struct _lr_InternalMirrorlist * lr_InternalMirrorlist;

/**
 * Create new empty internal mirrorlist.
 * @return              New allocated mirrorlist.
 */
lr_InternalMirrorlist lr_internalmirrorlist_new();

/**
 * Free internal mirrorlist.
 * @param iml           Internal mirrorlist.
 */
void lr_internalmirrorlist_free(lr_InternalMirrorlist iml);

/**
 * Append url to the mirrorlist.
 * @param iml           Internal mirrorlist.
 * @param url           Url.
 */
void lr_internalmirrorlist_append_url(lr_InternalMirrorlist iml,
                                      const char *url);

/**
 * Append mirrors from mirrorlist to the internal mirrorlist.
 * @param iml           Internal mirrorlist.
 * @param mirrorlist    Mirrorlist.
 */
void lr_internalmirrorlist_append_mirrorlist(lr_InternalMirrorlist iml,
                                             lr_Mirrorlist mirrorlist);

/**
 * Append mirrors from metalink to the internall mirrorlist.
 * @param iml           Internal mirrorlist.
 * @param metalink      Metalink.
 * @param suffix        Suffix that shoud be removed from the metalink urls.
 */
void lr_internalmirrorlist_append_metalink(lr_InternalMirrorlist iml,
                                           lr_Metalink metalink,
                                           const char *suffix);

/**
 * Get mirror on the selected position.
 * @param iml           Internal mirrorlist.
 * @param i             Position of the mirror.
 * @return              Selected mirror or NULL.
 */
lr_InternalMirror lr_internalmirrorlist_get(lr_InternalMirrorlist iml, int i);

/**
 * Get url of the mirror on the selected position.
 * @param iml           Internal mirrorlist.
 * @param i             Position of the mirror.
 * @return              Url of the mirror or NULL.
 */
char *lr_internalmirrorlist_get_url(lr_InternalMirrorlist iml, int i);

/**
 * Length of the internal mirrorlist.
 * @param iml           Internal mirrorlist.
 * @return              Length of the internall mirrorlist.
 */
int lr_internalmirrorlist_len(lr_InternalMirrorlist iml);

#ifdef __cplusplus
}
#endif

#endif
