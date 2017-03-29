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

#define _XOPEN_SOURCE   500 // Because of fdopen() and ftruncate()

#include <glib.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <curl/curl.h>
#include <attr/xattr.h>

#include "downloader.h"
#include "rcodes.h"
#include "util.h"
#include "downloadtarget.h"
#include "downloadtarget_internal.h"
#include "handle.h"
#include "handle_internal.h"
#include "cleanup.h"
#include "url_substitution.h"

volatile sig_atomic_t lr_interrupt = 0;

void
lr_sigint_handler(G_GNUC_UNUSED int sig)
{
    lr_interrupt = 1;
}

typedef enum {
    LR_DS_WAITING, /*!<
        The target is waiting to be processed. */
    LR_DS_RUNNING, /*!<
        The transfer is running. */
    LR_DS_FINISHED, /*!<
        The transfer is successfully finished. */
    LR_DS_FAILED, /*!<
        The transfer is finished without success. */
} LrDownloadState;

typedef enum {
    LR_HCS_DEFAULT, /*!<
        Default state */
    LR_HCS_HTTP_STATE_OK, /*!<
        HTTP headers with OK state */
    LR_HCS_INTERRUPTED, /*!<
        Download was interrupted (e.g. Content-Length doesn't match
        expected size etc.) */
    LR_HCS_DONE, /*!<
        All headers which we were looking for are already found*/
} LrHeaderCbState;

typedef struct {
    LrHandle *handle; /*!<
        Handle (could be NULL) */
    GSList *lrmirrors; /*!<
        List of LrMirrors created from the handle internal mirrorlist
        (could be NULL) */
} LrHandleMirrors;

typedef struct {
    LrInternalMirror *mirror; /*!<
        Mirror */
    int allowed_parallel_connections; /*!<
        Maximum number of allowed parallel connections to this mirror. -1 means no limit.
        Dynamicaly adjusted (decreased) if no fatal (temporary) error will occur. */
    int max_tried_parallel_connections; /*!<
        The maximum number of tried parallel connections to this mirror
        (including unsuccessful). */
    int running_transfers; /*!<
        How many transfers from this mirror are currently in progres. */
    int successful_transfers; /*!<
        How many transfers was finished successfully from the mirror. */
    int failed_transfers; /*!<
        How many transfers failed. */
} LrMirror;

typedef struct {
    LrDownloadState state; /*!<
        State of the download (transfer). */
    LrDownloadTarget *target; /*!<
        Download target */
    LrMirror *mirror; /*!<
        Mirror is:
        If a base_location is used then NULL.
        If state is LR_DS_WAITING then could be NULL (if no download
        was tried yet) or notNULL if previous download failed, in that case
        the mirror is the last used mirror (the one that failed).
        If state is LR_DS_RUNNING then currently used mirror.
        If state is LR_DS_FINISHED then mirror from which download was
        successfully performed.
        If state is LR_DS_FAILED then mirror from which last try
        was done. */
    LrProtocol protocol; /*!<
        Current protocol */
    CURL *curl_handle; /*!<
        Used curl handle or NULL */
    FILE *f; /*!<
        fdopened file descriptor from LrDownloadTarget and used
        in curl_handle. */
    char errorbuffer[CURL_ERROR_SIZE]; /*!<
        Error buffer used in curl handle */
    GSList *tried_mirrors; /*!<
        List of already tried mirrors (LrMirror *).
        This mirrors won't be tried again. */
    gboolean resume; /*!<
        Is resume enabled? Download target may state that resume is True
        but Librepo can decide that resuming won't be done.
        This variable states if the resume is enabled or not. */
    gint64 original_offset; /*!<
        If resume is enabled, this is the specified offset where to resume
        the downloading. If resume is not enabled, then value is -1. */
    gint resume_count; /*!<
        How many resumes were done */
    GSList *lrmirrors; /*!<
        List of all available mirors (LrMirror *).
        This list is generated from LrHandle related to this target
        and is common for all targets that uses the handle. */
    LrHandle *handle; /*!<
        LrHandle associated with this target */
    LrHeaderCbState headercb_state; /*!<
        State of the header callback for current transfer */
    gchar *headercb_interrupt_reason; /*!<
        Reason why was the transfer interrupted */
    gint64 writecb_recieved; /*!<
        Total number of bytes received by the write function
        during the current transfer. */
    gboolean writecb_required_range_written; /*!<
        If a byte range was specified to download and the
        range was downloaded, it is TRUE. Otherwise FALSE. */
    LrCbReturnCode cb_return_code; /*!<
        Last cb return code. */
    struct curl_slist *curl_rqheaders; /*!<
        Extra headers for request. */
} LrTarget;

typedef struct {

    // Configuration

    gboolean failfast; /*!<
        Fail fast */

    int max_parallel_connections; /*!<
        Maximal number of parallel downloads. */

    int max_connection_per_host; /*!<
        Maximal number of connections per host. -1 means no limit. */

    int max_mirrors_to_try; /*!<
        Maximal number of mirrors to try. Number <= 0 means no limit. */

    gint64 max_speed; /*!<
        Maximal speed in bytes per sec */

    long allowed_mirror_failures; /*!<
        See LRO_ALLOWEDMIRRORFAILURES */

    long adaptivemirrorsorting; /*!<
        See LRO_ADAPTIVEMIRRORSORTING */

    // Data

    CURLM *multi_handle; /*!<
        Curl Multi handle */

    GSList *handle_mirrors; /*!<
        All mirrors (list of pointers to LrHandleMirrors structures) */

    GSList *targets; /*!<
        List of all targets (list of pointers to LrTarget stuctures) */

    GSList *running_transfers; /*!<
        List of running transfers (list of pointer to LrTarget structures) */

} LrDownload;

/** Schema of structures as used in downloader module:
 *
 * +------------------------------+
 * |           LrDownload         |
 * +------------------------------+
 * | int max_parallel_connections |
 * | int max_connection_par_host  |
 * | int max_mirrors_to_try       |      +-------------------+
 * |                              |   /->|  LrHandleMirrors  |
 * |                              |  |   +-------------------+
 * | CURLM *multi_handle          |  |   | LrHandle *handle  |
 * |                              |  |   | GSList *lrmirrors --\
 * | GSList *handle_mirrors      ---/    +-------------------+ |
 * | GSList *targets             --\                           |
 * | GSList *running_transfers   ---\                          |
 * +------------------------------+  |                         |
 *                                   |                         |
 *   /------------------------------/                          |
 *  |                                                          |
 *  |                         /--------------------------------/
 *  |                        \/
 *  |          +------------------------------------+
 *  |          |              LrMirror              |
 *  |        +------------------------------------+-|
 *  |        |              LrMirror              | |
 *  |      +------------------------------------+-| |
 *  |      |              LrMirror              | | |
 *  |      +------------------------------------+ | |    +---------------------+
 *  |      | LrInternalMirror *mirror ------------------>|   LrInternalMirror  |
 *  |      | int allowed_parallel_connections   | |      +---------------------+
 *  |      | int max_tried_parallel_connections |-+      | char *url           |
 *  |      | int running_transfersers           |        | int preference      |
 *  |      | int successful_transfers           |        | LrProtocol protocol |
 *  |      + int failed_transfers               +<--\    +---------------------+
 *  |      +------------------------------------+   |
 *  |                                               |
 *  |        +----------------------------+   /-----/
 *  |        |          LrTarget          |   |
 *  |      +----------------------------+-|   |
 *  |      |          LrTarget          | |   |     +--------------------------+
 *  |    +----------------------------+-| |   |  /->|      LrDownloadTarget    |
 *   \-> |          LrTarget          | | |   |  |  +--------------------------+
 *       +----------------------------+ | |   |  |  | char *path               |
 *       | LrDownloadState state      | | |   |  |  | char *baseurl            |
 *       | LrDownloadTarget *target  -----------/   | int fd                   |
 *       | LrMirror *mirror          --------/      | LrChecksumType checks..  |
 *       | CURL *curl_handle          |-+           | char *checksum           |
 *       | FILE *f                    |             | int resume               |
 *       | GSList *tried_mirrors      |             | LrProgressCb progresscb  |
 *       | gint64 original_offset     |             | void *cbdata             |
 *       | GSlist *lrmirrors         ---\           | GStringChunk *chunk      |
 *       +----------------------------+  |          | int rcode                |
 *                                       |          | char *err                |
 *      Points to list of LrMirrors <---/           +--------------------------+
 */

static gboolean
is_max_mirrors_unlimited(const LrDownload *download)
{
    return download->max_mirrors_to_try <= 0;
}

static gboolean
can_try_more_mirrors(const LrDownload *download, int num_of_tried_mirrors)
{
    return is_max_mirrors_unlimited(download) ||
           num_of_tried_mirrors < download->max_mirrors_to_try;
}

static gboolean
has_running_transfers(const LrMirror *mirror)
{
    return mirror->running_transfers > 0;
}

static void
init_once_allowed_parallel_connections(LrMirror *mirror, int max_allowed_parallel_connections)
{
    if (mirror->allowed_parallel_connections == 0) {
        mirror->allowed_parallel_connections = max_allowed_parallel_connections;
    }
}

static void
increase_running_transfers(LrMirror *mirror)
{
    mirror->running_transfers++;
    if (mirror->max_tried_parallel_connections < mirror->running_transfers)
        mirror->max_tried_parallel_connections = mirror->running_transfers;
}

