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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <float.h>
#include <curl/curl.h>

#include "util.h"
#include "handle_internal.h"
#include "rcodes.h"
#include "fastestmirror.h"
#include "fastestmirror_internal.h"

#define LENGT_OF_MEASUREMENT        2.0    // Number of seconds (float point!)
#define HALF_OF_SECOND_IN_MICROS    500000

typedef struct {
    gchar *url;                 // Points to string passed by the user
    CURL *curl;
    double plain_connect_time;
} LrFastestMirror;

static LrFastestMirror *
lr_lrfastestmirror_new()
{
    LrFastestMirror *mirror = g_new0(LrFastestMirror, 1);
    mirror->plain_connect_time = 0.0;
    return mirror;
}

void
lr_lrfastestmirror_free(LrFastestMirror *mirror)
{
    if (!mirror)
        return;
    if (mirror->curl)
        curl_easy_cleanup(mirror->curl);
    g_free(mirror);
}

/** Create list of LrFastestMirror based on input list of URLs.
 */
gboolean
lr_fastestmirror_prepare(LrHandle *handle,
                         GSList *in_list,
                         GSList **out_list,
                         GError **err)
{
    gboolean ret = TRUE;
    GSList *list = NULL;

    assert(!err || *err == NULL);

    if (!in_list) {
        *out_list = NULL;
        return TRUE;
    }

    for (GSList *elem = in_list; elem; elem = g_slist_next(elem)) {
        gchar *url = elem->data;
        CURLcode curlcode;
        CURL *curlh;

        if (handle)
            curlh = curl_easy_duphandle(handle->curl_handle);
        else
            curlh = lr_get_curl_handle();

        if (!curlh) {
            g_set_error(err, LR_FASTESTMIRROR_ERROR, LRE_CURL,
                        "Cannot create curl handle");
            ret = FALSE;
            break;
        }

        LrFastestMirror *mirror = lr_lrfastestmirror_new();
        mirror->url = url;
        mirror->curl = curlh;

        curlcode = curl_easy_setopt(curlh, CURLOPT_URL, url);
        if (curlcode != CURLE_OK) {
            g_set_error(err, LR_FASTESTMIRROR_ERROR, LRE_CURL,
                        "curl_easy_setopt(_, CURLOPT_URL, %s) failed: %s",
                        url, curl_easy_strerror(curlcode));
            ret = FALSE;
            break;
        }

        curlcode = curl_easy_setopt(curlh, CURLOPT_CONNECT_ONLY, 1);
        if (curlcode != CURLE_OK) {
            g_set_error(err, LR_FASTESTMIRROR_ERROR, LRE_CURL,
                    "curl_easy_setopt(_, CURLOPT_CONNECT_ONLY, 1) failed: %s",
                    curl_easy_strerror(curlcode));
            ret = FALSE;
            break;
        }

        list = g_slist_append(list, mirror);
    }

    if (ret) {
        *out_list = list;
    } else {
        assert(!err || *err);
        g_slist_free_full(list, (GDestroyNotify)lr_lrfastestmirror_free);
        *out_list = NULL;
    }

    return ret;
}

