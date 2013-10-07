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

#include "downloader.h"
#include "rcodes.h"
#include "util.h"
#include "downloadtarget.h"
#include "downloadtarget_internal.h"
#include "handle.h"
#include "handle_internal.h"

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

typedef struct {
    LrInternalMirror *mirror; /*!<
        Mirror */
    int running_transfers; /*!<
        How many transfers from this mirror are currently in progres. */
    int successfull_transfers; /*!<
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
    CURL *curl_handle; /*!<
        Used curl handle or NULL */
    FILE *f; /*!<
        fdopened file descriptor from LrDownloadTarget and used
        in curl_handle. */
    GSList *tried_mirrors; /*!<
        List of already tried mirrors. This mirrors won't be tried again. */
    gint64 original_offset; /*!<
        If resume is enabled, this is the determinet offset where to resume
        the downloading. If resume is not enabled, then value is -1. */
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

    // Data

    LrHandle *lr_handle; /*!<
        Librepo handle */

    CURLM *multi_handle; /*!<
        Curl Multi handle */

    GSList *mirrors; /*!<
        All mirrors (list of pointers to LrMirror structures) */

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
 * | int max_mirrors_to_try       |
 * |                              |
 * | LrHandle *lr_handle          |
 * | CURLM *multi_handle          |
 * |                              |
 * | GSList *mirrors             ------\
 * | GSList *targets             --\    |
 * | GSList *running_transfers   ---\   |
 * +------------------------------+  |  |
 *                                   |  |
 *   /------------------------------/   |
 *  |                                   |
 *  |   /------------------------------/
 *  |  |
 *  |  |       +---------------------------+
 *  |  |       |         LrMirror          |
 *  |  |     +---------------------------+-|
 *  |  |     |         LrMirror          | |
 *  |  |   +---------------------------+-| |
 *  |   \->|         LrMirror          | | |
 *  |      +---------------------------+ | |    +---------------------+
 *  |      | LrInternalMirror *mirror --------->|   LrInternalMirror  |
 *  |      | int running_transfers     | |   /->+---------------------+
 *  |      | int successfull_transfers |-+   |  | char *url           |
 *  |      | int failed_transfers      |     |  | int preference      |
 *  |      +---------------------------+     |  | int fails           |
 *  |                                        |  +---------------------+
 *  |                                        |
 *  |        +----------------------------+  |
 *  |        |          LrTarget          |  |
 *  |      +----------------------------+-|  |
 *  |      |          LrTarget          | |  |     +--------------------------+
 *  |    +----------------------------+-| |  |  /->|      LrDownloadTarget    |
 *   \-> |          LrTarget          | | |  |  |  +--------------------------+
 *       +----------------------------+ | |  |  |  | char *path               |
 *       | LrDownloadState state      | | |  |  |  | char *baseurl            |
 *       | LrDownloadTarget *target  ----------/   | int fd                   |
 *       | LrMirror *mirror          -------/      | LrChecksumType checks..  |
 *       | CURL *curl_handle          |-+          | char *checksum           |
 *       | FILE *f                    |            | int resume               |
 *       | GSList *tried_mirrors      |            | LrProgressCb progresscb  |
 *       +----------------------------+            | void *cbdata             |
 *                                                 | GStringChunk *chunk      |
 *                                                 | int rcode                |
 *                                                 | char *err                |
 *                                                 +--------------------------+
 */

static int
lr_progresscb(void *ptr,
              double total_to_download,
              double now_downloaded,
              G_GNUC_UNUSED double total_to_upload,
              G_GNUC_UNUSED double now_uploaded)
{
    LrTarget *target = ptr;

    assert(target);
    assert(target->target);

    if (!target->target->progresscb)
        return 0;

    return target->target->progresscb(target->target->cbdata,
                                      total_to_download,
                                      now_downloaded);
}

static gboolean
prepare_next_transfer(LrDownload *dd, GError **err)
{
    LrTarget *target = NULL;
    LrMirror *mirror = NULL;
    char *full_url = NULL;
    int complete_url_in_path = 0;

    assert(dd);
    assert(!err || *err == NULL);

    // Select a waiting target

    for (GSList *elem = dd->targets; elem; elem = g_slist_next(elem)) {
        LrTarget *c_target = elem->data;
        if (c_target->state == LR_DS_WAITING) {
            target = c_target;
            break;
        }
    }

    if (!target)  // No target is waiting
        return TRUE;

    // Check if path is a complete URL
    complete_url_in_path = strstr(target->target->path, "://") ? 1 : 0;

    if (!target->target->baseurl
        && !dd->mirrors
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

    // Select a base part of url (use the baseurl or some mirror)
    if (complete_url_in_path) {
        // In path we got a complete url, do not use mirror or basepath
        full_url = g_strdup(target->target->path);
    } else if (target->target->baseurl) {
        // Use base URL
        full_url = lr_pathconcat(target->target->baseurl,
                                 target->target->path,
                                 NULL);
    } else {
        // Try to find a suitable mirror

        int at_least_one_suitable_mirror_found = 0;
        //  ^^^ This variable is used to indentify that all possible mirrors
        // were already tried and the transfer shoud be marked as failed.

        for (GSList *elem = dd->mirrors; elem; elem = g_slist_next(elem)) {
            LrMirror *c_mirror = elem->data;

            if (g_slist_find(target->tried_mirrors, c_mirror)) {
                // This mirror was already tried for this target
                continue;
            }

            at_least_one_suitable_mirror_found = 1;

            // Number of transfers which are downloading from the mirror
            // should always be lower or equal than maximum allowed number
            // of connection to a single host.
            assert(dd->max_connection_per_host == -1 ||
                   c_mirror->running_transfers <= dd->max_connection_per_host);

            if (dd->max_connection_per_host == -1 ||
                c_mirror->running_transfers < dd->max_connection_per_host)
            {
                // Use this mirror
                mirror = c_mirror;
                break;
            }
        }

        if (mirror) {
            // Suitable (untried and with available capacity) mirror found
            full_url = lr_pathconcat(mirror->mirror->url,
                                     target->target->path,
                                     NULL);
        } else if (!at_least_one_suitable_mirror_found) {
            // No suitable mirror even exists => Set transfer as failed
            g_debug("%s: All mirrors were tried without success", __func__);
            target->state = LR_DS_FAILED;
            lr_downloadtarget_set_error(target->target, LRE_NOURL,
                        "Cannot download, all mirrors were already tried "
                        "without success");
            if (dd->failfast) {
                g_set_error(err, LR_DOWNLOADER_ERROR, LRE_NOURL,
                            "Cannot download %s: All mirrors were tried",
                            target->target->path);
                return FALSE;
            }
            return TRUE;
        }
    }

    if (!full_url) {
        // No free mirror
        g_debug("%s: No free mirror", __func__);
        return TRUE;
    }

    g_debug("%s: URL: %s", __func__, full_url);

    // Prepare CURL easy handle
    CURLcode c_rc;
    CURL *h;
    if (dd->lr_handle)
        h = curl_easy_duphandle(dd->lr_handle->curl_handle);
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

    lr_free(full_url);

    // Prepare FILE
    int fd;

    if (target->target->fd != -1) {
        // Use supplied filedescriptor
        fd = dup(target->target->fd);
        if (fd == -1) {
            g_set_error(err, LR_DOWNLOADER_ERROR, LRE_IO,
                        "dup(%d) failed: %s",
                        target->target->fd, strerror(errno));
            curl_easy_cleanup(h);
            return FALSE;
        }
    } else {
        // Use supplied filename
        int open_flags = O_CREAT|O_TRUNC|O_RDWR;
        if (target->target->resume)
            open_flags &= ~O_TRUNC;

        fd = open(target->target->fn, open_flags, 0666);
        if (fd < 0) {
            g_set_error(err, LR_DOWNLOADER_ERROR, LRE_IO,
                        "Cannot open %s: %s",
                        target->target->fn, strerror(errno));
            curl_easy_cleanup(h);
            return FALSE;
        }
    }

    FILE *f = fdopen(fd, "w+b");
    if (!f) {
        g_set_error(err, LR_DOWNLOADER_ERROR, LRE_IO,
                    "fdopen(%d) failed: %s",
                    fd, strerror(errno));
        curl_easy_cleanup(h);
        return FALSE;
    }

    target->f = f;

    // Set fd to Curl handle
    c_rc = curl_easy_setopt(h, CURLOPT_WRITEDATA, f);
    if (c_rc != CURLE_OK) {
        g_set_error(err, LR_DOWNLOADER_ERROR, LRE_CURL,
                    "curl_easy_setopt(h, CURLOPT_WRITEDATA, f) failed: %s",
                    curl_easy_strerror(c_rc));
        fclose(f);
        curl_easy_cleanup(h);
        return FALSE;
    }

    // Resume - set offset to resume incomplete download
    if (target->target->resume) {
        gint64 used_offset;

        // Determine offset
        if (target->original_offset == -1) {
            fseek(f, 0L, SEEK_END);
            used_offset = ftell(f);
            if (used_offset == -1) {
                // An error while determining offset =>
                // Download the whole file again
                used_offset = 0;
            }
            target->original_offset = used_offset;
        } else {
            used_offset = target->original_offset;
        }

        g_debug("%s: Used offset for download resume: %"G_GINT64_FORMAT,
                __func__, used_offset);

        c_rc = curl_easy_setopt(h, CURLOPT_RESUME_FROM_LARGE,
                                (curl_off_t) used_offset);
        if (c_rc != CURLE_OK) {
            g_set_error(err, LR_DOWNLOADER_ERROR, LRE_CURL,
                        "curl_easy_setopt(h, LR_DOWNLOADER_ERROR, %"
                        G_GINT64_FORMAT") failed: %s",
                        used_offset, curl_easy_strerror(c_rc));
            fclose(f);
            curl_easy_cleanup(h);
            return FALSE;
        }
    }

    // Prepare progress callback
    if (target->target->progresscb) {
        curl_easy_setopt(h, CURLOPT_PROGRESSFUNCTION, lr_progresscb);
        curl_easy_setopt(h, CURLOPT_NOPROGRESS, 0);
        curl_easy_setopt(h, CURLOPT_PROGRESSDATA, target);
    }

    // Add the new handle to the curl multi handle
    curl_multi_add_handle(dd->multi_handle, h);

    // Set the state of transfer as running
    target->state = LR_DS_RUNNING;

    // Set mirror for the target
    target->mirror = mirror;  // mirror could be NULL if baseurl is used

    // Save curl handle for the current transfer
    target->curl_handle = h;

    // Add the transfer to the list of running transfers
    dd->running_transfers = g_slist_append(dd->running_transfers, target);

    return TRUE;
}

static gboolean
check_transfer_statuses(LrDownload *dd, GError **err)
{
    assert(dd);
    assert(!err || *err == NULL);

    int freed_transfers = 0;
    int msgs_in_queue;
    CURLMsg *msg;

    while (msg = curl_multi_info_read(dd->multi_handle, &msgs_in_queue)) {
        LrTarget *target = NULL;
        char *effective_url = NULL;
        GError *tmp_err = NULL;

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

        // Check status of finished transfer
        if (msg->data.result != CURLE_OK) {
            // There was an error that is reported by CURLcode
            g_set_error(&tmp_err, LR_DOWNLOADER_ERROR, LRE_CURL,
                        "Curl error: %s for %s",
                        curl_easy_strerror(msg->data.result),
                        effective_url);
        } else {
            // curl return code is CURLE_OK but we need to check status code
            long code;
            curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &code);

            // If no code, assume success

            if (code) {
                if (effective_url && g_str_has_prefix(effective_url, "http")) {
                    // Check HTTP(S) code
                    if (code/100 != 2) {
                        g_set_error(&tmp_err,
                                    LR_DOWNLOADER_ERROR,
                                    LRE_BADSTATUS,
                                    "Status code: %ld for %s",
                                    code, effective_url);
                    }
                } else if (effective_url) {
                    // Check FTP
                    if (code/100 != 2) {
                        g_set_error(&tmp_err,
                                    LR_DOWNLOADER_ERROR,
                                    LRE_BADSTATUS,
                                    "Status code: %ld for %s",
                                    code, effective_url);
                    }
                } else {
                    g_set_error(&tmp_err,
                                LR_DOWNLOADER_ERROR,
                                LRE_BADSTATUS,
                                "Status code: %ld", code);
                }
            }
        }

        // Clean stuff after the current handle
        curl_multi_remove_handle(dd->multi_handle, target->curl_handle);
        curl_easy_cleanup(target->curl_handle);
        target->curl_handle = NULL;
        dd->running_transfers = g_slist_remove(dd->running_transfers,
                                               (gconstpointer) target);
        target->tried_mirrors = g_slist_append(target->tried_mirrors,
                                               target->mirror);

        guint num_of_tried_mirrors = g_slist_length(target->tried_mirrors);

        // Checksum checking
        if (!tmp_err
            && target->target->checksum
            && target->target->checksumtype != LR_CHECKSUM_UNKNOWN)
        {
            int fd;
            gboolean ret, matches;

            fflush(target->f);
            fd = fileno(target->f);
            lseek(fd, 0, SEEK_SET);

            ret = lr_checksum_fd_cmp(target->target->checksumtype,
                                     fd,
                                     target->target->checksum,
                                     1,
                                     &matches,
                                     &tmp_err);
            if (ret && !matches) {
                // Checksums doesn't match
                g_set_error(&tmp_err,
                        LR_DOWNLOADER_ERROR,
                        LRE_BADCHECKSUM,
                        "Downloading successfull, but checksum doesn't match");
            } else if (ret == FALSE) {
                // Error while checksum calculation
                g_propagate_prefixed_error(err, tmp_err, "Downloading from %s "
                        "was successfull but error encountered while "
                        "checksuming: ", effective_url);
                fclose(target->f);
                target->f = NULL;
                lr_free(effective_url);
                return FALSE;
            }
        }

        fclose(target->f);
        target->f = NULL;

        GError *fail_fast_error = NULL;

        if (tmp_err) {
            // There was an error during transfer

            g_debug("%s: Error during transfer: %s", __func__, tmp_err->message);

            int complete_url_in_path = strstr(target->target->path, "://") ? 1 : 0;

            if (!complete_url_in_path
                && !target->target->baseurl
                && (dd->max_mirrors_to_try <= 0
                    || num_of_tried_mirrors < dd->max_mirrors_to_try))
            {
                // Try another mirror
                g_debug("%s: Ignore error - Try another mirror", __func__);
                target->state = LR_DS_WAITING;
                g_error_free(tmp_err);  // Ignore the error
            } else {
                // No more retry (or baseurl used) => set target as failed
                g_debug("%s: No more retries (tried: %d)",
                        __func__, num_of_tried_mirrors);
                target->state = LR_DS_FAILED;
                lr_downloadtarget_set_error(target->target,
                                            tmp_err->code,
                                            "Download failed: %s",
                                            tmp_err->message);
                if (dd->failfast)
                    g_propagate_error(&fail_fast_error, tmp_err);
                else
                    g_error_free(tmp_err);
            }

            // Truncate file - remove downloaded garbage (error html page etc.)
            off_t original_offset;
            if (target->original_offset > -1)
                // If resume enabled, truncate file to its original position
                original_offset = target->original_offset;
            else
                // If no resume enabled, just truncate whole file
                original_offset = 0;

            int rc;
            if (target->target->fn)
                rc = truncate(target->target->fn, original_offset);
            else
                rc = ftruncate(target->target->fd, original_offset);

            if (rc == -1) {
                lr_free(effective_url);
                g_set_error(err, LR_DOWNLOADER_ERROR, LRE_IO,
                            "ftruncate() failed: %s", strerror(errno));
                return FALSE;
            }

            if (!target->target->fn) {
                // In case fd is used, seek to the original offset
                off_t rc_offset = lseek(target->target->fd,
                                        original_offset,
                                        SEEK_SET);
                if (rc_offset == -1) {
                    lr_free(effective_url);
                    g_set_error(err, LR_DOWNLOADER_ERROR, LRE_IO,
                                "lseek() failed: %s", strerror(errno));
                    return FALSE;
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
        }

        lr_free(effective_url);
        freed_transfers++;

        if (fail_fast_error) {
            // A single download failed - interrupt whole downloading
            g_propagate_error(err, fail_fast_error);
            return FALSE;
        }
    }

    // At this point, after handles of finished transfers were removed
    // from the multi_handle, we could add new waiting transfers.

    for (int x = 0; x < freed_transfers; x++) {
        gboolean rc = prepare_next_transfer(dd, err);
        if (!rc)
            return FALSE;
    }

    return TRUE;
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
                            "select() error: %s", strerror(errno));
                return FALSE;
            }
        }

        // This do-while loop is important. Because if curl_multi_perform sets
        // still_running to 0, we need to check if there are any next
        // transfers available (we need to call check_transfer_statuses).
        // Because if there will be no available next transfers and the
        // curl multi handle is empty (all transfers already
        // finished - this is what still_running == 0 is meaning),
        // then next iteration of main downloding loop cause a 1sec waiting
        // on the select() call.
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
lr_download(LrHandle *lr_handle,
            GSList *targets,
            gboolean failfast,
            GError **err)
{
    gboolean ret;
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

    // Prepare download data
    dd.failfast = failfast;
    dd.max_parallel_connections = lr_handle->maxparalleldownloads;
    dd.max_connection_per_host = lr_handle->maxdownloadspermirror;
    dd.max_mirrors_to_try = (lr_handle) ? lr_handle->maxmirrortries : 0;
    dd.lr_handle = lr_handle;

    dd.multi_handle = curl_multi_init();
    if (!dd.multi_handle) {
        // Something went wrong
        g_set_error(err, LR_DOWNLOADER_ERROR, LRE_CURLM,
                    "curl_multi_init() call failed");
        return FALSE;
    }

    // Prepare list of LrMirrors
    dd.mirrors = NULL;
    for (GSList *elem = (lr_handle) ? lr_handle->internal_mirrorlist : NULL;
         elem;
         elem = g_slist_next(elem))
    {
        LrInternalMirror *imirror = elem->data;

        assert(imirror);
        assert(imirror->url);
        g_debug("%s: Mirror: %s", __func__, imirror->url);

        LrMirror *mirror = lr_malloc0(sizeof(*mirror));
        mirror->mirror = imirror;
        dd.mirrors = g_slist_append(dd.mirrors, mirror);
    }

    // Prepare list of LrTargets
    dd.targets = NULL;
    for (GSList *elem = targets; elem; elem = g_slist_next(elem)) {
        LrDownloadTarget *dtarget = elem->data;

        assert(dtarget);
        assert(dtarget->path);
        assert((dtarget->fd > 0 && !dtarget->fn) || (dtarget->fd < 0 && dtarget->fn));
        g_debug("%s: Target: %s (%s)", __func__,
                dtarget->path,
                (dtarget->baseurl) ? dtarget->baseurl : "-");

        LrTarget *target = lr_malloc0(sizeof(*target));
        target->state           = LR_DS_WAITING;
        target->target          = dtarget;
        target->original_offset = -1;
        target->target->rcode   = LRE_UNFINISHED;
        target->target->err     = "Not finished";
        dd.targets = g_slist_append(dd.targets, target);
    }

    dd.running_transfers = NULL;

    // Prepare the first set of transfers
    for (int x = 0; x < dd.max_parallel_connections; x++) {
        ret = prepare_next_transfer(&dd, &tmp_err);
        if (!ret)
            goto lr_download_cleanup;
    }

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

            lr_downloadtarget_set_error(target->target, LRE_UNFINISHED,
                    "Not finnished - interrupted by error: %s",
                    tmp_err->message);
        }

        g_slist_free(dd.running_transfers);
        dd.running_transfers = NULL;

        g_propagate_error(err, tmp_err);
    }

    assert(dd.running_transfers == NULL);

    curl_multi_cleanup(dd.multi_handle);

    for (GSList *elem = dd.mirrors; elem; elem = g_slist_next(elem)) {
        LrMirror *mirror = elem->data;
        lr_free(mirror);
    }
    g_slist_free(dd.mirrors);

    for (GSList *elem = dd.targets; elem; elem = g_slist_next(elem)) {
        LrTarget *target = elem->data;
        assert(target->curl_handle == NULL);
        assert(target->f == NULL);
        g_slist_free(target->tried_mirrors);
        lr_free(target);
    }
    g_slist_free(dd.targets);

    return ret;
}