static gboolean
is_parallel_connections_limited_and_reached(const LrMirror *mirror)
{
    return mirror->allowed_parallel_connections != -1 &&
           mirror->running_transfers >= mirror->allowed_parallel_connections;
}

static void
mirror_update_statistics(LrMirror *mirror, gboolean transfer_success)
{
    mirror->running_transfers--;
    if (transfer_success)
        mirror->successful_transfers++;
    else
        mirror->failed_transfers++;
}

/** Create GSList of LrMirrors (if it doesn't exist) for a handle.
 * If the list already exists (if more targets use the same handle)
 * then just set the list to the current target.
 * If the list doesn't exist yet, create it then create a mapping between
 * the list and the handle (LrHandleMirrors) and set the list to
 * the current target.
 */
static GSList *
lr_prepare_lrmirrors(GSList *list, LrTarget *target)
{
    LrHandle *handle = target->handle;

    for (GSList *elem = list; elem; elem = g_slist_next(elem)) {
        LrHandleMirrors *handle_mirrors = elem->data;
        if (handle_mirrors->handle == handle) {
            // List of LrMirrors for this handle is already created
            target->lrmirrors = handle_mirrors->lrmirrors;
            return list;
        }
    }

    GSList *lrmirrors = NULL;

    if (handle && handle->internal_mirrorlist) {
        g_debug("%s: Preparing internal mirror list for handle id: %p", __func__, handle);
        for (GSList *elem = handle->internal_mirrorlist;
             elem;
             elem = g_slist_next(elem))
        {
            LrInternalMirror *imirror = elem->data;

            assert(imirror);
            assert(imirror->url);

            if (!imirror || !imirror->url || !strlen(imirror->url))
                continue;

            g_debug("%s: Mirror: %s", __func__, imirror->url);

            LrMirror *mirror = lr_malloc0(sizeof(*mirror));
            mirror->mirror = imirror;
            lrmirrors = g_slist_append(lrmirrors, mirror);
        }
    }

    LrHandleMirrors *handle_mirrors = lr_malloc0(sizeof(*handle_mirrors));
    handle_mirrors->handle = handle;
    handle_mirrors->lrmirrors = lrmirrors;

    target->lrmirrors = lrmirrors;
    list = g_slist_append(list, handle_mirrors);

    return list;
}


/** Progress callback for CURL handles.
 * progress callback set by the user of librepo.
 */
static int
lr_progresscb(void *ptr,
              double total_to_download,
              double now_downloaded,
              G_GNUC_UNUSED double total_to_upload,
              G_GNUC_UNUSED double now_uploaded)
{
    int ret = LR_CB_OK;
    LrTarget *target = ptr;

    assert(target);
    assert(target->target);

    if (target->state != LR_DS_RUNNING)
        return ret;
    if (!target->target->progresscb)
        return ret;

    ret = target->target->progresscb(target->target->cbdata,
                                     total_to_download,
                                     now_downloaded);

    target->cb_return_code = ret;

    return ret;
}

#define STRLEN(s) (sizeof(s)/sizeof(s[0]) - 1)


/** Header callback for CURL handles.
 * It parses HTTP and FTP headers and try to find length of the content
 * (file size of the target). If the size is different then the expected
 * size, then the transfer is interrupted.
 * This callback is used only if the expected size is specified.
 */
static size_t
lr_headercb(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    assert(userdata);

    size_t ret = size * nmemb;
    LrTarget *lrtarget = userdata;
    LrHeaderCbState state = lrtarget->headercb_state;

    if (state == LR_HCS_DONE || state == LR_HCS_INTERRUPTED) {
        // Nothing to do
        return ret;
    }

    char *header = g_strstrip(g_strndup(ptr, size*nmemb));
    gint64 expected = lrtarget->target->expectedsize;

    if (state == LR_HCS_DEFAULT) {
        if (lrtarget->protocol == LR_PROTOCOL_HTTP
            && g_str_has_prefix(header, "HTTP/")) {
            // Header of a HTTP protocol
            if (g_strrstr(header, "200") && !(
                            g_strrstr(header, "connection established") ||
                            g_strrstr(header, "Connection established") ||
                            g_strrstr(header, "Connection Established")
                        )) {
                lrtarget->headercb_state = LR_HCS_HTTP_STATE_OK;
            } else {
                // Do nothing (do not change the state)
                // in case of redirection, 200 OK still could come
                g_debug("%s: Non OK HTTP header status: %s", __func__, header);
            }
        } else if (lrtarget->protocol == LR_PROTOCOL_FTP) {
            // Headers of a FTP protocol
            if (g_str_has_prefix(header, "213 ")) {
                // Code 213 shoud keep the file size
                gint64 content_length = g_ascii_strtoll(header+4, NULL, 0);

                g_debug("%s: Server returned size: \"%s\" "
                        "(converted %"G_GINT64_FORMAT"/%"G_GINT64_FORMAT
                        " expected)",
                        __func__, header+4, content_length, expected);

                // Compare expected size and size reported by a FTP server
                if (content_length > 0 && content_length != expected) {
                    g_debug("%s: Size doesn't match (%"G_GINT64_FORMAT
                            " != %"G_GINT64_FORMAT")",
                            __func__, content_length, expected);
                    lrtarget->headercb_state = LR_HCS_INTERRUPTED;
                    lrtarget->headercb_interrupt_reason = g_strdup_printf(
                        "FTP server reports size: %"G_GINT64_FORMAT" "
                        "via 213 code, but expected size is: %"G_GINT64_FORMAT,
                        content_length, expected);
                    ret++;  // Return error value
                } else {
                    lrtarget->headercb_state = LR_HCS_DONE;
                }
            } else if (g_str_has_prefix(header, "150")) {
                // Code 150 shoud keep the file size
                // TODO: See parse150 in /usr/lib64/python2.7/ftplib.py
            }
        }
    }

    if (state == LR_HCS_HTTP_STATE_OK) {
        if (g_str_has_prefix(header, "Content-Length: ")) {
            // Content-Length header found
            char *content_length_str = header + STRLEN("Content-Length: ");
            gint64 content_length = g_ascii_strtoll(content_length_str,
                                                    NULL, 0);
            g_debug("%s: Server returned Content-Length: \"%s\" "
                    "(converted %"G_GINT64_FORMAT"/%"G_GINT64_FORMAT" expected)",
                    __func__, content_length_str, content_length, expected);

            // Compare expected size and size reported by a HTTP server
            if (content_length > 0 && content_length != expected) {
                g_debug("%s: Size doesn't match (%"G_GINT64_FORMAT
                        " != %"G_GINT64_FORMAT")",
                        __func__, content_length, expected);
                lrtarget->headercb_state = LR_HCS_INTERRUPTED;
                lrtarget->headercb_interrupt_reason = g_strdup_printf(
                    "Server reports Content-Length: %"G_GINT64_FORMAT" but "
                    "expected size is: %"G_GINT64_FORMAT,
                    content_length, expected);
                ret++;  // Return error value
            } else {
                lrtarget->headercb_state = LR_HCS_DONE;
            }
        }
    }

    g_free(header);

    return ret;
}


/** Write callback for CURL handles.
 * This callback handles situation when an user wants only specified
 * byte range of the target file.
 */
size_t
lr_writecb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t cur_written_expected = nmemb;
    size_t cur_written;
    LrTarget *target = (LrTarget *) userdata;
    gint64 all = size * nmemb;  // Total number of bytes from curl
    gint64 range_start = target->target->byterangestart;
    gint64 range_end = target->target->byterangeend;

    if (range_start <= 0 && range_end <= 0) {
        // Write everything curl give to you
        target->writecb_recieved += all;
        return fwrite(ptr, size, nmemb, target->f);
    }

    /* Deal with situation when user wants only specific byte range of the
     * target file, and write only the range.
     */

    gint64 cur_range_start = target->writecb_recieved;
    gint64 cur_range_end = cur_range_start + all;

    target->writecb_recieved += all;

    if (target->target->byterangestart > 0) {
        // If byterangestart is specified, then CURLOPT_RESUME_FROM_LARGE
        // is used by default
        cur_range_start += target->target->byterangestart;
        cur_range_end   += target->target->byterangestart;
    } else if (target->original_offset > 0) {
        cur_range_start += target->original_offset;
        cur_range_end   += target->original_offset;
    }

    if (cur_range_end < range_start)
        // The wanted byte range doesn't start yet
        return nmemb;

    if (range_end != 0 && cur_range_start > range_end) {
        // The wanted byte range is over
        // Return zero that will lead to transfer abortion
        // with error code CURLE_WRITE_ERROR
        target->writecb_required_range_written = TRUE;
        return 0;
    }

    size = 1;
    nmemb = all;

    if (cur_range_start >= range_start) {
        // Write the current curl passed range from the start
        ;
    } else {
        // Find the right starting offset
        gint64 offset = range_start - cur_range_start;
        assert(offset > 0);
        ptr += offset;
        // Corret the length appropriately
        nmemb = all - offset;
    }

    if (range_end != 0) {
        // End range is specified

        if (cur_range_end <= range_end) {
            // Write the current curl passed range to the end
            ;
        } else {
            // Find the length of the new sequence
            gint64 offset = cur_range_end - range_end;
            assert(offset > 0);
            // Corret the length appropriately
            nmemb -= (offset - 1);
        }
    }

    assert(nmemb > 0);
    cur_written = fwrite(ptr, size, nmemb, target->f);
    if (cur_written != nmemb) {
        g_debug("%s: Error while writting out file: %s",
                __func__, g_strerror(errno));
        return 0; // There was an error
    }

    return cur_written_expected;
}