static gboolean
lr_fastestmirror_perform(GSList *list, GError **err)
{
    assert(!err || *err == NULL);

    if (!list)
        return TRUE;

    CURLM *multihandle = curl_multi_init();
    if (!multihandle) {
        g_set_error(err, LR_FASTESTMIRROR_ERROR, LRE_CURL,
                    "curl_multi_init() error");
        return FALSE;
    }

    // Add curl easy handles to multi handle
    for (GSList *elem = list; elem; elem = g_slist_next(elem)) {
        LrFastestMirror *mirror = elem->data;
        curl_multi_add_handle(multihandle, mirror->curl);
    }

    int still_running;
    gdouble elapsed_time = 0.0;
    GTimer *timer = g_timer_new();
    g_timer_start(timer);

    do {
        struct timeval timeout;
        int rc, cm_rc;
        int maxfd = -1;
        long curl_timeout = -1;
        fd_set fdread, fdwrite, fdexcep;

        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdexcep);

        // Set suitable timeout to play around with
        timeout.tv_sec  = 0;
        timeout.tv_usec = HALF_OF_SECOND_IN_MICROS;

        cm_rc = curl_multi_timeout(multihandle, &curl_timeout);
        if (cm_rc != CURLM_OK) {
            g_set_error(err, LR_FASTESTMIRROR_ERROR, LRE_CURLM,
                        "curl_multi_timeout() error: %s",
                        curl_multi_strerror(cm_rc));
            curl_multi_cleanup(multihandle);
            return FALSE;
        }

        // Set timeout to a reasonable value
        if (curl_timeout >= 0) {
            timeout.tv_sec = curl_timeout / 1000;
            if (timeout.tv_sec >= 1) {
                timeout.tv_sec = 0;
                timeout.tv_usec = HALF_OF_SECOND_IN_MICROS;
            } else {
                timeout.tv_usec = (curl_timeout % 1000) * 1000;
                if (timeout.tv_usec > HALF_OF_SECOND_IN_MICROS)
                    timeout.tv_usec = HALF_OF_SECOND_IN_MICROS;
            }
        }

        // Get file descriptors from the transfers
        cm_rc = curl_multi_fdset(multihandle, &fdread, &fdwrite,
                                 &fdexcep, &maxfd);
        if (cm_rc != CURLM_OK) {
            g_set_error(err, LR_FASTESTMIRROR_ERROR, LRE_CURLM,
                        "curl_multi_fdset() error: %s",
                        curl_multi_strerror(cm_rc));
            return FALSE;
        }

        rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
        if (rc < 0) {
            if (errno == EINTR) {
                g_debug("%s: select() interrupted by signal", __func__);
            } else {
                g_set_error(err, LR_FASTESTMIRROR_ERROR, LRE_SELECT,
                            "select() error: %s", strerror(errno));
                return FALSE;
            }
        }

        curl_multi_perform(multihandle, &still_running);

        // Break loop after some reasonable amount of time
        elapsed_time = g_timer_elapsed(timer, NULL);

    } while(still_running && elapsed_time < LENGT_OF_MEASUREMENT);

    g_timer_destroy(timer);

    // Remove curl easy handles from multi handle
    // and calculate plain_connect_time
    for (GSList *elem = list; elem; elem = g_slist_next(elem)) {
        LrFastestMirror *mirror = elem->data;
        CURL *curl = mirror->curl;

        // Remove handle
        curl_multi_remove_handle(multihandle, curl);

        // Calculate plain_connect_time
        char *effective_url;
        curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url);

        if (!effective_url) {
            // No effective url is most likely an error
            mirror->plain_connect_time = DBL_MAX;
        } else if (g_str_has_prefix(effective_url, "file://")) {
            // Local directories are considered to be the best mirrors
            mirror->plain_connect_time = 0.0;
        } else {
            // Get connect time
            double namelookup_time;
            double connect_time;
            curl_easy_getinfo(curl, CURLINFO_NAMELOOKUP_TIME, &namelookup_time);
            curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME, &connect_time);

            if (connect_time == 0.0) {
                // Zero connect time is most likely an error
                connect_time = DBL_MAX;
            }

            mirror->plain_connect_time = connect_time;
            //g_debug("%s: name_lookup: %3.6f connect_time:  %3.6f (%3.6f) | %s",
            //        __func__, namelookup_time, connect_time,
            //        mirror->plain_connect_time, mirror->url);
        }
    }

    curl_multi_cleanup(multihandle);
    return TRUE;
}

gboolean
lr_fastestmirror(LrHandle *handle, GSList **list, GError **err)
{
    assert(!err || *err == NULL);

    g_debug("%s: Fastest mirror determination in progress...", __func__);

    if (!list || *list == NULL)
        return TRUE;

    gboolean ret;
    GSList *lrfastestmirrors;
    ret = lr_fastestmirror_prepare(handle, *list, &lrfastestmirrors, err);
    if (!ret) {
        g_debug("%s: Error while lr_fastestmirror_prepare()", __func__);
        return FALSE;
    }

    ret = lr_fastestmirror_perform(lrfastestmirrors, err);
    if (!ret) {
        g_debug("%s: Error while lr_fastestmirror_perform()", __func__);
        g_slist_free_full(lrfastestmirrors,
                          (GDestroyNotify)lr_lrfastestmirror_free);
        return FALSE;
    }

    // Sort the mirrors by the connection time
    GSList *new_list = NULL;
    while (lrfastestmirrors) {
        LrFastestMirror *mirror = lrfastestmirrors->data;
        double min_value = mirror->plain_connect_time;

        for (GSList *m=g_slist_next(lrfastestmirrors); m; m=g_slist_next(m)) {
            LrFastestMirror *current_mirror = m->data;
            if (current_mirror->plain_connect_time < min_value) {
                min_value = current_mirror->plain_connect_time;
                mirror = current_mirror;
            }
        }

        g_debug("%s: %3.6f : %s", __func__, min_value, mirror->url);
        new_list = g_slist_append(new_list, mirror->url);
        lrfastestmirrors = g_slist_remove(lrfastestmirrors, mirror);
        lr_lrfastestmirror_free(mirror);
    }

    assert(!lrfastestmirrors);
    g_slist_free(*list);
    *list = new_list;

    return TRUE;
}

