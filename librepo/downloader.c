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

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <curl/curl.h>

#include "downloader.h"
#include "rcodes.h"
#include "util.h"
#include "downloadtarget.h"
#include "downloadtarget_internal.h"
#include "handle.h"
#include "handle_internal.h"
#include "setup.h"

volatile sig_atomic_t lr_interrupt = 0;

void
lr_sigint_handler(int sig)
{
    LR_UNUSED(sig);
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
} lr_DownloadState;

typedef struct {
    lr_LrMirror *mirror; /*!<
        Mirror */
    int running_transfers; /*!<
        How many transfers from this mirror are currently in progres. */
    int successfull_transfers; /*!<
        How many transfers was finished successfully from the mirror. */
    int failed_transfers; /*!<
        How many transfers failed. */
} lr_Mirror;

typedef struct {
    lr_DownloadState state; /*!<
        State of the download (transfer). */
    lr_DownloadTarget *target; /*!<
        Download target */
    lr_Mirror *mirror; /*!<
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
        fdopened file descriptor from lr_DownloadTarget and used
        in curl_handle. */
    GSList *tried_mirrors; /*!<
        List of already tried mirrors. This mirrors won't be tried again. */
    gint64 original_offset; /*!<
        If resume is enabled, this is the determinet offset where to resume
        the downloading. If resume is not enabled, then value is -1. */
} lr_Target;

typedef struct {

    // Configuration

    int max_parallel_connections; /*!<
        Maximal number of parallel downloads. */

    int max_connection_per_host; /*!<
        Maximal number of connections per host. -1 means no limit. */

    int max_mirrors_to_try; /*!<
        Maximal number of mirrors to try. Number <= 0 means no limit. */

    // Data

    lr_Handle lr_handle; /*!<
        Librepo handle */

    CURLM *multi_handle; /*!<
        Curl Multi handle */

    GSList *mirrors; /*!<
        All mirrors (list of pointers to lr_Mirror structures) */

    GSList *targets; /*!<
        List of all targets (list of pointers to lr_Target stuctures) */

    GSList *running_transfers; /*!<
        List of running transfers (list of pointer to lr_Target structures) */

} lr_Download;