/** Select a suitable mirror
 */
static gboolean
select_suitable_mirror(LrDownload *dd,
                       LrTarget *target,
                       LrMirror **selected_mirror,
                       GError **err)
{

    gboolean at_least_one_suitable_mirror_found = FALSE;
    //  ^^^ This variable is used to indentify that all possible mirrors
    // were already tried and the transfer shoud be marked as failed.

    assert(dd);
    assert(target);
    assert(selected_mirror);
    assert(!err || *err == NULL);

    *selected_mirror = NULL;

    // Iterate over mirror for the target
    for (GSList *elem = target->lrmirrors; elem; elem = g_slist_next(elem)) {
        LrMirror *c_mirror = elem->data;
        gchar *mirrorurl = c_mirror->mirror->url; // shortcut

        if (g_slist_find(target->tried_mirrors, c_mirror)) {
            // This mirror was already tried for this target
            continue;
        }

        if (c_mirror->mirror->protocol == LR_PROTOCOL_RSYNC) {
            // Skip rsync mirrors
            g_debug("%s: Skipping rsync url: %s", __func__, mirrorurl);
            continue;
        }

        if (target->handle
            && target->handle->offline
            && c_mirror->mirror->protocol != LR_PROTOCOL_FILE)
        {
            // Skip each url that doesn't have "file://" or "file:" prefix
            g_debug("%s: Skipping mirror %s - Offline mode enabled",
                    __func__, mirrorurl);
            continue;
        }

        if (c_mirror->successful_transfers == 0 &&
            dd->allowed_mirror_failures > 0 &&
            c_mirror->failed_transfers >= dd->allowed_mirror_failures)
        {
            // Skip bad mirrors
            g_debug("%s: Skipping bad mirror (%d failures and no success): %s",
                    __func__, c_mirror->failed_transfers, mirrorurl);
            continue;
        }

        at_least_one_suitable_mirror_found = TRUE;

        // Number of transfers which are downloading from the mirror
        // should always be lower or equal than maximum allowed number
        // of connection to a single host.
        assert(dd->max_connection_per_host == -1 ||
               c_mirror->running_transfers <= dd->max_connection_per_host);

        // Init max of allowed parallel connections from config
        init_once_allowed_parallel_connections(c_mirror, dd->max_connection_per_host);

        // Check number of connections to the mirror
        if (is_parallel_connections_limited_and_reached(c_mirror))
        {
            continue;
        }

        // This mirror looks suitable - use it
        *selected_mirror = c_mirror;
        return TRUE;
    }

    if (!at_least_one_suitable_mirror_found) {
        // No suitable mirror even exists => Set transfer as failed
        g_debug("%s: All mirrors were tried without success", __func__);
        target->state = LR_DS_FAILED;

        lr_downloadtarget_set_error(target->target, LRE_NOURL,
                    "Cannot download, all mirrors were already tried "
                    "without success");


        // Call end callback
        LrEndCb end_cb =  target->target->endcb;
        if (end_cb) {
            int ret = end_cb(target->target->cbdata,
                             LR_TRANSFER_ERROR,
                             "No more mirrors to try - All mirrors "
                             "were already tried without success");
            if (ret == LR_CB_ERROR) {
                target->cb_return_code = LR_CB_ERROR;
                g_debug("%s: Downloading was aborted by LR_CB_ERROR "
                        "from end callback", __func__);
                g_set_error(err, LR_DOWNLOADER_ERROR, LRE_CBINTERRUPTED,
                        "Interupted by LR_CB_ERROR from end callback");
                return FALSE;
            }
        }

        if (dd->failfast) {
            // Fail immediately
            g_set_error(err, LR_DOWNLOADER_ERROR, LRE_NOURL,
                        "Cannot download %s: All mirrors were tried",
                        target->target->path);
            return FALSE;
        }
    }

    return TRUE;
}


/** Select next target
 */
static gboolean
select_next_target(LrDownload *dd,
                   LrTarget **selected_target,
                   char **selected_full_url,
                   GError **err)
{
    assert(dd);
    assert(selected_target);
    assert(selected_full_url);
    assert(!err || *err == NULL);

    *selected_target = NULL;
    *selected_full_url = NULL;

    for (GSList *elem = dd->targets; elem; elem = g_slist_next(elem)) {
        LrTarget *target = elem->data;
        LrMirror *mirror = NULL;
        char *full_url = NULL;
        int complete_url_in_path = 0;

        if (target->state != LR_DS_WAITING)  // Pick only waiting targets
            continue;

        // Determine if path is a complete URL

        complete_url_in_path = strstr(target->target->path, "://") ? 1 : 0;

        // Sanity check

        if (!target->target->baseurl
            && !target->lrmirrors
            && !complete_url_in_path)
        {
            // Used relative path with empty internal mirrorlist
            // and no basepath specified!
            g_debug("%s: Empty mirrorlist and no basepath specified", __func__);
            g_set_error(err, LR_DOWNLOADER_ERROR, LRE_NOURL,
                        "Empty mirrorlist and no basepath specified!");
            return FALSE;
        }

        g_debug("%s: Selecting mirror for: %s", __func__, target->target->path);

        // Prepare full target URL

        if (complete_url_in_path) {
            // Path is a complete URL (do not use mirror nor base URL)
            full_url = g_strdup(target->target->path);
        } else if (target->target->baseurl) {
            // Base URL is specified
            full_url = lr_pathconcat(target->target->baseurl,
                                     target->target->path,
                                     NULL);
        } else {
            // Find a suitable mirror
            if (!select_suitable_mirror(dd, target, &mirror , err))
                return FALSE;

            if (mirror) {
                // A mirror was found
                full_url = lr_pathconcat(mirror->mirror->url,
                                         target->target->path,
                                         NULL);
            } else {
                // No free mirror
                g_debug("%s: Currently there is no free mirror for: %s",
                        __func__, target->target->path);
            }
        }

        // If LRO_OFFLINE is specified, check if the obtained full_url
        // is local or not
        // This condition should never be true for a full_url built
        // from a mirror, because select_suitable_mirror() checks if
        // the URL is local if LRO_OFFLINE is enabled by itself.
        if (full_url
            && target->handle
            && target->handle->offline
            && !lr_is_local_path(full_url))
        {
            g_debug("%s: Skipping %s because LRO_OFFLINE is specified",
                    __func__, full_url);

            // Mark the target as failed
            target->state = LR_DS_FAILED;
            lr_downloadtarget_set_error(target->target, LRE_NOURL,
                    "Cannot download, offline mode is specified and no "
                    "local URL is available");

            // Call end callback
            LrEndCb end_cb =  target->target->endcb;
            if (end_cb) {
                int ret = end_cb(target->target->cbdata,
                                 LR_TRANSFER_ERROR,
                                "Cannot download: Offline mode is specified "
                                "and no local URL is available");
                if (ret == LR_CB_ERROR) {
                    target->cb_return_code = LR_CB_ERROR;
                    g_debug("%s: Downloading was aborted by LR_CB_ERROR "
                            "from end callback", __func__);
                    g_set_error(err, LR_DOWNLOADER_ERROR, LRE_CBINTERRUPTED,
                            "Interupted by LR_CB_ERROR from end callback");
                    return FALSE;
                }
            }

            if (dd->failfast) {
                // Fail immediately
                g_set_error(err, LR_DOWNLOADER_ERROR, LRE_NOURL,
                            "Cannot download %s: Offline mode is specified "
                            "and no local URL is available",
                            target->target->path);
                return FALSE;
            }
        }

        if (full_url) {  // A waiting target found
            target->mirror = mirror;  // Note: mirror is NULL if baseurl is used

            *selected_target = target;
            *selected_full_url = full_url;

            return TRUE;
        }
    }

    // No suitable target found
    return TRUE;
}


#define XATTR_LIBREPO   "user.Librepo.DownloadInProgress"

/** Add an extendend attribute that indiciates that
 * the file is being/was downloaded by Librepo
 */
static void
add_librepo_xattr(int fd, const gchar *fn)
{
    _cleanup_free_ gchar *dst = NULL;

    if (!fn)
        dst = g_strdup_printf("fd: %d", fd);
    else
        dst = g_strdup(fn);

    int attr_ret = fsetxattr(fd, XATTR_LIBREPO, "", 1, 0);
    if (attr_ret == -1) {
        g_debug("%s: Cannot set xattr %s (%s): %s",
                __func__, XATTR_LIBREPO, dst, g_strerror(errno));
    }
}


/** Check if the file was downloaded by Librepo
 */
static gboolean
has_librepo_xattr(int fd)
{
    ssize_t attr_ret = fgetxattr(fd, XATTR_LIBREPO, NULL, 0);
    if (attr_ret == -1) {
        //g_debug("%s: Cannot get xattr %s: %s",
        //        __func__, XATTR_LIBREPO, g_strerror(errno));
        return FALSE;
    }
    return TRUE;
}


/** Remove Librepo extended attribute
 */
static void
remove_librepo_xattr(int fd)
{
    fremovexattr(fd, XATTR_LIBREPO);
}


/** Prepares next transfer
 */
