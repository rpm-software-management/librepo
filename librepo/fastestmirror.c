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
#include <unistd.h>
#include <errno.h>
#include <float.h>
#include <curl/curl.h>

#include "util.h"
#include "cleanup.h"
#include "handle_internal.h"
#include "rcodes.h"
#include "fastestmirror.h"
#include "fastestmirror_internal.h"

#define LENGTH_OF_MEASUREMENT        2.0    // Number of seconds (float point!)
#define HALF_OF_SECOND_IN_MICROS    500000

#define CACHE_GROUP_METADATA    ":_librepo_:"   // Group with metadata
#define CACHE_KEY_TS            "ts"            // Timestamp
// FIXME: next time the cache version is updated, please fix this typo! It
// should obviously have been "connecttime".
#define CACHE_KEY_CONNECTTIME   "connectime"    // Time of response
#define CACHE_KEY_VERSION       "version"       // Version of cache format

#define CACHE_VERSION   1   // Current version of cache format

#define CACHE_RECORD_MAX_AGE    (LRO_FASTESTMIRRORMAXAGE_DEFAULT * 6)

typedef struct {
    gchar *path;
    GKeyFile *keyfile;
} LrFastestMirrorCache;

static LrFastestMirror *
lr_lrfastestmirror_new()
{
    LrFastestMirror *mirror = g_new0(LrFastestMirror, 1);
    mirror->plain_connect_time = 0.0;
    mirror->cached = FALSE;
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

static gboolean
lr_fastestmirrorcache_load(LrFastestMirrorCache **cache,
                           gchar *path,
                           LrFastestMirrorCb cb,
                           void *cbdata,
                           GError **err)
{
    assert(cache);
    assert(!err || *err == NULL);

    if (!path) {
        // No cache file specified
        *cache = NULL;
        return TRUE;
    }

    cb(cbdata, LR_FMSTAGE_CACHELOADING, path);

    GKeyFile *keyfile = g_key_file_new();

    *cache = lr_malloc0(sizeof(LrFastestMirrorCache));
    (*cache)->path = g_strdup(path);
    (*cache)->keyfile = keyfile;

    if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
        // Cache file doesn't exist
        cb(cbdata, LR_FMSTAGE_CACHELOADINGSTATUS,
           "Cache doesn't exist");
    } else {
        // Cache exists, try to load it

        gboolean something_wrong = FALSE;
        GError *tmp_err = NULL;
        gboolean ret = g_key_file_load_from_file(keyfile,
                                                 path,
                                                 G_KEY_FILE_NONE,
                                                 &tmp_err);
        if (!ret) {
            // Cannot parse cache file
            char *msg = g_strdup_printf("Cannot parse fastestmirror "
                                        "cache %s: %s", path, tmp_err->message);
            g_debug("%s: %s", __func__, msg);
            cb(cbdata, LR_FMSTAGE_CACHELOADINGSTATUS, msg);
            something_wrong = TRUE;
            g_free(msg);
            g_error_free(tmp_err);
        } else {
            // File parsed successfully
            if (!g_key_file_has_group(keyfile, CACHE_GROUP_METADATA)) {
                // Not a fastestmirror cache
                g_debug("%s: File %s is not a fastestmirror cache file",
                          __func__, path);
                cb(cbdata, LR_FMSTAGE_CACHELOADINGSTATUS,
                   "File is not a fastestmirror cache");
                something_wrong = TRUE;
            } else {
                int version = (int) g_key_file_get_integer(keyfile,
                                                           CACHE_GROUP_METADATA,
                                                           CACHE_KEY_VERSION,
                                                           NULL);
                if (version != CACHE_VERSION) {
                    g_debug("%s: Old cache version %d vs %d",
                            __func__, version, CACHE_VERSION);
                    cb(cbdata, LR_FMSTAGE_CACHELOADINGSTATUS,
                       "Old version of cache format");
                    something_wrong = TRUE;
                }
            }
        }

        if (something_wrong) {
            // Reinit keyfile
            g_key_file_free(keyfile);
            keyfile = g_key_file_new();
            (*cache)->keyfile = keyfile;
        } else {
            gsize len;
            gchar **array = g_key_file_get_groups(keyfile, &len);
            g_debug("%s: Loaded: %"G_GSIZE_FORMAT" records", __func__, len);

            // Remove really outdated records
            gint64 current_time = g_get_real_time() / 1000000;
            char **group, *groupname;
            for (group=array; groupname=*group, groupname; group++) {
                if (g_str_has_prefix(groupname, ":_"))
                    continue;

                gint64 ts = g_key_file_get_int64(keyfile,
                                                 groupname,
                                                 CACHE_KEY_TS,
                                                 NULL);
                if (ts < (current_time - CACHE_RECORD_MAX_AGE)) {
                    // Record is too old, remove it
                    g_debug("%s: Removing too old record from cache: %s "
                            "(ts: %"G_GINT64_FORMAT")",
                            __func__, groupname, ts);
                    g_key_file_remove_group(keyfile, groupname, NULL);
                }
            }
            g_strfreev(array);

            cb(cbdata, LR_FMSTAGE_CACHELOADINGSTATUS, NULL);
        }
    }

    // Set version of cache format
    g_key_file_set_integer(keyfile,
                           CACHE_GROUP_METADATA,
                           CACHE_KEY_VERSION,
                           CACHE_VERSION);

    return TRUE;
}