gboolean
lr_fastestmirror_sort_internalmirrorlist(LrHandle *handle,
                                         GError **err)
{
    assert(!err || *err == NULL);
    GSList *list = g_slist_prepend(NULL, handle);
    gboolean ret = lr_fastestmirror_sort_internalmirrorlists(list, err);
    g_slist_free(list);

    return ret;
}

gboolean
lr_fastestmirror_sort_internalmirrorlists(GSList *handles,
                                          GError **err)
{
    assert(!err || *err == NULL);

    if (!handles)
        return TRUE;

    // Prepare list of hosts
    GHashTable *hosts_ht = g_hash_table_new_full(g_str_hash,
                                                 g_str_equal,
                                                 g_free,
                                                 NULL);

    for (GSList *ehandle = handles; ehandle; ehandle = g_slist_next(ehandle)) {
        LrHandle *handle = ehandle->data;
        GSList *mirrors = handle->internal_mirrorlist;
        for (GSList *elem = mirrors; elem; elem = g_slist_next(elem)) {
            LrInternalMirror *imirror = elem->data;
            gchar *host = lr_url_without_path(imirror->url);
            g_hash_table_insert(hosts_ht, host, NULL);
        }
    }

    GList *tmp_list_of_urls = g_hash_table_get_keys(hosts_ht);
    GSList *list_of_urls = NULL;
    for (GList *elem = tmp_list_of_urls; elem; elem = g_list_next(elem)) {
        list_of_urls = g_slist_prepend(list_of_urls, elem->data);
    }
    g_list_free(tmp_list_of_urls);

    // Sort this list by the connection time
    LrHandle *main_handle = handles->data;  // Network configuration for the
                                            // test is used from the first
                                            // handle
    gboolean ret = lr_fastestmirror(main_handle, &list_of_urls, err);
    if (!ret) {
        g_debug("%s: lr_fastestmirror failed", __func__);
        g_slist_free(list_of_urls);
        return FALSE;
    }

    // Apply sorted order to each handle
    for (GSList *ehandle = handles; ehandle; ehandle = g_slist_next(ehandle)) {
        LrHandle *handle = ehandle->data;
        GSList *mirrors = handle->internal_mirrorlist;
        GSList *new_list = NULL;
        for (GSList *elem = list_of_urls; elem; elem = g_slist_next(elem)) {
            gchar *host = elem->data;
            for (GSList *ime = mirrors; ime; ime = g_slist_next(ime)) {
                LrInternalMirror *im = ime->data;
                gchar *im_host = lr_url_without_path(im->url);
                if (!g_strcmp0(im_host, host)) {
                    new_list = g_slist_prepend(new_list, im);
                    // XXX: Maybe convert GSList to GList to make
                    // this delete more efficient
                    mirrors = g_slist_delete_link(mirrors, ime);
                    break;
                }
            }
        }

        // If multiple mirrors with the same lr_url_without_path(url)
        // were present, only the first occurrence was inserted to the
        // the new_list and removed from the mirrors list.
        // The remaining occurences will be moved here.
        for (GSList *elem = mirrors; elem; elem = g_slist_next(elem)) {
            LrInternalMirror *im = elem->data;
            new_list = g_slist_prepend(new_list, im);
        }
        g_slist_free(mirrors);

        // Set sorted list to the handle (reversed, because the items
        // of the new_list were prepended)
        handle->internal_mirrorlist = g_slist_reverse(new_list);
    }

    g_slist_free(list_of_urls);

    return TRUE;
}