static gboolean
prepare_next_transfer(LrDownload *dd, gboolean *candidatefound, GError **err)
{
    LrTarget *target;
    char *full_url = NULL;
    LrProtocol protocol = LR_PROTOCOL_OTHER;
    gboolean ret;

    assert(dd);
    assert(!err || *err == NULL);

    *candidatefound = FALSE;

    ret = select_next_target(dd, &target, &full_url, err);
    if (!ret)  // Error
        return FALSE;

    if (!target)  // Nothing to do
        return TRUE;

    *candidatefound = TRUE;

    g_debug("%s: URL: %s", __func__, full_url);

    protocol = lr_detect_protocol(full_url);

    // Prepare CURL easy handle
    CURLcode c_rc;
    CURL *h;
    if (target->handle)
        h = curl_easy_duphandle(target->handle->curl_handle);
    else
        h = lr_get_curl_handle();
    if (!h) {
        // Something went wrong
        g_set_error(err, LR_DOWNLOADER_ERROR, LRE_CURL,
                    "curl_easy_duphandle() call failed");
        return FALSE;
    }

    // Set URL
    c_rc = curl_easy_setopt(h, CURLOPT_URL, full_url);
    if (c_rc != CURLE_OK) {
        g_set_error(err, LR_DOWNLOADER_ERROR, LRE_CURL,
                    "curl_easy_setopt(h, CURLOPT_URL, %s) failed: %s",
                    full_url, curl_easy_strerror(c_rc));
        lr_free(full_url);
        curl_easy_cleanup(h);
        return FALSE;
    }

    // Set error buffer
    target->errorbuffer[0] = '\0';
    c_rc = curl_easy_setopt(h, CURLOPT_ERRORBUFFER, target->errorbuffer);
    if (c_rc != CURLE_OK) {
        g_set_error(err, LR_DOWNLOADER_ERROR, LRE_CURL,
                    "curl_easy_setopt(h, CURLOPT_ERRORBUFFER, %s) failed: %s",
                    full_url, curl_easy_strerror(c_rc));
        lr_free(full_url);
        curl_easy_cleanup(h);
        return FALSE;
    }

    lr_free(full_url);

    // Prepare FILE
    int fd;

    if (target->target->fd != -1) {
        // Use supplied filedescriptor
        fd = dup(target->target->fd);
        if (fd == -1) {
            g_set_error(err, LR_DOWNLOADER_ERROR, LRE_IO,
                        "dup(%d) failed: %s",
                        target->target->fd, g_strerror(errno));
            curl_easy_cleanup(h);
            return FALSE;
        }
    } else {
        // Use supplied filename
        int open_flags = O_CREAT|O_TRUNC|O_RDWR;
        if (target->resume)
            open_flags &= ~O_TRUNC;

        fd = open(target->target->fn, open_flags, 0666);
        if (fd < 0) {
            g_set_error(err, LR_DOWNLOADER_ERROR, LRE_IO,
                        "Cannot open %s: %s",
                        target->target->fn, g_strerror(errno));
            curl_easy_cleanup(h);
            return FALSE;
        }
    }

    FILE *f = fdopen(fd, "w+b");
    if (!f) {
        g_set_error(err, LR_DOWNLOADER_ERROR, LRE_IO,
                    "fdopen(%d) failed: %s",
                    fd, g_strerror(errno));
        curl_easy_cleanup(h);
        return FALSE;
    }

    target->f = f;
    target->writecb_recieved = 0;
    target->writecb_required_range_written = FALSE;

    // Allow resume only for files that were originally being
    // downloaded by librepo
    if (target->resume && !has_librepo_xattr(fd)) {
        target->resume = FALSE;
        g_debug("%s: Resume ignored, existing file was not originally "
                "being downloaded by Librepo", __func__);
        if (ftruncate(fd, 0) == -1) {
            g_set_error(err, LR_DOWNLOADER_ERROR, LRE_IO,
                        "ftruncate() failed: %s", g_strerror(errno));
            fclose(f);
            curl_easy_cleanup(h);
            return FALSE;
        }
    }

    if (target->resume && target->resume_count >= LR_DOWNLOADER_MAXIMAL_RESUME_COUNT) {
        target->resume = FALSE;
        g_debug("%s: Download resume ignored, maximal number of attemtps (%d)"
                " has been reached", __func__, LR_DOWNLOADER_MAXIMAL_RESUME_COUNT);
    }

    // Resume - set offset to resume incomplete download
    if (target->resume) {
        target->resume_count++;

        if (target->original_offset == -1) {
            // Determine offset
            fseek(f, 0L, SEEK_END);
            gint64 determined_offset = ftell(f);
            if (determined_offset == -1) {
                // An error while determining offset =>
                // Download the whole file again
                determined_offset = 0;
            }
            target->original_offset = determined_offset;
        }

        gint64 used_offset = target->original_offset;
        g_debug("%s: Used offset for download resume: %"G_GINT64_FORMAT,
                __func__, used_offset);

        c_rc = curl_easy_setopt(h, CURLOPT_RESUME_FROM_LARGE,
                                (curl_off_t) used_offset);
        assert(c_rc == CURLE_OK);
    }

    // Add librepo extended attribute to the file
    // This xattr states that file is being downloaded by librepo
    // This xattr is removed once the file is completly downloaded
    // If librepo tries to resume a download, it checks if the xattr is present.
    // If it isn't the download is not resumed, but whole file is
    // downloaded again.
    add_librepo_xattr(fd, target->target->fn);

    if (target->target->byterangestart > 0) {
        assert(!target->target->resume);
        g_debug("%s: byterangestart is specified -> resume is set to %"
                G_GINT64_FORMAT, __func__, target->target->byterangestart);
        c_rc = curl_easy_setopt(h, CURLOPT_RESUME_FROM_LARGE,
                                (curl_off_t) target->target->byterangestart);
        assert(c_rc == CURLE_OK);
    }

    // Prepare progress callback
    target->cb_return_code = LR_CB_OK;
    if (target->target->progresscb) {
        c_rc = curl_easy_setopt(h, CURLOPT_PROGRESSFUNCTION, lr_progresscb) ||
               curl_easy_setopt(h, CURLOPT_NOPROGRESS, 0) ||
               curl_easy_setopt(h, CURLOPT_PROGRESSDATA, target);
        assert(c_rc == CURLE_OK);
    }

    // Prepare header callback
    if (target->target->expectedsize > 0) {
        c_rc = curl_easy_setopt(h, CURLOPT_HEADERFUNCTION, lr_headercb) ||
               curl_easy_setopt(h, CURLOPT_HEADERDATA, target);
        assert(c_rc == CURLE_OK);
    }

    // Prepare write callback
    c_rc = curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, lr_writecb) ||
           curl_easy_setopt(h, CURLOPT_WRITEDATA, target);
    assert(c_rc == CURLE_OK);

    // Set extra HTTP headers
    struct curl_slist *headers = NULL;
    if (target->handle && target->handle->httpheader) {
        // Fill in headers specified by user in LrHandle via LRO_HTTPHEADER
        for (int x=0; target->handle->httpheader[x]; x++) {
            headers = curl_slist_append(headers, target->handle->httpheader[x]);
            assert(headers);
        }
    }
    if (target->target->no_cache) {
        // Add headers that tell proxy to serve us fresh data
        headers = curl_slist_append(headers, "Cache-Control: no-cache");
        headers = curl_slist_append(headers, "Pragma: no-cache");
        assert(headers);
    }
    target->curl_rqheaders = headers;
    curl_easy_setopt(h, CURLOPT_HTTPHEADER, headers);

    // Add the new handle to the curl multi handle
    CURLMcode cm_rc = curl_multi_add_handle(dd->multi_handle, h);
    assert(cm_rc == CURLM_OK);

    // Set the state of transfer as running
    target->state = LR_DS_RUNNING;

    // Increase running transfers counter for mirror
    if (target->mirror) {
        increase_running_transfers(target->mirror);
    }

    // Set the state of header callback for this transfer
    target->headercb_state = LR_HCS_DEFAULT;
    g_free(target->headercb_interrupt_reason);
    target->headercb_interrupt_reason = NULL;

    // Set protocol of the target
    target->protocol = protocol;

    // Save curl handle for the current transfer
    target->curl_handle = h;

    // Add the transfer to the list of running transfers
    dd->running_transfers = g_slist_append(dd->running_transfers, target);

    return TRUE;
}

static gboolean
set_max_speeds_to_transfers(LrDownload *dd, GError **err)
{
    guint length;
    gint64 single_target_speed;

    assert(!err || *err == NULL);

    if (!dd->max_speed)  // Nothing to do
        return TRUE;

    length = g_slist_length(dd->running_transfers);
    if (!length)  // Nothing to do
        return TRUE;

    // Calculate a max speed (rounded up) per target
    single_target_speed = (dd->max_speed + (length - 1)) / length;

    for (GSList *elem = dd->running_transfers; elem; elem = g_slist_next(elem)) {
        LrTarget *ltarget = elem->data;
        CURL *curl_handle = ltarget->curl_handle;
        CURLcode code = curl_easy_setopt(curl_handle,
                                         CURLOPT_MAX_RECV_SPEED_LARGE,
                                         (curl_off_t) single_target_speed);
        if (code != CURLE_OK) {
            g_set_error(err, LR_DOWNLOADER_ERROR, LRE_CURLSETOPT,
                        "Cannot set CURLOPT_MAX_RECV_SPEED_LARGE option: %s",
                        curl_easy_strerror(code));
            return FALSE;
        }
    }

    return TRUE;
}