static gboolean
lr_fastestmirrorcache_lookup(LrFastestMirrorCache *cache,
                             gchar *url,
                             gint64 *ts,
                             double *connecttime)
{
    if (!cache || !cache->keyfile || !url)
        return FALSE;

    GKeyFile *keyfile = cache->keyfile;

    if (!g_key_file_has_group(keyfile, url))
        return FALSE;

    gint64 l_ts;
    double l_connecttime;
    GError *tmp_err = NULL;

    // Get timestamp
    l_ts = g_key_file_get_int64(keyfile, url, CACHE_KEY_TS, &tmp_err);
    if (tmp_err) {
        g_error_free(tmp_err);
        return FALSE;
    }

    // Get connect time
    l_connecttime = (double) g_key_file_get_double(keyfile,
                                                   url,
                                                   CACHE_KEY_CONNECTTIME,
                                                   &tmp_err);
    if (tmp_err) {
        g_error_free(tmp_err);
        return FALSE;
    }

    *ts = l_ts;
    *connecttime = l_connecttime;

    return TRUE;
}

static void
lr_fastestmirrorcache_update(LrFastestMirrorCache *cache,
                             gchar *url,
                             gint64 ts,
                             double connecttime)
{
    if (!cache || !cache->keyfile || !url)
        return;

    GKeyFile *keyfile = cache->keyfile;

    g_key_file_set_int64(keyfile, url, CACHE_KEY_TS, ts);
    g_key_file_set_double(keyfile, url, CACHE_KEY_CONNECTTIME, connecttime);
}

static gboolean
lr_fastestmirrorcache_write(LrFastestMirrorCache *cache, GError **err)
{
    assert(!err || *err == NULL);

    if (!cache || !cache->keyfile)
        return TRUE;

    // Gen cache content
    GError *tmp_err = NULL;
    _cleanup_free_ gchar *content =
                          g_key_file_to_data(cache->keyfile, NULL, &tmp_err);
    if (tmp_err) {
        g_propagate_error(err, tmp_err);
        return FALSE;
    }

    // Write cache
    FILE *f = fopen(cache->path, "w");
    if (!f) {
        g_set_error(err, LR_FASTESTMIRROR_ERROR, LRE_IO,
                    "Cannot open %s: %s", cache->path, g_strerror(errno));
        return FALSE;
    }

    fputs(content, f);
    fclose(f);

    return TRUE;
}

static void
lr_fastestmirrorcache_free(LrFastestMirrorCache *cache)
{
    if (!cache)
        return;

    g_free(cache->path);
    g_key_file_free(cache->keyfile);
    g_free(cache);
}

/** Create list of LrFastestMirror based on input list of URLs.
 */
