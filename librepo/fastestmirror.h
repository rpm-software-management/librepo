/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2013  Tomas Mlcoch
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

#ifndef __LR_FASTESTMIRROR_H__
#define __LR_FASTESTMIRROR_H__

#include <glib.h>
#include <curl/curl.h>

#include <librepo/url_substitution.h>
#include <librepo/mirrorlist.h>
#include <librepo/metalink.h>
#include <librepo/handle.h>

G_BEGIN_DECLS

typedef struct {
    gchar *url;                 // Points to string passed by the user
    CURL *curl;                 // Curl handle or NULL
    double plain_connect_time;  // Mirror connect time (<0.0 if connection was unsuccessful)
    gboolean cached;            // Was connect time load from cache?
} LrFastestMirror;


/** Free LrFastestMirror
 */
void
lr_lrfastestmirror_free(LrFastestMirror *mirror);


/** Sorts list or mirror URLs by their connections times.
 * @param handle        LrHandle or NULL
 * @param list          Pointer to the GSList of urls (char* or gchar*)
 *                      that will be sorted.
 * @param err           GError **
 * @return              TRUE if everything is ok, FALSE if the err is set.
 */
gboolean
lr_fastestmirror(LrHandle *handle, GSList **list, GError **err);


/** For list of URLs create a sorted list of LrFastestMirrors
 * Note: url strings in outlist poins to the strings from inlist!
 * @param handle        LrHandle or NULL
 * @param inlist        GSList of urls (char* or gchar*), this list
 *                      stays unchanged
 * @param outlist       GSList of LrFastestMirror*
 * @param err           GError **
 * @return              TRUE if everything is ok, FLASE if the err is set
 */
gboolean
lr_fastestmirror_detailed(LrHandle *handle,
                          GSList *inlist,
                          GSList **outlist,
                          GError **err);


G_END_DECLS

#endif