static gboolean
prepare_next_transfers(LrDownload *dd, GError **err)
{
    guint length = g_slist_length(dd->running_transfers);
    guint free_slots = dd->max_parallel_connections - length;

    assert(!err || *err == NULL);

    gboolean candidatefound = TRUE;
    while (free_slots > 0 && candidatefound) {
        gboolean ret = prepare_next_transfer(dd, &candidatefound, err);
        if (!ret)
            return FALSE;
        free_slots--;
    }

    // Set maximal speed for each target
    if (dd->max_speed)
        if (!set_max_speeds_to_transfers(dd, err))
            return FALSE;

    return TRUE;
}


/** Check the finished transfer
 * Evaluate CURL return code and status code of protocol if needed.
 * @param serious_error     Serious error is an error that isn't fatal,
 *                          but mirror that generate it should be penalized.
 *                          E.g.: Connection timeout - a mirror we are unable
 *                          to connect at is pretty useless for us, but
 *                          this could be only temporary state.
 *                          No fatal but also no good.
 * @param fatal_error       An error that cannot be recovered - e.g.
 *                          we cannot write to a socket, we cannot write
 *                          data to disk, bad function argument, ...
 */
static gboolean
check_finished_transfer_status(CURLMsg *msg,
                               LrTarget *target,
                               gboolean *serious_error,
                               gboolean *fatal_error,
                               GError **transfer_err,
                               GError **err)
{
    long code = 0;
    char *effective_url = NULL;

    assert(msg);
    assert(target);
    assert(!transfer_err || *transfer_err == NULL);
    assert(!err || *err == NULL);

    *fatal_error = FALSE;

    curl_easy_getinfo(msg->easy_handle,
                      CURLINFO_EFFECTIVE_URL,
                      &effective_url);

    if (msg->data.result != CURLE_OK) {
        // There was an error that is reported by CURLcode

        if (msg->data.result == CURLE_WRITE_ERROR &&
            target->writecb_required_range_written)
        {
            // Download was interrupted by writecb because
            // user want only specified byte range of the
            // target and the range was already downloaded
            g_debug("%s: Transfer was interrupted by writecb() "
                    "because the required range "
                    "(%"G_GINT64_FORMAT"-%"G_GINT64_FORMAT") "
                    "was downloaded.", __func__,
                    target->target->byterangestart,
                    target->target->byterangeend);
        } else if (target->headercb_state == LR_HCS_INTERRUPTED) {
            // Download was interrupted by header callback
            g_set_error(transfer_err, LR_DOWNLOADER_ERROR, LRE_CURL,
                        "Interrupted by header callback: %s",
                        target->headercb_interrupt_reason);
        } else {
            // There was a CURL error
            g_set_error(transfer_err, LR_DOWNLOADER_ERROR, LRE_CURL,
                        "Curl error (%d): %s for %s [%s]",
                        msg->data.result,
                        curl_easy_strerror(msg->data.result),
                        effective_url,
                        target->errorbuffer);

            switch (msg->data.result) {
            case CURLE_ABORTED_BY_CALLBACK:
            case CURLE_BAD_FUNCTION_ARGUMENT:
            case CURLE_CONV_REQD:
            case CURLE_COULDNT_RESOLVE_PROXY:
            case CURLE_FILESIZE_EXCEEDED:
            case CURLE_INTERFACE_FAILED:
#if LR_CURL_VERSION_CHECK(7, 21, 5)
            case CURLE_NOT_BUILT_IN:
#endif
            case CURLE_OUT_OF_MEMORY:
            //case CURLE_RECV_ERROR:  // See RhBug: 1219817
            //case CURLE_SEND_ERROR:
            case CURLE_SSL_CACERT_BADFILE:
            case CURLE_SSL_CRL_BADFILE:
            case CURLE_WRITE_ERROR:
                // Fatal error
                g_debug("%s: Fatal error - Curl code (%d): %s for %s [%s]",
                        __func__, msg->data.result,
                        curl_easy_strerror(msg->data.result),
                        effective_url,
                        target->errorbuffer);
                *fatal_error = TRUE;
                break;
            case CURLE_OPERATION_TIMEDOUT:
                // Serious error
                g_debug("%s: Serious error - Curl code (%d): %s for %s [%s]",
                        __func__, msg->data.result,
                        curl_easy_strerror(msg->data.result),
                        effective_url,
                        target->errorbuffer);
                *serious_error = TRUE;
                break;
            default:
                // Other error are not considered fatal
                break;
            }
        }

        return TRUE;
    }

    // curl return code is CURLE_OK but we need to check status code
    curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &code);
    if (code) {
        // Check status codes for some protocols
        if (effective_url && g_str_has_prefix(effective_url, "http")) {
            // Check HTTP(S) code
            if (code/100 != 2) {
                g_set_error(transfer_err,
                            LR_DOWNLOADER_ERROR,
                            LRE_BADSTATUS,
                            "Status code: %ld for %s", code, effective_url);
            }
        } else if (effective_url) {
            // Check FTP
            if (code/100 != 2) {
                g_set_error(transfer_err,
                            LR_DOWNLOADER_ERROR,
                            LRE_BADSTATUS,
                            "Status code: %ld for %s", code, effective_url);
            }
        } else {
            // Other protocols
            g_set_error(transfer_err,
                        LR_DOWNLOADER_ERROR,
                        LRE_BADSTATUS,
                        "Status code: %ld for %s", code, effective_url);
        }
    }

    return TRUE;
}


static gchar *
list_of_checksums_to_str(GSList *checksums)
{
    if (!checksums)
        return NULL;

    gchar *expected = g_strdup("");

    // Prepare pretty messages with list of expected checksums
    for (GSList *elem = checksums; elem; elem = g_slist_next(elem)) {
        gchar *tmp = NULL;
        LrDownloadTargetChecksum *chksum = elem->data;
        if (!chksum || !chksum->value || chksum->type == LR_CHECKSUM_UNKNOWN)
            continue;  // Bad checksum

        const gchar *chtype_str = lr_checksum_type_to_str(chksum->type);
        tmp = g_strconcat(expected, chksum->value, "(",
                          chtype_str ? chtype_str : "UNKNOWN",
                          ") ", NULL);
        free(expected);
        expected = tmp;
    }

    return expected;
}


static gboolean
check_finished_trasfer_checksum(int fd,
                                GSList *checksums,
                                gboolean *checksum_matches,
                                GError **transfer_err,
                                GError **err)
{
    gboolean matches = TRUE;
    GSList *calculated_chksums = NULL;

    for (GSList *elem = checksums; elem; elem = g_slist_next(elem)) {
        LrDownloadTargetChecksum *chksum = elem->data;
        LrDownloadTargetChecksum *calculated_chksum = NULL;
        gchar *calculated = NULL;

        if (!chksum || !chksum->value || chksum->type == LR_CHECKSUM_UNKNOWN)
            continue;  // Bad checksum

        lseek(fd, 0, SEEK_SET);
        gboolean ret = lr_checksum_fd_compare(chksum->type,
                                              fd,
                                              chksum->value,
                                              1,
                                              &matches,
                                              &calculated,
                                              err);
        if (!ret)
            return FALSE;

        // Store calculated checksum
        calculated_chksum = lr_downloadtargetchecksum_new(chksum->type,
                                                          calculated);
        g_free(calculated);
        calculated_chksums = g_slist_append(calculated_chksums,
                                            calculated_chksum);

        if (matches) {
            // At least one checksum matches
            g_debug("%s: Checksum (%s) %s is OK", __func__,
                    lr_checksum_type_to_str(chksum->type),
                    chksum->value);
            break;
        }
    }

    *checksum_matches = matches;

    if (!matches) {
        // Checksums doesn't match
        _cleanup_free_ gchar *calculated = NULL;
        _cleanup_free_ gchar *expected = NULL;

        calculated = list_of_checksums_to_str(calculated_chksums);
        expected = list_of_checksums_to_str(checksums);

        // Set error message
        g_set_error(transfer_err,
                LR_DOWNLOADER_ERROR,
                LRE_BADCHECKSUM,
                "Downloading successful, but checksum doesn't match. "
                "Calculated: %s Expected: %s", calculated, expected);
    }

    g_slist_free_full(calculated_chksums,
                      (GDestroyNotify) lr_downloadtargetchecksum_free);

    return TRUE;
}


/** Truncate file - Used to remove downloaded garbage (error html pages, etc.)
 */
static gboolean
truncate_transfer_file(LrTarget *target, GError **err)
{
    off_t original_offset = 0;  // Truncate whole file by default
    int rc;

    assert(!err || *err == NULL);

    if (target->original_offset > -1)
        // If resume is enabled -> truncate file to its original position
        original_offset = target->original_offset;

    if (target->target->fn)  // Truncate by filename
        rc = truncate(target->target->fn, original_offset);
    else  // Truncate by file descriptor number
        rc = ftruncate(target->target->fd, original_offset);

    if (rc == -1) {
        g_set_error(err, LR_DOWNLOADER_ERROR, LRE_IO,
                    "ftruncate() failed: %s", g_strerror(errno));
        return FALSE;
    }

    if (!target->target->fn) {
        // In case fd is used, seek to the original offset
        if (lseek(target->target->fd, original_offset, SEEK_SET) == -1) {
            g_set_error(err, LR_DOWNLOADER_ERROR, LRE_IO,
                        "lseek() failed: %s", g_strerror(errno));
            return FALSE;
        }
    }

    return TRUE;
}


/** Return mirror rank or -1.0 if the rank cannot be determined
 * (e.g. when is too early)
 * Rank is currently just success rate for the mirror
 */
static gdouble
mirror_rank(LrMirror *mirror)
{
    gdouble rank = -1.0;

    int successful = mirror->successful_transfers;
    int failed = mirror->failed_transfers;
    int finished_transfers = successful + failed;

    if (finished_transfers < 3)
        return rank; // Do not judge too early

    rank = successful / (double) finished_transfers;

    return rank;
}