static gboolean
lr_fastestmirror_prepare(LrHandle *handle,
                         GSList *in_list,
                         GSList **out_list,
                         LrFastestMirrorCache *cache,
                         GError **err)
{
    gboolean ret = TRUE;
    GSList *list = NULL;

    assert(!err || *err == NULL);

    if (!in_list) {
        *out_list = NULL;
        return TRUE;
    }

    gint64 maxage = LRO_FASTESTMIRRORMAXAGE_DEFAULT;
    gint64 current_time = g_get_real_time() / 1000000;

    if (handle)
        maxage = (gint64) handle->fastestmirrormaxage;

    for (GSList *elem = in_list; elem; elem = g_slist_next(elem)) {
        gchar *url = elem->data;
        CURLcode curlcode;
        CURL *curlh;

        // Try to find item in the cache
        gint64 ts;
        double connecttime;
        if (lr_fastestmirrorcache_lookup(cache, url, &ts, &connecttime)) {
            if (ts >= (current_time - maxage)) {
                // Use cached entry
                g_debug("%s: Using cached connect time for: %s (%f)",
                        __func__, url, connecttime);
                LrFastestMirror *mirror = lr_lrfastestmirror_new();
                mirror->url = url;
                mirror->curl = NULL;
                mirror->plain_connect_time = connecttime;
                mirror->cached = TRUE;
                list = g_slist_append(list, mirror);
                continue;
            } else {
                g_debug("%s: Cached connect time too old: %s", __func__, url);
            }
        } else {
            g_debug("%s: Not found in cache: %s", __func__, url);
        }

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

        curlcode = curl_easy_setopt(curlh, CURLOPT_URL, url);
        if (curlcode != CURLE_OK) {
            g_set_error(err, LR_FASTESTMIRROR_ERROR, LRE_CURL,
                        "curl_easy_setopt(_, CURLOPT_URL, %s) failed: %s",
                        url, curl_easy_strerror(curlcode));
            curl_easy_cleanup(curlh);
            ret = FALSE;
            break;
        }

        curlcode = curl_easy_setopt(curlh, CURLOPT_CONNECT_ONLY, 1);
        if (curlcode != CURLE_OK) {
            g_set_error(err, LR_FASTESTMIRROR_ERROR, LRE_CURL,
                    "curl_easy_setopt(_, CURLOPT_CONNECT_ONLY, 1) failed: %s",
                    curl_easy_strerror(curlcode));
            curl_easy_cleanup(curlh);
            ret = FALSE;
            break;
        }

        LrFastestMirror *mirror = lr_lrfastestmirror_new();
        mirror->url = url;
        mirror->curl = curlh;

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
lr_fastestmirror_perform(GSList *list,
                         gdouble length_of_measurement,
                         LrFastestMirrorCb cb,
                         void *cbdata,
                         GError **err)
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
    long handles_added = 0;
    for (GSList *elem = list; elem; elem = g_slist_next(elem)) {
        LrFastestMirror *mirror = elem->data;
        if (mirror->curl) {
            curl_multi_add_handle(multihandle, mirror->curl);
            handles_added++;
        }
    }

    if (handles_added == 0) {
        curl_multi_cleanup(multihandle);
        return TRUE;
    }

    cb(cbdata, LR_FMSTAGE_DETECTION, (void *) &handles_added);

    int still_running;
    gdouble elapsed_time = 0.0;
    _cleanup_timer_destroy_ GTimer *timer = g_timer_new();
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
            curl_multi_cleanup(multihandle);
            return FALSE;
        }

        rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
        if (rc < 0) {
            if (errno == EINTR) {
                g_debug("%s: select() interrupted by signal", __func__);
            } else {
                g_set_error(err, LR_FASTESTMIRROR_ERROR, LRE_SELECT,
                            "select() error: %s", g_strerror(errno));
                curl_multi_cleanup(multihandle);
                return FALSE;
            }
        }

        curl_multi_perform(multihandle, &still_running);

        // Break loop after some reasonable amount of time
        elapsed_time = g_timer_elapsed(timer, NULL);

    } while(still_running && elapsed_time < length_of_measurement);

    // Remove curl easy handles from multi handle
    // and calculate plain_connect_time
    for (GSList *elem = list; elem; elem = g_slist_next(elem)) {
        LrFastestMirror *mirror = elem->data;
        CURL *curl = mirror->curl;

        if (!curl)
            continue;

        // Remove handle
        curl_multi_remove_handle(multihandle, curl);

        // Calculate plain_connect_time
        char *effective_url;
        curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url);

        if (!effective_url) {
            // No effective url is most likely an error
            mirror->plain_connect_time = -1.0;
        } else if (g_str_has_prefix(effective_url, "file:")) {
            // Local directories are considered to be the best mirrors
            mirror->plain_connect_time = 0.0;
        } else {
            // Get connect time
            double namelookup_time;
            double connect_time;
            double plain_connect_time;
            curl_easy_getinfo(curl, CURLINFO_NAMELOOKUP_TIME, &namelookup_time);
            curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME, &connect_time);

            if (connect_time == 0.0) {
                // Zero connect time is most likely an error
                plain_connect_time = -1.0;
            } else {
                plain_connect_time = connect_time - namelookup_time;
            }

            mirror->plain_connect_time = plain_connect_time;
            //g_debug("%s: name_lookup: %3.6f connect_time:  %3.6f (%3.6f) | %s",
            //        __func__, namelookup_time, connect_time,
            //        mirror->plain_connect_time, mirror->url);
        }
    }

    curl_multi_cleanup(multihandle);
    return TRUE;
}