/** Schema of structures as used in downloader module:
 *
 * +------------------------------+
 * |           lr_Download        |
 * +------------------------------+
 * | int max_parallel_connections |
 * | int max_connection_par_host  |
 * | int max_mirrors_to_try       |
 * |                              |
 * | lr_Handle lr_handle          |
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
 *  |  |       |         lr_Mirror         |
 *  |  |     +---------------------------+-|
 *  |  |     |         lr_Mirror         | |
 *  |  |   +---------------------------+-| |
 *  |   \->|         lr_Mirror         | | |
 *  |      +---------------------------+ | |    +----------------+
 *  |      | lr_LrMirror *mirror      --------->|   lr_LrMirror  |
 *  |      | int running_transfers     | |   /->+----------------+
 *  |      | int successfull_transfers |-+   |  | char *url      |
 *  |      | int failed_transfers      |     |  | int preference |
 *  |      +---------------------------+     |  | int fails      |
 *  |                                        |  +----------------+
 *  |                                        |
 *  |        +----------------------------+  |
 *  |        |          lr_Target         |  |
 *  |      +----------------------------+-|  |
 *  |      |          lr_Target         | |  |     +--------------------------+
 *  |    +----------------------------+-| |  |  /->|      lr_DownloadTarget   |
 *   \-> |          lr_Target         | | |  |  |  +--------------------------+
 *       +----------------------------+ | |  |  |  | char *path               |
 *       | lr_DownloadState state     | | |  |  |  | char *baseurl            |
 *       | lr_DownloadTarget *target ----------/   | int fd                   |
 *       | lr_Mirror *mirror         -------/      | lr_ChecksumType checks.. |
 *       | CURL *curl_handle          |-+          | char *checksum           |
 *       | FILE *f                    |            | int resume               |
 *       | GSList *tried_mirrors      |            | lr_ProgressCb progresscb |
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
              double total_to_upload,
              double now_uploaded)
{
    lr_Target *target = ptr;

    LR_UNUSED(total_to_upload);
    LR_UNUSED(now_uploaded);

    assert(target);
    assert(target->target);

    if (!target->target->progresscb)
        return 0;

    return target->target->progresscb(target->target->cbdata,
                                      total_to_download,
                                      now_downloaded);
}

static int
prepare_next_transfer(lr_Download *dd, GError **err)
{
    lr_Target *target = NULL;
    lr_Mirror *mirror = NULL;
    char *full_url = NULL;
    int complete_url_in_path = 0;

    assert(dd);
    assert(!err || *err == NULL);

    // Select a waiting target

    for (GSList *elem = dd->targets; elem; elem = g_slist_next(elem)) {
        lr_Target *c_target = elem->data;
        if (c_target->state == LR_DS_WAITING) {
            target = c_target;
            break;
        }
    }

    if (!target)  // No target is waiting
        return LRE_OK;

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
        return LRE_NOURL;
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
            lr_Mirror *c_mirror = elem->data;

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
                        "Cannot download, all mirrors were alredy tried "
                        "without success");
            return LRE_OK;
        }
    }

    if (!full_url) {
        // No free mirror
        g_debug("%s: No free mirror", __func__);
        return LRE_OK;
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
        return LRE_CURL;
    }

    // Set URL
    c_rc = curl_easy_setopt(h, CURLOPT_URL, full_url);
    if (c_rc != CURLE_OK) {
        g_set_error(err, LR_DOWNLOADER_ERROR, LRE_CURL,
                    "curl_easy_setopt(h, CURLOPT_URL, %s) failed: %s",
                    full_url, curl_easy_strerror(c_rc));
        lr_free(full_url);
        curl_easy_cleanup(h);
        return LRE_CURL;
    }

    lr_free(full_url);

    // Open FILE from fd
    int new_fd = dup(target->target->fd);
    if (new_fd == -1) {
        g_set_error(err, LR_DOWNLOADER_ERROR, LRE_IO,
                    "dup(%d) failed: %s",
                    target->target->fd, strerror(errno));
        curl_easy_cleanup(h);
        return LRE_IO;
    }

    FILE *f = fdopen(new_fd, "w+b");
    if (!f) {
        g_set_error(err, LR_DOWNLOADER_ERROR, LRE_IO,
                    "fdopen(%d) failed: %s",
                    new_fd, strerror(errno));
        curl_easy_cleanup(h);
        return LRE_IO;
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
        return LRE_CURL;
    }

    // Resume - set offset to resume incomplete download
    if (target->target->resume != 0) {
        gint64 used_offset = target->target->resume;

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
            return LRE_CURL;
        }
    }

    // Prepare callback
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

    return LRE_OK;
}

static int
check_transfer_statuses(lr_Download *dd, GError **err)
{
    assert(dd);
    assert(!err || *err == NULL);

    int freed_transfers = 0;
    int msgs_in_queue;
    CURLMsg *msg;

    while (msg = curl_multi_info_read(dd->multi_handle, &msgs_in_queue)) {
        lr_Target *target = NULL;
        char *effective_url = NULL;
        GError *tmp_err = NULL;

        if (msg->msg != CURLMSG_DONE) {
            // We are only interested in messages about finished transfers
            continue;
        }

        // Find the target with this curl easy handle
        for (GSList *elem = dd->running_transfers; elem; elem = g_slist_next(elem)) {
            lr_Target *ltarget = elem->data;
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
                                    "Status code: %d for %s",
                                    code, effective_url);
                    }
                } else if (effective_url) {
                    // Check FTP
                    if (code/100 != 2) {
                        g_set_error(&tmp_err,
                                    LR_DOWNLOADER_ERROR,
                                    LRE_BADSTATUS,
                                    "Status code: %d for %s",
                                    code, effective_url);
                    }
                } else {
                    g_set_error(&tmp_err,
                                LR_DOWNLOADER_ERROR,
                                LRE_BADSTATUS,
                                "Status code: %d", code);
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
            int fd, ret;

            fflush(target->f);
            fd = fileno(target->f);
            lseek(fd, 0, SEEK_SET);

            ret = lr_checksum_fd_cmp(target->target->checksumtype,
                                     fd,
                                     target->target->checksum,
                                     1,
                                     &tmp_err);
            if (ret == 1) {
                // Checksums doesn't match
                g_set_error(&tmp_err,
                        LR_DOWNLOADER_ERROR,
                        LRE_BADCHECKSUM,
                        "Downloading successfull, but checksum doesn't match");
            } else if (ret == -1) {
                // Error while checksum calculation
                int code = tmp_err->code;
                g_propagate_prefixed_error(err, tmp_err, "Downloading from %s "
                        "was successfull but error encountered while "
                        "checksuming: ", effective_url);
                fclose(target->f);
                target->f = NULL;
                lr_free(effective_url);
                return code;
            }
        }

        fclose(target->f);
        target->f = NULL;

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

            int rc = ftruncate(target->target->fd, original_offset);
            if (rc == -1) {
                lr_free(effective_url);
                g_set_error(err, LR_DOWNLOADER_ERROR, LRE_IO,
                            "ftruncate() failed: %s", strerror(errno));
                return LRE_IO;
            }
            off_t rc_offset = lseek(target->target->fd, original_offset, SEEK_SET);
            if (rc_offset == -1) {
                lr_free(effective_url);
                g_set_error(err, LR_DOWNLOADER_ERROR, LRE_IO,
                            "lseek() failed: %s", strerror(errno));
                return LRE_IO;
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
    }

    // At this point, after handles of finished transfers were removed
    // from the multi_handle, we could add new waiting transfers.

    for (int x = 0; x < freed_transfers; x++) {
        int rc = prepare_next_transfer(dd, err);
        if (rc != LRE_OK)
            return rc;
    }

    return LRE_OK;
}

static int
lr_perform(lr_Download *dd, GError **err)
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
        return LRE_CURLM;
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
            return LRE_CURLM;
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
            return LRE_CURLM;
        }

        rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
        if (rc < 0) {
            if (errno == EINTR) {
                g_debug("%s: select() interrupted by signal", __func__);
                //goto retry;
            } else {
                g_set_error(err, LR_DOWNLOADER_ERROR, LRE_SELECT,
                            "select() error: %s", strerror(errno));
                return LRE_SELECT;
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
            if (rc != LRE_OK)
                return rc;

            if (lr_interrupt) {
                g_set_error(err, LR_DOWNLOADER_ERROR, LRE_INTERRUPTED,
                            "Interrupted by signal");
                return LRE_INTERRUPTED;
            }

            // Do curl_multi_perform()
            do { // Before version 7.20.0 CURLM_CALL_MULTI_PERFORM can appear
                cm_rc = curl_multi_perform(dd->multi_handle, &still_running);
            } while (cm_rc == CURLM_CALL_MULTI_PERFORM);

            if (cm_rc != CURLM_OK) {
                g_set_error(err, LR_DOWNLOADER_ERROR, LRE_CURLM,
                            "curl_multi_perform() error: %s",
                            curl_multi_strerror(cm_rc));
                return LRE_CURLM;
            }
        } while (still_running == 0 && dd->running_transfers);
    }

    return check_transfer_statuses(dd, err);
}

int
lr_download(lr_Handle lr_handle, GSList *targets, GError **err)
{
    int ret = LRE_OK;
    lr_Download dd;             // dd stands for Download Data
    GError *tmp_err = NULL;

    assert(!err || *err == NULL);

    if (lr_interrupt) {
        g_set_error(err, LR_DOWNLOADER_ERROR, LRE_INTERRUPTED,
                    "Interrupted by signal");
        return LRE_INTERRUPTED;
    }

    if (!targets) {
        g_debug("%s: No targets", __func__);
        return LRE_OK;
    }

    // Prepare download data
    dd.max_parallel_connections = 3; // TODO
    dd.max_connection_per_host = 2; // TODO
    dd.max_mirrors_to_try = (lr_handle) ? lr_handle->maxmirrortries : 0;
    dd.lr_handle = lr_handle;

    dd.multi_handle = curl_multi_init();
    if (!dd.multi_handle) {
        // Something went wrong
        g_set_error(err, LR_DOWNLOADER_ERROR, LRE_CURLM,
                    "curl_multi_init() call failed");
        return LRE_CURLM;
    }

    // Prepare list of lr_Mirrors
    dd.mirrors = NULL;
    for (GSList *elem = (lr_handle) ? lr_handle->internal_mirrorlist : NULL;
         elem;
         elem = g_slist_next(elem))
    {
        lr_LrMirror *imirror = elem->data;

        assert(imirror);
        assert(imirror->url);
        g_debug("%s: Mirror: %s", __func__, imirror->url);

        lr_Mirror *mirror = lr_malloc0(sizeof(*mirror));
        mirror->mirror = imirror;
        dd.mirrors = g_slist_append(dd.mirrors, mirror);
    }

    // Prepare list of lr_Targets
    dd.targets = NULL;
    for (GSList *elem = targets; elem; elem = g_slist_next(elem)) {
        lr_DownloadTarget *dtarget = elem->data;

        assert(dtarget);
        assert(dtarget->path);
        assert(dtarget->fd > 0);
        g_debug("%s: Target: %s (%s)", __func__,
                dtarget->path,
                (dtarget->baseurl) ? dtarget->baseurl : "-");

        lr_Target *target = lr_malloc0(sizeof(*target));
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
        if (ret != LRE_OK)
            goto lr_download_cleanup;
    }

    // Perform!
    g_debug("%s: Downloading started", __func__);
    ret = lr_perform(&dd, &tmp_err);

lr_download_cleanup:

    if (tmp_err) {
        // If there was an error, stop all transfers that are in progress.
        g_debug("%s: Error while downloading: %s", __func__, tmp_err->message);

        for (GSList *elem = dd.running_transfers; elem; elem = g_slist_next(elem)){
            lr_Target *target = elem->data;

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
        lr_Mirror *mirror = elem->data;
        lr_free(mirror);
    }
    g_slist_free(dd.mirrors);

    for (GSList *elem = dd.targets; elem; elem = g_slist_next(elem)) {
        lr_Target *target = elem->data;
        assert(target->curl_handle == NULL);
        assert(target->f == NULL);
        g_slist_free(target->tried_mirrors);
        lr_free(target);
    }
    g_slist_free(dd.targets);

    return ret;
}

int
lr_download_target(lr_Handle lr_handle,
                   lr_DownloadTarget *target,
                   GError **err)
{
    int ret = LRE_OK;
    GSList *list = NULL;

    assert(!err || *err == NULL);

    if (!target)
        return ret;

    list = g_slist_prepend(list, target);

    ret = lr_download(lr_handle, list, err);

    g_slist_free(list);

    return ret;
}

int
lr_download_url(lr_Handle lr_handle, const char *url, int fd, GError **err)
{
    int ret = LRE_OK;
    lr_DownloadTarget *target;
    GError *tmp_err = NULL;

    assert(url);
    assert(!err || *err == NULL);

    target = lr_downloadtarget_new(url, NULL, fd, LR_CHECKSUM_UNKNOWN,
                                    NULL, 0, NULL, NULL);

    ret = lr_download_target(lr_handle, target, &tmp_err);

    if (tmp_err) {
        ret = tmp_err->code;
        g_propagate_error(err, tmp_err);
    } else if (target->err) {
        ret = target->rcode;
        g_set_error(err, LR_DOWNLOADER_ERROR, target->rcode, target->err);
    }

    lr_downloadtarget_free(target);

    return ret;
}

typedef struct {
    double downloaded; /*!<
        Currently downloaded */

    double total; /*!<
        Total size to download */

    int totalsizereporters; /*!<
        How many transfers reported total size yet */

    int transfers; /*!<
        Number of transfers */

    lr_ProgressCb cb; /*!<
        User callback */

    void *cbdata; /*!<
        User callback data */

} lr_SharedCallbackData;