gboolean
lr_download_target(LrHandle *lr_handle,
                   LrDownloadTarget *target,
                   GError **err)
{
    gboolean ret;
    GSList *list = NULL;

    assert(!err || *err == NULL);

    if (!target)
        return TRUE;

    list = g_slist_prepend(list, target);

    ret = lr_download(lr_handle, list, TRUE, err);

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

    target = lr_downloadtarget_new(url, NULL, fd, NULL, LR_CHECKSUM_UNKNOWN,
                                    NULL, 0, 0, NULL, NULL, NULL);

    ret = lr_download_target(lr_handle, target, &tmp_err);

    assert(ret || tmp_err);
    assert(!(target->err) || !ret);

    if (!ret) {
        g_propagate_error(err, tmp_err);
    }

    lr_downloadtarget_free(target);

    return ret;
}

typedef struct {
    LrProgressCb cb; /*!<
        User callback */

    void *cbdata; /*!<
        User callback data */

    GSList *singlecbdata; /*!<
        List of LrCallbackData */

} LrSharedCallbackData;

typedef struct {
    double downloaded;
    double total;
    LrSharedCallbackData *sharedcbdata;
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
    return shared_cbdata->cb(shared_cbdata->cbdata,
                             totalsize,
                             downloaded);
}

gboolean
lr_download_single_cb(LrHandle *lr_handle,
                      GSList *targets,
                      gboolean failfast,
                      LrProgressCb cb,
                      void *cbdata,
                      GError **err)
{
    gboolean ret;
    LrSharedCallbackData shared_cbdata;

    assert(!err || *err == NULL);

    shared_cbdata.cb                 = cb;
    shared_cbdata.cbdata             = cbdata;
    shared_cbdata.singlecbdata       = NULL;

    // "Inject" callbacks and callback data to the targets
    for (GSList *elem = targets; elem; elem = g_slist_next(elem)) {
        LrDownloadTarget *target = elem->data;

        LrCallbackData *cbdata = lr_malloc(sizeof(*cbdata));
        cbdata->downloaded        = 0.0;
        cbdata->sharedcbdata      = &shared_cbdata;

        target->progresscb  = (cb) ? lr_multi_progress_func : NULL;
        target->cbdata      = cbdata;

        shared_cbdata.singlecbdata = g_slist_append(shared_cbdata.singlecbdata,
                                                    cbdata);
    }

    ret = lr_download(lr_handle, targets, failfast, err);

    // Remove callbacks and callback data
    for (GSList *elem = targets; elem; elem = g_slist_next(elem)) {
        LrDownloadTarget *target = elem->data;
        lr_free(target->cbdata);
        target->cbdata = NULL;
        target->progresscb = NULL;
    }

    return ret;
}