static void
null_cb(G_GNUC_UNUSED void *clientp,
        G_GNUC_UNUSED LrFastestMirrorStages stage,
        G_GNUC_UNUSED void *ptr)
{
    return;
}


static gint
cmp_fastestmirrors(gconstpointer a,
                   gconstpointer b)
{
    const LrFastestMirror *a_mirror = a;
    const LrFastestMirror *b_mirror = b;
    double a_ct = a_mirror->plain_connect_time;
    double b_ct = b_mirror->plain_connect_time;

    if (a_ct < 0.0 && b_ct < 0.0)
        return 0;
    if (a_ct < 0.0)
        return 1;
    if (b_ct < 0.0)
        return -1;

    if (a_ct < b_ct)
        return -1;
    else if (a_ct == b_ct)
        return 0;
    else
        return 1;
}


gboolean
lr_fastestmirror_detailed(LrHandle *handle,
                          GSList *inlist,
                          GSList **outlist,
                          GError **err)
{
    assert(!err || *err == NULL);

    char *fastestmirrorcache = NULL;
    gdouble length_of_measurement = LENGTH_OF_MEASUREMENT;
    LrFastestMirrorCb cb = null_cb;
    void *cbdata = NULL;

    if (handle) {
        fastestmirrorcache = handle->fastestmirrorcache;
        if (handle->fastestmirrorcb)
            cb = handle->fastestmirrorcb;
        cbdata = handle->fastestmirrordata;
        length_of_measurement = handle->fastestmirrortimeout;

        if (handle->offline) {
            g_debug("%s: Fastest mirror determination "
                    "skipped... LRO_OFFLINE enabled", __func__);
            return TRUE;
        }
    }

    g_debug("%s: Fastest mirror determination in progress...", __func__);
    cb(cbdata, LR_FMSTAGE_INIT, NULL);

    if (!inlist) {
        cb(cbdata, LR_FMSTAGE_STATUS, NULL);
        return TRUE;
    }

    // Load cache
    gboolean ret;
    LrFastestMirrorCache *cache = NULL;
    ret = lr_fastestmirrorcache_load(&cache,
                                     fastestmirrorcache,
                                     cb,
                                     cbdata,
                                     err);
    if (!ret) {
        cb(cbdata, LR_FMSTAGE_STATUS, "Cannot load cache");
        return FALSE;
    }

    // Prepare list of LrFastestMirror elements
    GSList *lrfastestmirrors;
    ret = lr_fastestmirror_prepare(handle, inlist, &lrfastestmirrors, cache, err);
    if (!ret) {
        cb(cbdata, LR_FMSTAGE_STATUS, "Error while lr_fastestmirror_prepare()");
        g_debug("%s: Error while lr_fastestmirror_prepare()", __func__);
        lr_fastestmirrorcache_free(cache);
        return FALSE;
    }

    ret = lr_fastestmirror_perform(lrfastestmirrors,
                                   length_of_measurement,
                                   cb,
                                   cbdata,
                                   err);
    if (!ret) {
        cb(cbdata, LR_FMSTAGE_STATUS, "Error while detection");
        g_debug("%s: Error while lr_fastestmirror_perform()", __func__);
        g_slist_free_full(lrfastestmirrors,
                          (GDestroyNotify)lr_lrfastestmirror_free);
        lr_fastestmirrorcache_free(cache);
        return FALSE;
    }

    cb(cbdata, LR_FMSTAGE_FINISHING, NULL);

    // Sort the mirrors by the connection time
    lrfastestmirrors = g_slist_sort(lrfastestmirrors, cmp_fastestmirrors);

    // Update cache
    gint64 ts = g_get_real_time() / 1000000; // TimeStamp
    for (GSList *elem = lrfastestmirrors; elem; elem = g_slist_next(elem)) {
        LrFastestMirror *mirror = elem->data;
        if (mirror->cached == FALSE) {
            lr_fastestmirrorcache_update(cache,
                                         mirror->url,
                                         ts,
                                         mirror->plain_connect_time);
        }
    }

    lr_fastestmirrorcache_write(cache, NULL);
    lr_fastestmirrorcache_free(cache);

    *outlist = lrfastestmirrors;

    cb(cbdata, LR_FMSTAGE_STATUS, NULL);

    return TRUE;
}