typedef struct {
    int totalsizereported;
    double downloaded;
    lr_SharedCallbackData *sharedcbdata;
} lr_CallbackData;

int
lr_multi_progress_func(void* ptr,
                       double total_to_download,
                       double now_downloaded)
{
    lr_CallbackData *cbdata = ptr;
    lr_SharedCallbackData *shared_cbdata = cbdata->sharedcbdata;

    if (total_to_download && !cbdata->totalsizereported) {
        // Report total size to the shared callback data
        cbdata->totalsizereported = 1;
        shared_cbdata->total += total_to_download;
        shared_cbdata->totalsizereporters++;
    }

    // Calculate download delta for this transfer
    double delta = now_downloaded - cbdata->downloaded;
    cbdata->downloaded = now_downloaded;

    assert(delta >= 0.0);

    // Update currently downloaded size
    shared_cbdata->downloaded += delta;

    // Prepare total size for a call of the user callback
    double totalsize;
    if (shared_cbdata->totalsizereporters == shared_cbdata->transfers)
        totalsize = shared_cbdata->total;
    else
        totalsize = 0.0;  // Do not repor total size yet

    // Call user callback
    return shared_cbdata->cb(shared_cbdata->cbdata,
                             totalsize,
                             shared_cbdata->downloaded);
}