/** Sort mirrors. Penalize the error ones.
 * In fact only move the current finished mirror forward or backward
 * by one position.
 * @param mirrors   GSList of mirrors (order of list elements won't be
 *                  changed, only data pointers)
 * @param mirror    Mirror of just finished transfer
 * @param success   Was download from the mirror successful
 * @param serious   If success is FALSE, serious mean that error was serious
 *                  (like connection timeout), and the mirror should be
 *                  penalized more that usual.
 */
static gboolean
sort_mirrors(GSList *mirrors, LrMirror *mirror, gboolean success, gboolean serious)
{
    GSList *elem = mirrors;
    GSList *prev = NULL;
    GSList *next = NULL;
    gdouble rank_cur;

    assert(mirrors);
    assert(mirror);

    for (; elem && elem->data != mirror; elem = g_slist_next(elem))
        prev = elem;

    assert(elem);  // Mirror should always exists in the list of mirrors

    next = g_slist_next(elem);

    if (!success && !next)
        goto exit; // Penalization not needed - Mirror is already the last one
    if (success && !prev)
        goto exit; // Bonus not needed - Mirror is already the first one

    // Serious errors
    if (serious && mirror->successful_transfers == 0) {
        // Mirror that encounter a serious error and has no successful
        // transfers should be moved at the end of the list
        // (such mirror is probably down/broken/buggy)
        GSList *last = g_slist_last(elem);
        elem->data = last->data;
        last->data = (gpointer) mirror;
        g_debug("%s: Mirror %s was moved at the end", __func__, mirror->mirror->url);
        goto exit; // No more hadling needed
    }

    // Calculate ranks
    rank_cur  = mirror_rank(mirror);
    if (rank_cur < 0.0)
        goto exit; // Too early to judge


    if (!success) {
        // Penalize
        gdouble rank_next = mirror_rank(next->data);
        if (rank_next < 0.0 || rank_next > rank_cur) {
            elem->data = next->data;
            next->data = (gpointer) mirror;
            g_debug("%s: Mirror %s was penalized", __func__, mirror->mirror->url);
        }
    } else {
        // Bonus
        gdouble rank_prev = mirror_rank(prev->data);
        if (rank_prev < rank_cur) {
            elem->data = prev->data;
            prev->data = mirror;
            g_debug("%s: Mirror %s was awarded", __func__, mirror->mirror->url);
        }
    }

exit:
    if (g_getenv("LIBREPO_DEBUG_ADAPTIVEMIRRORSORTING")) {
        // Debug
        g_debug("%s: Updated order of mirrors (for %p):", __func__, mirrors);
        for (GSList *elem = mirrors; elem; elem = g_slist_next(elem)) {
            LrMirror *m = elem->data;
            g_debug(" %s (s: %d f: %d)", m->mirror->url,
                   m->successful_transfers, m->failed_transfers);
        }
    }

    return TRUE;
}


static gboolean
check_transfer_statuses(LrDownload *dd, GError **err)
{
    assert(dd);
    assert(!err || *err == NULL);

    int msgs_in_queue;
    CURLMsg *msg;

    while ((msg = curl_multi_info_read(dd->multi_handle, &msgs_in_queue))) {
        LrTarget *target = NULL;
        char *effective_url = NULL;
        int fd;
        gboolean matches = TRUE;
        GError *transfer_err = NULL;
        GError *tmp_err = NULL;
        gboolean ret;
        gboolean serious_error = FALSE;
        gboolean fatal_error = FALSE;
        GError *fail_fast_error = NULL;

        if (msg->msg != CURLMSG_DONE) {
            // We are only interested in messages about finished transfers
            continue;
        }

        // Find the target with this curl easy handle
        for (GSList *elem = dd->running_transfers; elem; elem = g_slist_next(elem)) {
            LrTarget *ltarget = elem->data;
            if (ltarget->curl_handle == msg->easy_handle)
                target = ltarget;
        }

        assert(target);  // Each easy handle used in the multi handle
                         // should always belong to some target from
                         // the running_transfers list

        curl_easy_getinfo(msg->easy_handle,
                          CURLINFO_EFFECTIVE_URL,
                          &effective_url);

        effective_url = g_strdup(effective_url); // Make the effetive url
                                                  // persistent to survive
                                                  // the curl_easy_cleanup()

        g_debug("%s: Transfer finished: %s (Effective url: %s)",
                __func__, target->target->path, effective_url);

        //
        // Check status of finished transfer
        //
        ret = check_finished_transfer_status(msg, target, &serious_error,
                                             &fatal_error, &transfer_err, err);
        if (!ret)  // Error
            return FALSE;

        if (transfer_err)  // Transfer was unsuccessful
            goto transfer_error;

        //
        // Checksum checking
        //
        fflush(target->f);
        fd = fileno(target->f);
        ret = check_finished_trasfer_checksum(fd,
                                              target->target->checksums,
                                              &matches,
                                              &transfer_err,
                                              &tmp_err);
        if (!ret) { // Error
            g_propagate_prefixed_error(err, tmp_err, "Downloading from %s"
                    "was successful but error encountered while "
                    "checksuming: ", effective_url);
            return FALSE;
        }

        if (transfer_err)  // Checksum doesn't match
            goto transfer_error;

        //
        // Any other checks should go here
        //

transfer_error:

        //
        // Cleanup
        //
        curl_multi_remove_handle(dd->multi_handle, target->curl_handle);
        curl_easy_cleanup(target->curl_handle);
        target->curl_handle = NULL;
        g_free(target->headercb_interrupt_reason);
        target->headercb_interrupt_reason = NULL;
        fclose(target->f);
        target->f = NULL;
        if (target->curl_rqheaders) {
            curl_slist_free_all(target->curl_rqheaders);
            target->curl_rqheaders = NULL;
        }

        dd->running_transfers = g_slist_remove(dd->running_transfers,
                                               (gconstpointer) target);
        target->tried_mirrors = g_slist_append(target->tried_mirrors,
                                               target->mirror);

        if (target->mirror) {
            gboolean success = transfer_err == NULL;
            mirror_update_statistics(target->mirror, success);
            if (dd->adaptivemirrorsorting)
                sort_mirrors(target->lrmirrors, target->mirror, success, serious_error);
        }

        if (transfer_err) {  // There was an error during transfer
            int complete_url_in_path = strstr(target->target->path, "://") ? 1 : 0;
            guint num_of_tried_mirrors = g_slist_length(target->tried_mirrors);
            gboolean retry = FALSE;

            g_debug("%s: Error during transfer: %s", __func__, transfer_err->message);

            // Call mirrorfailure callback
            LrMirrorFailureCb mf_cb =  target->target->mirrorfailurecb;
            if (mf_cb) {
                int rc = mf_cb(target->target->cbdata,
                               transfer_err->message,
                               effective_url);
                if (rc == LR_CB_ABORT) {
                    // User wants to abort this download, so make the error fatal
                    fatal_error = TRUE;
                } else if (rc == LR_CB_ERROR) {
                    gchar *original_err_msg = g_strdup(transfer_err->message);
                    g_clear_error(&transfer_err);
                    g_debug("%s: Downloading was aborted by LR_CB_ERROR from "
                            "mirror failure callback. Original error was: "
                            "%s", __func__, original_err_msg);
                    g_set_error(&transfer_err, LR_DOWNLOADER_ERROR, LRE_CBINTERRUPTED,
                                "Downloading was aborted by LR_CB_ERROR from "
                                "mirror failure callback. Original error was: "
                                "%s", original_err_msg);
                    g_free(original_err_msg);
                    fatal_error = TRUE;
                    target->cb_return_code = LR_CB_ERROR;
                }
            }

            if (!fatal_error &&
                !complete_url_in_path &&
                !target->target->baseurl)
            {   
              
                // Temporary error (serious_error) during download occured and
                // another transfers are running or there are successful transfers
                // and fewer failed transfers than tried parallel connections. It may be mirror is OK
                // but accepts fewer parallel connections.
                if (serious_error &&
                    (has_running_transfers(target->mirror) ||
                      (target->mirror->successful_transfers > 0 &&
                        target->mirror->failed_transfers < target->mirror->max_tried_parallel_connections)))
                {
                    g_debug("%s: Lower maximum of allowed parallel connections for this mirror", __func__);
                    if (has_running_transfers(target->mirror))
                        target->mirror->allowed_parallel_connections = target->mirror->running_transfers;
                    else
                        target->mirror->allowed_parallel_connections = 1;

                    // Give used mirror another chance
                    target->tried_mirrors = g_slist_remove(target->tried_mirrors, target->mirror);
                    num_of_tried_mirrors = g_slist_length(target->tried_mirrors);
                }

                if (can_try_more_mirrors(dd, num_of_tried_mirrors))
                {
                  // Try another mirror
                  g_debug("%s: Ignore error - Try another mirror", __func__);
                  target->state = LR_DS_WAITING;
                  retry = TRUE;
                  g_error_free(transfer_err);  // Ignore the error

                  // Truncate file - remove downloaded garbage (error html page etc.)
                  if (!truncate_transfer_file(target, err))
                      return FALSE;
                }
            }
            
            if (!retry) {
                // No more mirrors to try or baseurl used or fatal error
                g_debug("%s: No more retries (tried: %d)",
                        __func__, num_of_tried_mirrors);
                target->state = LR_DS_FAILED;

                // Call end callback
                LrEndCb end_cb =  target->target->endcb;
                if (end_cb) {
                    int rc = end_cb(target->target->cbdata,
                                    LR_TRANSFER_ERROR,
                                    transfer_err->message);
                    if (rc == LR_CB_ERROR) {
                        target->cb_return_code = LR_CB_ERROR;
                        g_debug("%s: Downloading was aborted by LR_CB_ERROR "
                                "from end callback", __func__);
                    }
                }

                lr_downloadtarget_set_error(target->target,
                                            transfer_err->code,
                                            "Download failed: %s",
                                            transfer_err->message);
                if (dd->failfast) {
                    // Fail fast is enabled, fail on any error
                    g_propagate_error(&fail_fast_error, transfer_err);
                } else if (target->cb_return_code == LR_CB_ERROR) {
                    // Callback returned LR_CB_ERROR, abort the downloading
                    g_debug("%s: Downloading was aborted by LR_CB_ERROR", __func__);
                    g_propagate_error(&fail_fast_error, transfer_err);
                } else {
                    // Fail fast is disabled and callback doesn't repor serious
                    // error, so this download is aborted, but other download
                    // can continue (do not abort whole downloading)
                    g_error_free(transfer_err);
                }
            }

        } else {
            // No error encountered, transfer finished successfully
            target->state = LR_DS_FINISHED;
            lr_downloadtarget_set_error(target->target, LRE_OK, NULL);
            if (target->mirror)
                lr_downloadtarget_set_usedmirror(target->target,
                                                 target->mirror->mirror->url);
            lr_downloadtarget_set_effectiveurl(target->target,
                                               effective_url);

            // Remove xattr that states that the file is being downloaded
            // by librepo, because the file is now completly downloaded
            // and the xattr is not needed (is is useful only for resuming)
            remove_librepo_xattr(target->target->fd);

            // Call end callback
            LrEndCb end_cb = target->target->endcb;
            if (end_cb) {
                int rc = end_cb(target->target->cbdata,
                                LR_TRANSFER_SUCCESSFUL,
                                NULL);
                if (rc == LR_CB_ERROR) {
                    target->cb_return_code = LR_CB_ERROR;
                    g_debug("%s: Downloading was aborted by LR_CB_ERROR "
                            "from end callback", __func__);
                    g_set_error(&fail_fast_error, LR_DOWNLOADER_ERROR,
                                LRE_CBINTERRUPTED,
                                "Interupted by LR_CB_ERROR from end callback");
                }
            }
        }

        lr_free(effective_url);

        if (fail_fast_error) {
            // Interrupt whole downloading
            // A fatal error occurred or interrupted by callback
            g_propagate_error(err, fail_fast_error);
            return FALSE;
        }
    }

    // At this point, after handles of finished transfers were removed
    // from the multi_handle, we could add new waiting transfers.
    return prepare_next_transfers(dd, err);
}


