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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "lrmirrorlist.h"

LrProtocol
lr_detect_protocol(const char *url)
{
    assert(url);

    if (g_str_has_prefix(url, "http://") || g_str_has_prefix(url, "https://"))
        return LR_PROTOCOL_HTTP;

    if (g_str_has_prefix(url, "ftp://"))
        return LR_PROTOCOL_FTP;

    if (g_str_has_prefix(url, "file:/"))
        return LR_PROTOCOL_FILE;

    if (g_str_has_prefix(url, "rsync://"))
        return LR_PROTOCOL_RSYNC;

    if (g_str_has_prefix(url, "oci://"))
        return LR_PROTOCOL_OCI;

    return LR_PROTOCOL_OTHER;
}

static LrInternalMirror *
lr_lrmirror_new(const char *url, LrUrlVars *urlvars)
{
    LrInternalMirror *mirror;

    mirror = lr_malloc0(sizeof(*mirror));
    mirror->url = lr_url_substitute(url, urlvars);
    return mirror;
}

static void
lr_lrmirror_free(void *data)
{
    LrInternalMirror *mirror = data;
    lr_free(mirror->url);
    lr_free(mirror);
}

void
lr_lrmirrorlist_free(LrInternalMirrorlist *list)
{
    if (!list)
        return;

    g_slist_free_full(list, lr_lrmirror_free);
}

LrInternalMirrorlist *
lr_lrmirrorlist_append_url(LrInternalMirrorlist *list,
                           const char *url,
                           LrUrlVars *urlvars)
{
    if (!url || !strlen(url))
        return list;

    LrInternalMirror *mirror = lr_lrmirror_new(url, urlvars);
    mirror->preference = 100;
    mirror->protocol = lr_detect_protocol(mirror->url);

    //g_debug("%s: Appending URL: %s", __func__, mirror->url);

    return g_slist_append(list, mirror);
}

LrInternalMirrorlist *
lr_lrmirrorlist_append_mirrorlist(LrInternalMirrorlist *list,
                                  LrMirrorlist *mirrorlist,
                                  LrUrlVars *urlvars)
{
    if (!mirrorlist || !mirrorlist->urls)
        return list;

    for (GSList *elem = mirrorlist->urls; elem; elem = g_slist_next(elem)) {
        char *url = elem->data;

        if (!url || !strlen(url))
            continue;

        LrInternalMirror *mirror = lr_lrmirror_new(url, urlvars);
        mirror->preference = 100;
        mirror->protocol = lr_detect_protocol(mirror->url);
        list = g_slist_append(list, mirror);

        //g_debug("%s: Appending URL: %s", __func__, mirror->url);
    }

    return list;
}

LrInternalMirrorlist *
lr_lrmirrorlist_append_metalink(LrInternalMirrorlist *list,
                                LrMetalink *metalink,
                                const char *suffix,
                                LrUrlVars *urlvars)
{
    size_t suffix_len = 0;

    if (!metalink || !metalink->urls)
        return list;

    if (suffix)
        suffix_len = strlen(suffix);

    for (GSList *elem = metalink->urls; elem; elem = g_slist_next(elem)) {
        LrMetalinkUrl *metalinkurl = elem->data;
        assert(metalinkurl);
        char *url = metalinkurl->url;

        if (!url)
            continue;  // No url present

        size_t url_len = strlen(url);

        if (!url_len)
            continue;  // No url present

        char *url_copy = NULL;

        if (suffix_len) {
            /* Remove suffix if necessary */
            if (url_len >= suffix_len
                && !strcmp(url+(url_len-suffix_len), suffix))
                url_copy = g_strndup(url, url_len-suffix_len);
        }

        if (!url_copy)
            url_copy = g_strdup(url);

        LrInternalMirror *mirror = lr_lrmirror_new(url_copy, urlvars);
        mirror->preference = metalinkurl->preference;
        mirror->protocol = lr_detect_protocol(mirror->url);
        g_free(url_copy);
        list = g_slist_append(list, mirror);

        //g_debug("%s: Appending URL: %s", __func__, mirror->url);
    }

    return list;
}

LrInternalMirrorlist *
lr_lrmirrorlist_append_lrmirrorlist(LrInternalMirrorlist *list,
                                    LrInternalMirrorlist *other)
{
    if (!other)
        return list;

    for (LrInternalMirrorlist *elem = other; elem; elem = g_slist_next(elem)) {
        LrInternalMirror *oth = elem->data;
        if (!oth->url || !strlen(oth->url))
            continue;
        LrInternalMirror *mirror = lr_lrmirror_new(oth->url, NULL);
        mirror->preference = oth->preference;
        mirror->protocol = oth->protocol;
        list = g_slist_append(list, mirror);
        //g_debug("%s: Appending URL: %s", __func__, mirror->url);
    }

    return list;
}

LrInternalMirror *
lr_lrmirrorlist_nth(LrInternalMirrorlist *list,
                    unsigned int nth)
{
    return g_slist_nth_data(list, nth);
}

char *
lr_lrmirrorlist_nth_url(LrInternalMirrorlist *list,
                        unsigned int nth)
{
    LrInternalMirror *mirror = g_slist_nth_data(list, nth);
    return (mirror) ? mirror->url : NULL;
}