gboolean
lr_fastestmirror(LrHandle *handle,
                 GSList **list,
                 GError **err)
{
    GSList *lrfastestmirrors = NULL;
    GSList *new_list = NULL;

    assert(!err || *err == NULL);

    // Prepare list of LrFastestMirror elements
    gboolean ret = lr_fastestmirror_detailed(handle, *list, &lrfastestmirrors, err);
    if (!ret) {
        g_slist_free_full(lrfastestmirrors,
                          (GDestroyNotify)lr_lrfastestmirror_free);
        return FALSE;
    }

    // Sort the mirrors by the connection time
    //
    // Note that always picking the single best mirror has undesirable properties like
    // forcing the user to always pick the single lowest latency mirror regardless of its
    // actual bandwidth performance, and high density install bases all using fastestmirror
    // tend to dogpile the single mirror nearest to all of them
    //
    // Instead of using a strict sorted list, shuffle all mirrors with lower latency than 2x
    // the best mirror, to introduce enough entropy to spread the load across nearby mirrors.
    double bestMirrorLatency = 0;
    if (lrfastestmirrors != NULL) {
        bestMirrorLatency = ((LrFastestMirror *)lrfastestmirrors->data)->plain_connect_time;
    }

    for (GSList *elem = lrfastestmirrors; elem; elem = g_slist_next(elem)) {
        LrFastestMirror *mirror = elem->data;
        g_debug("%s: %3.6f : %s", __func__, mirror->plain_connect_time, mirror->url);
        if (mirror->plain_connect_time >= 0 &&
            mirror->plain_connect_time < (2.0 * bestMirrorLatency)) { // Shuffle nearby mirrors
            new_list = g_slist_insert(new_list, mirror->url, g_random_int_range(0, g_slist_length(new_list)+1));
        } else { // Far away mirrors appended as backup options
            new_list = g_slist_append(new_list, mirror->url);
        }
    }

    g_slist_free_full(lrfastestmirrors, (GDestroyNotify)lr_lrfastestmirror_free);
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

    _cleanup_timer_destroy_ GTimer *timer = g_timer_new();
    g_timer_start(timer);

    LrHandle *main_handle = handles->data;  // Network configuration for the
                                            // test is used from the first
                                            // handle

    // Prepare list of hosts
    gchar *fastestmirrorcache = main_handle->fastestmirrorcache;
    _cleanup_hashtable_unref_ GHashTable *hosts_ht =
                                          g_hash_table_new_full(g_str_hash,
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

        // Cache related warning
        if (fastestmirrorcache) {
            if (handle->fastestmirrorcache
                && g_strcmp0(fastestmirrorcache, handle->fastestmirrorcache))
                g_warning("%s: Multiple fastestmirror caches are specified! "
                          "Used one is %s (%s is ignored)", __func__,
                          fastestmirrorcache, handle->fastestmirrorcache);
        } else {
            if (handle->fastestmirrorcache)
                g_warning("%s: First handle doesn't have a fastestmirror "
                          "cache specified but other one has: %s",
                          __func__, handle->fastestmirrorcache);
        }
    }

    _cleanup_list_free_ GList *tmp_list_of_urls = g_hash_table_get_keys(hosts_ht);
    _cleanup_slist_free_ GSList *list_of_urls = NULL;
    int number_of_mirrors = 0;
    for (GList *elem = tmp_list_of_urls; elem; elem = g_list_next(elem)) {
        list_of_urls = g_slist_prepend(list_of_urls, elem->data);
        number_of_mirrors++;
    }

    if (number_of_mirrors <= 1) {
        // Nothing to do
        return TRUE;
    }

    // Sort this list by the connection time
    gboolean ret = lr_fastestmirror(main_handle,
                                    &list_of_urls,
                                    err);
    if (!ret) {
        g_debug("%s: lr_fastestmirror failed", __func__);
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
                _cleanup_free_ gchar *im_host = lr_url_without_path(im->url);
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
        // The remaining occurrences will be moved here.
        for (GSList *elem = mirrors; elem; elem = g_slist_next(elem)) {
            LrInternalMirror *im = elem->data;
            new_list = g_slist_prepend(new_list, im);
        }
        g_slist_free(mirrors);

        // Set sorted list to the handle (reversed, because the items
        // of the new_list were prepended)
        handle->internal_mirrorlist = g_slist_reverse(new_list);
    }

    g_timer_stop(timer);
    g_debug("%s: Duration: %f", __func__, g_timer_elapsed(timer, NULL));

    return TRUE;
}