static gboolean
lr_perform(LrDownload *dd, GError **err)
{
    CURLMcode cm_rc;    // CurlM_ReturnCode
    int still_running;

    assert(dd);
    assert(!err || *err == NULL);

    do { // Before version 7.20.0 CURLM_CALL_MULTI_PERFORM can appear
        cm_rc = curl_multi_perform(dd->multi_handle, &still_running);
    } while (cm_rc == CURLM_CALL_MULTI_PERFORM);

    if (cm_rc != CURLM_OK) {
        g_set_error(err, LR_DOWNLOADER_ERROR, LRE_CURLM,
                    "curl_multi_perform() error: %s",
                    curl_multi_strerror(cm_rc));
        return FALSE;
    }

    if (lr_interrupt) {
        // Check interrupt after each call of curl_multi_perform
        g_set_error(err, LR_DOWNLOADER_ERROR, LRE_INTERRUPTED,
                    "Interrupted by signal");
        return FALSE;
    }

    while (dd->running_transfers) {
        int rc;
        int maxfd = -1;
        long curl_timeout = -1;
        struct timeval timeout;
        fd_set fdread, fdwrite, fdexcep;

        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdexcep);

        // Set suitable timeout to play around with
        timeout.tv_sec  = 1;
        timeout.tv_usec = 0;

        cm_rc = curl_multi_timeout(dd->multi_handle, &curl_timeout);
        if (cm_rc != CURLM_OK) {
            g_set_error(err, LR_DOWNLOADER_ERROR, LRE_CURLM,
                        "curl_multi_timeout() error: %s",
                        curl_multi_strerror(cm_rc));
            return FALSE;
        }

        if (curl_timeout >= 0) {
            timeout.tv_sec = curl_timeout / 1000;
            if (timeout.tv_sec > 1)
                timeout.tv_sec = 1;
            else
                timeout.tv_usec = (curl_timeout % 1000) * 1000;
        }

        // Get file descriptors from the transfers
        cm_rc = curl_multi_fdset(dd->multi_handle, &fdread, &fdwrite,
                                 &fdexcep, &maxfd);
        if (cm_rc != CURLM_OK) {
            g_set_error(err, LR_DOWNLOADER_ERROR, LRE_CURLM,
                        "curl_multi_fdset() error: %s",
                        curl_multi_strerror(cm_rc));
            return FALSE;
        }

        rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
        if (rc < 0) {
            if (errno == EINTR) {
                g_debug("%s: select() interrupted by signal", __func__);
                //goto retry;
            } else {
                g_set_error(err, LR_DOWNLOADER_ERROR, LRE_SELECT,
                            "select() error: %s", g_strerror(errno));
                return FALSE;
            }
        }

        // This do-while loop is important. Because if curl_multi_perform sets
        // still_running to 0, we need to check if there are any next
        // transfers available (we need to call check_transfer_statuses).
        // Because if there will be no next transfers available and the
        // curl multi handle is empty (all transfers already
        // finished - this is what still_running == 0 means),
        // then the next iteration of main downloding loop cause a 1sec
        // waiting on the select() call.
        do {
            // Check if any handle finished and potentialy add one or more
            // waiting downloads to the multi_handle.
            rc = check_transfer_statuses(dd, err);
            if (!rc)
                return FALSE;

            if (lr_interrupt) {
                g_set_error(err, LR_DOWNLOADER_ERROR, LRE_INTERRUPTED,
                            "Interrupted by signal");
                return FALSE;
            }

            // Do curl_multi_perform()
            do { // Before version 7.20.0 CURLM_CALL_MULTI_PERFORM can appear
                cm_rc = curl_multi_perform(dd->multi_handle, &still_running);
                if (lr_interrupt) {
                    // Check interrupt after each call of curl_multi_perform
                    g_set_error(err, LR_DOWNLOADER_ERROR, LRE_INTERRUPTED,
                                "Interrupted by signal");
                    return FALSE;
                }
            } while (cm_rc == CURLM_CALL_MULTI_PERFORM);

            if (cm_rc != CURLM_OK) {
                g_set_error(err, LR_DOWNLOADER_ERROR, LRE_CURLM,
                            "curl_multi_perform() error: %s",
                            curl_multi_strerror(cm_rc));
                return FALSE;
            }
        } while (still_running == 0 && dd->running_transfers);
    }

    return check_transfer_statuses(dd, err);
}