int
lr_download_single_cb(lr_Handle lr_handle,
                      GSList *targets,
                      lr_ProgressCb cb,
                      void *cbdata,
                      GError **err)
{
    int ret = LRE_OK;
    lr_SharedCallbackData shared_cbdata;

    assert(!err || *err == NULL);

    shared_cbdata.downloaded         = 0.0;
    shared_cbdata.total              = 0.0;
    shared_cbdata.totalsizereporters = 0;
    shared_cbdata.transfers          = g_slist_length(targets);
    shared_cbdata.cb                 = cb;
    shared_cbdata.cbdata             = cbdata;

    // "Inject" callbacks and callback data to the targets
    for (GSList *elem = targets; elem; elem = g_slist_next(elem)) {
        lr_DownloadTarget *target = elem->data;

        lr_CallbackData *cbdata = lr_malloc(sizeof(*cbdata));
        cbdata->totalsizereported = 0;
        cbdata->downloaded        = 0.0;
        cbdata->sharedcbdata      = &shared_cbdata;

        target->progresscb  = (cb) ? lr_multi_progress_func : NULL;
        target->cbdata      = cbdata;
    }

    ret = lr_download(lr_handle, targets, err);

    // Remove callbacks and callback data
    for (GSList *elem = targets; elem; elem = g_slist_next(elem)) {
        lr_DownloadTarget *target = elem->data;
        lr_free(target->cbdata);
        target->cbdata = NULL;
        target->progresscb = NULL;
    }

    return ret;
}