gboolean
lr_download(GSList *targets,
            gboolean failfast,
            GError **err)
{
    gboolean ret = FALSE;
    LrDownload dd;             // dd stands for Download Data
    GError *tmp_err = NULL;

    assert(!err || *err == NULL);

    if (lr_interrupt) {
        g_set_error(err, LR_DOWNLOADER_ERROR, LRE_INTERRUPTED,
                    "Interrupted by signal");
        return FALSE;
    }

    if (!targets) {
        g_debug("%s: No targets", __func__);
        return TRUE;
    }

    // XXX: Downloader configuration (max parallel connections etc.)
    // is taken from the handle of the first target.
    LrHandle *lr_handle = ((LrDownloadTarget *) targets->data)->handle;

    // Prepare download data
    dd.failfast = failfast;

    if (lr_handle) {
        dd.max_parallel_connections = lr_handle->maxparalleldownloads;
        dd.max_connection_per_host = lr_handle->maxdownloadspermirror;
        dd.max_mirrors_to_try = lr_handle->maxmirrortries;
        dd.max_speed = lr_handle->maxspeed;
        dd.allowed_mirror_failures = lr_handle->allowed_mirror_failures;
        dd.adaptivemirrorsorting = lr_handle->adaptivemirrorsorting;
    } else {
        // No handle, this is allowed when a complete URL is passed
        // via relative_url param.
        dd.max_parallel_connections = LRO_MAXPARALLELDOWNLOADS_DEFAULT;
        dd.max_connection_per_host = LRO_MAXDOWNLOADSPERMIRROR_DEFAULT;
        dd.max_mirrors_to_try = LRO_MAXMIRRORTRIES_DEFAULT;
        dd.max_speed = LRO_MAXSPEED_DEFAULT;
        dd.allowed_mirror_failures = LRO_ALLOWEDMIRRORFAILURES_DEFAULT;
        dd.adaptivemirrorsorting = LRO_ADAPTIVEMIRRORSORTING_DEFAULT;
    }

    dd.multi_handle = curl_multi_init();
    if (!dd.multi_handle) {
        // Something went wrong
        g_set_error(err, LR_DOWNLOADER_ERROR, LRE_CURLM,
                    "curl_multi_init() call failed");
        return FALSE;
    }

    // Prepare list of LrTargets and LrHandleMirrors
    dd.handle_mirrors = NULL;
    dd.targets = NULL;
    for (GSList *elem = targets; elem; elem = g_slist_next(elem)) {
        LrDownloadTarget *dtarget = elem->data;

        // Assertions
        assert(dtarget);
        assert(dtarget->path);
        assert((dtarget->fd > 0 && !dtarget->fn) || (dtarget->fd < 0 && dtarget->fn));
        g_debug("%s: Target: %s (%s)", __func__,
                dtarget->path,
                (dtarget->baseurl) ? dtarget->baseurl : "-");

        // Cleanup of LrDownloadTarget
        lr_downloadtarget_reset(dtarget);

        // Create and fill LrTarget
        LrTarget *target = lr_malloc0(sizeof(*target));
        target->state           = LR_DS_WAITING;
        target->target          = dtarget;
        target->original_offset = -1;
        target->resume          = dtarget->resume;
        target->target->rcode   = LRE_UNFINISHED;
        target->target->err     = "Not finished";
        target->handle          = dtarget->handle;
        dd.targets = g_slist_append(dd.targets, target);
        // Add list of handle internal mirrors to dd.handle_mirrors
        // if doesn't exists yet and set the list reference
        // to the target.
        dd.handle_mirrors = lr_prepare_lrmirrors(dd.handle_mirrors, target);
    }

    dd.running_transfers = NULL;

    // Prepare the first set of transfers
    if (!prepare_next_transfers(&dd, &tmp_err))
        goto lr_download_cleanup;

    // Perform!
    g_debug("%s: Downloading started", __func__);
    ret = lr_perform(&dd, &tmp_err);

    assert(ret || tmp_err);

lr_download_cleanup:

    if (tmp_err) {
        // If there was an error, stop all transfers that are in progress.
        g_debug("%s: Error while downloading: %s", __func__, tmp_err->message);

        for (GSList *elem = dd.running_transfers; elem; elem = g_slist_next(elem)){
            LrTarget *target = elem->data;

            curl_multi_remove_handle(dd.multi_handle, target->curl_handle);
            curl_easy_cleanup(target->curl_handle);
            target->curl_handle = NULL;
            fclose(target->f);
            target->f = NULL;
            g_free(target->headercb_interrupt_reason);
            target->headercb_interrupt_reason = NULL;

            // Call end callback
            LrEndCb end_cb =  target->target->endcb;
            if (end_cb) {
                gchar *msg = g_strdup_printf("Not finished - interrupted by "
                                             "error: %s", tmp_err->message);
                end_cb(target->target->cbdata, LR_TRANSFER_ERROR, msg);
                // No need to check end_cb return value, because there
                // already was an error
                g_free(msg);
            }

            lr_downloadtarget_set_error(target->target, LRE_UNFINISHED,
                    "Not finished - interrupted by error: %s",
                    tmp_err->message);
        }

        g_slist_free(dd.running_transfers);
        dd.running_transfers = NULL;

        g_propagate_error(err, tmp_err);
    }

    assert(dd.running_transfers == NULL);

    curl_multi_cleanup(dd.multi_handle);

    // Clean up dd.handle_mirrors
    for (GSList *elem = dd.handle_mirrors; elem; elem = g_slist_next(elem)) {
        LrHandleMirrors *handle_mirrors = elem->data;
        for (GSList *el = handle_mirrors->lrmirrors; el; el = g_slist_next(el)) {
            LrMirror *mirror = el->data;
            lr_free(mirror);
        }
        g_slist_free(handle_mirrors->lrmirrors);
        lr_free(handle_mirrors);
    }
    g_slist_free(dd.handle_mirrors);

    // Clean up targets
    for (GSList *elem = dd.targets; elem; elem = g_slist_next(elem)) {
        LrTarget *target = elem->data;
        assert(target->curl_handle == NULL);
        assert(target->f == NULL);

        // Remove file created for the target if download was
        // unsuccessful and the file doesn't exists before or
        // its original content was overwritten
        if (target->state != LR_DS_FINISHED) {
            if (!target->resume || target->original_offset == 0) {
                // Remove target file if the file doesn't
                // exist before or was empty or was overwritten
                if (target->target->fn) {
                    // We can remove only files that were specified by fn
                    if (unlink(target->target->fn) != 0) {
                        g_debug("%s: Error while removing: %s",
                                __func__, g_strerror(errno));
                    }
                }
            }
        }

        g_slist_free(target->tried_mirrors);
        lr_free(target);
    }
    g_slist_free(dd.targets);

    return ret;
}

gboolean
lr_download_target(LrDownloadTarget *target,
                   GError **err)
{
    gboolean ret;
    GSList *list = NULL;

    assert(!err || *err == NULL);

    if (!target)
        return TRUE;

    list = g_slist_prepend(list, target);

    ret = lr_download(list, TRUE, err);

    g_slist_free(list);

    return ret;
}

gboolean
lr_download_url(LrHandle *lr_handle, const char *url, int fd, GError **err)
{
    gboolean ret;
    LrDownloadTarget *target;
    GError *tmp_err = NULL;

    assert(url);
    assert(!err || *err == NULL);

    // Prepare target
    target = lr_downloadtarget_new(lr_handle,
                                   url, NULL, fd, NULL,
                                   NULL, 0, 0, NULL, NULL,
                                   NULL, NULL, NULL, 0, 0, FALSE);

    // Download the target
    ret = lr_download_target(target, &tmp_err);

    assert(ret || tmp_err);
    assert(!(target->err) || !ret);

    if (!ret)
        g_propagate_error(err, tmp_err);

    lr_downloadtarget_free(target);

    lseek(fd, 0, SEEK_SET);

    return ret;
}

typedef struct {
    LrProgressCb cb; /*!<
        User callback */

    LrMirrorFailureCb mfcb; /*!<
        Mirror failure callback */

    GSList *singlecbdata; /*!<
        List of LrCallbackData */

} LrSharedCallbackData;

typedef struct {
    double downloaded;  /*!< Currently downloaded bytes of target */
    double total;       /*!< Total size of the target */
    void *userdata;     /*!< User data related to the target */
    LrSharedCallbackData *sharedcbdata; /*!< Shared cb data */
} LrCallbackData;

int
lr_multi_progress_func(void* ptr,
                       double total_to_download,
                       double now_downloaded)
{
    LrCallbackData *cbdata = ptr;
    LrSharedCallbackData *shared_cbdata = cbdata->sharedcbdata;

    if (cbdata->downloaded > now_downloaded
        || cbdata->total != total_to_download)
    {
        // Reset counters
        // This is not first mirror for the transfer,
        // we have already downloaded some data
        cbdata->total = total_to_download;

        // Call progress cb with zeroized params
        // This should tell progress cb, that the total_to_download
        // size is changed.
        int ret = shared_cbdata->cb(cbdata->userdata, 0.0, 0.0);
        if (ret != LR_CB_OK)
            return ret;
    }

    cbdata->downloaded = now_downloaded;

    // Prepare values for the user callback
    double totalsize = 0.0;
    double downloaded = 0.0;

    for (GSList *elem = shared_cbdata->singlecbdata; elem; elem = g_slist_next(elem)) {
        LrCallbackData *singlecbdata = elem->data;
        totalsize += singlecbdata->total;
        downloaded += singlecbdata->downloaded;
    }

    if (downloaded > totalsize)
        totalsize = downloaded;

    // Call user callback
    return shared_cbdata->cb(cbdata->userdata,
                             totalsize,
                             downloaded);
}

int
lr_multi_mf_func(void *ptr, const char *msg, const char *url)
{
    LrCallbackData *cbdata = ptr;
    LrSharedCallbackData *shared_cbdata = cbdata->sharedcbdata;
    return shared_cbdata->mfcb(cbdata->userdata, msg, url);
}

gboolean
lr_download_single_cb(GSList *targets,
                      gboolean failfast,
                      LrProgressCb cb,
                      LrMirrorFailureCb mfcb,
                      GError **err)
{
    gboolean ret;
    LrSharedCallbackData shared_cbdata;

    assert(!err || *err == NULL);

    shared_cbdata.cb                 = cb;
    shared_cbdata.mfcb               = mfcb;
    shared_cbdata.singlecbdata       = NULL;

    // "Inject" callbacks and callback data to the targets
    for (GSList *elem = targets; elem; elem = g_slist_next(elem)) {
        LrDownloadTarget *target = elem->data;

        LrCallbackData *lrcbdata = lr_malloc0(sizeof(*lrcbdata));
        lrcbdata->downloaded        = 0.0;
        lrcbdata->total             = 0.0;
        lrcbdata->userdata          = target->cbdata;
        lrcbdata->sharedcbdata      = &shared_cbdata;

        target->progresscb      = (cb) ? lr_multi_progress_func : NULL;
        target->mirrorfailurecb = (mfcb) ? lr_multi_mf_func : NULL;
        target->cbdata          = lrcbdata;

        shared_cbdata.singlecbdata = g_slist_append(shared_cbdata.singlecbdata,
                                                    lrcbdata);
    }

    ret = lr_download(targets, failfast, err);

    // Remove callbacks and callback data
    for (GSList *elem = targets; elem; elem = g_slist_next(elem)) {
        LrDownloadTarget *target = elem->data;
        LrCallbackData *cbdata = target->cbdata;
        target->cbdata = cbdata->userdata;
        target->progresscb = NULL;
        target->mirrorfailurecb = NULL;
        lr_free(cbdata);
    }
    g_slist_free(shared_cbdata.singlecbdata);

    return ret;
}
