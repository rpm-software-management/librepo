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

#define _POSIX_SOURCE   200112L
#define _BSD_SOURCE

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <curl/curl.h>

#include "setup.h"
#include "curl.h"
#include "rcodes.h"
#include "util.h"
#include "handle_internal.h"
#include "curltargetlist.h"

/* Callback stuff */

struct _lr_SharedCallbackData {
    short *counted;     /*!< wich downloads have already been included into
                             the total_size*/
    int count;          /*!< number of downloaded files*/
    int advertise;      /*!< advertise total_size -
                             0 if at least one download has unknown total size */
    double downloaded;  /*!< yet downloaded  */
    double total_size;  /*!< total size to download */

    lr_ProgressCb cb;   /*!< pointer to user callback */
    void *user_data;    /*!< user callback data */
};
typedef struct _lr_SharedCallbackData * lr_SharedCallbackData;

struct _lr_CallbackData {
    int id;                         /*!< id of the current callback*/
    double downloaded;              /*!< yet downloaded */
    lr_SharedCallbackData scb_data; /*!< shared callback data */
};
typedef struct _lr_CallbackData * lr_CallbackData;

int
lr_progress_func(void* ptr,
                 double total_to_download,
                 double now_downloaded,
                 double total_to_upload,
                 double now_uploaded)
{
    double delta;
    double total_size = 0.0;
    lr_CallbackData cb_data = ptr;
    lr_SharedCallbackData scd_data = cb_data->scb_data;

    LR_UNUSED(total_to_upload);
    LR_UNUSED(now_uploaded);

    if (total_to_download) {
        if (!scd_data->counted[cb_data->id]) {
            int advertise = 1;
            scd_data->counted[cb_data->id] = 1;
            scd_data->total_size += total_to_download;
            for (int x = 0; x < scd_data->count; x++)
                if (scd_data->counted[x] == 0) advertise = 0;
            scd_data->advertise = advertise;
        }
    }


    delta = now_downloaded - cb_data->downloaded;
    cb_data->downloaded = now_downloaded;
    assert(delta >= 0.0);
    if (delta < 0.001)
        return 0;  /* Little step - Do not advertize */
    scd_data->downloaded += delta;
    total_size = scd_data->advertise ? scd_data->total_size : 0.0;

    /* Call user callback */
    DEBUGASSERT(scd_data->cb);
    return scd_data->cb(scd_data->user_data, total_size, scd_data->downloaded);
}

/* End of callback stuff */

int
lr_curl_single_download(lr_Handle handle,
                        const char *url,
                        int fd)
{
    CURLcode c_rc = CURLE_OK;
    CURL *c_h = NULL;
    FILE *f = NULL;
    long status_code = 0;

    if (!url)
        return LRE_NOURL;

    c_h = curl_easy_duphandle(handle->curl_handle);
    if (!c_h)
        return LRE_CURLDUP;

    c_rc = curl_easy_setopt(c_h, CURLOPT_URL, url);
    if (c_rc != CURLE_OK) {
        curl_easy_cleanup(c_h);
        handle->last_curl_error = c_rc;
        return LRE_CURL;
    }

    f = fdopen(dup(fd), "w");
    if (!f) {
        curl_easy_cleanup(c_h);
        return LRE_IO;
    }

    c_rc = curl_easy_setopt(c_h, CURLOPT_WRITEDATA, f);
    if (c_rc != CURLE_OK) {
        curl_easy_cleanup(c_h);
        handle->last_curl_error = c_rc;
        fclose(f);
        return LRE_CURL;
    }

    c_rc = curl_easy_perform(c_h);
    if (c_rc != CURLE_OK) {
        curl_easy_cleanup(c_h);
        handle->last_curl_error = c_rc;
        fclose(f);
        return LRE_CURL;
    }

    fclose(f);

    /* Check status code */
    curl_easy_getinfo(c_h, CURLINFO_RESPONSE_CODE, &status_code);
    if (status_code && (status_code != 200 && status_code != 226)) {
        handle->status_code = status_code;
        curl_easy_cleanup(c_h);
        return LRE_BADSTATUS;
    }

    curl_easy_cleanup(c_h);

    return LRE_OK;
}

int
lr_curl_single_mirrored_download(lr_Handle handle,
                                 const char *filename,
                                 int fd,
                                 lr_ChecksumType checksum_type,
                                 const char *checksum)
{
    int rc;
    int mirrors;
    lr_InternalMirrorlist iml = handle->internal_mirrorlist;

    if (!iml)
        return LRE_NOURL;

    mirrors = lr_internalmirrorlist_len(handle->internal_mirrorlist);
    if (mirrors < 1)
        return LRE_NOURL;

    DEBUGF(fprintf(stderr, "Downloading %s\n", filename));

    for (int x=0; x < mirrors; x++) {
        char *full_url;
        char *url = lr_internalmirrorlist_get_url(iml, x);
        DEBUGF(fprintf(stderr, "Trying mirror: %s\n", url));

        lseek(fd, 0, SEEK_SET);
        ftruncate(fd, 0);

        full_url = lr_pathconcat(url, filename, NULL);
        rc = lr_curl_single_download(handle, full_url, fd);
        lr_free(full_url);

        if (rc == LRE_OK) {
            /* Download successful */
            DEBUGF(fprintf(stderr, "Download successful\n"));

            /* Check checksum */
            if (checksum && checksum_type != LR_CHECKSUM_UNKNOWN) {
                DEBUGF(fprintf(stderr, "Checking checksum\n"));
                lseek(fd, 0, SEEK_SET);
                if (lr_checksum_fd_cmp(checksum_type, fd, checksum)) {
                    DEBUGF(fprintf(stderr, "Bad checksum\n"));
                    rc = LRE_BADCHECKSUM;
                    continue; /* Try next mirror */
                }
            }

            /* Store used mirror into the handler */
            handle->used_mirror = lr_strdup(url);
            break;
        } else {
            DEBUGF(fprintf(stderr, "Download rc: %d\n", rc));
        }
    }

    return rc;
}

int
lr_curl_multi_download(lr_Handle handle, lr_CurlTargetList targets)
{
    int not = lr_curltargetlist_len(targets);  /* Number Of Targets */
    int nom;            /* Number Of Mirrors */
    int ret = LRE_OK;
    CURL *curl_easy_interfaces[not];
    FILE *open_files[not];
    CURLMcode cm_rc;
    CURLM *cm_h = NULL; /* Curl Multi Handle */
    lr_InternalMirrorlist iml;

    struct _lr_SharedCallbackData shared_cb_data;
    struct _lr_CallbackData cb_data[not];

    assert(handle);

    iml = handle->internal_mirrorlist;
    if (!iml)
        return LRE_NOURL;

    nom = lr_internalmirrorlist_len(iml);
    if (nom < 1)
        return LRE_NOURL;

    if (not == 0) {
        /* Maybe user callback shoud be called here */
        return LRE_OK;
    }

    /* Initialize shared callback data */
    shared_cb_data.counted = lr_malloc0(sizeof(short) * not);
    shared_cb_data.count = not;
    shared_cb_data.advertise = 0;
    shared_cb_data.downloaded = 0;
    shared_cb_data.total_size = 0;
    shared_cb_data.cb = handle->user_cb;
    shared_cb_data.user_data = handle->user_data;

    /* --- Since here it shoud be safe to use "goto cleanup" --- */

    for (int i = 0; i < nom; i++) {
        int used = 0;
        char *mirror = lr_internalmirrorlist_get_url(iml, i);
        int still_running;  /* Number of still running downloads */
        int failed_downloads = 0;
        CURLMsg *msg;  /* for picking up messages with the transfer status */
        int msgs_left; /* how many messages are left */

        DEBUGF(fprintf(stderr, "Using mirror %s\n", mirror));

        cm_h = curl_multi_init();
        if (!cm_h) {
            ret = LRE_CURLDUP;
            goto cleanup;
        }

        /*  Prepare curl_easy_handlers for every file */
        DEBUGF(fprintf(stderr, "CURL multi handle targets:\n"));
        for (int x = 0; x < not; x++) {
            FILE *f;
            char *url;
            CURL *c_h;
            CURLcode c_rc;
            lr_CurlTarget t = lr_curltargetlist_get(targets, x);

            curl_easy_interfaces[x] = NULL;
            open_files[x] = NULL;

            if (t->downloaded)
                /* Already downloaded */
                continue;

            c_h = curl_easy_duphandle(handle->curl_handle);
            if (!c_h) {
                ret = LRE_CURLDUP;
                DEBUGF(fprintf(stderr, "Cannot dup CURL handle\n"));
                goto cleanup;
            }
            curl_easy_interfaces[x] = c_h;

            lseek(t->fd, 0, SEEK_SET);
            ftruncate(t->fd, 0);
            f = fdopen(dup(t->fd), "w");
            if (!f) {
                ret = LRE_IO;
                DEBUGF(fprintf(stderr, "Cannot dup fd %d\n", t->fd));
                goto cleanup;
            }
            open_files[x] = f;

            url = lr_pathconcat(mirror, t->path, NULL);
            DEBUGF(fprintf(stderr, "  %s\n", url));

            c_rc = curl_easy_setopt(c_h, CURLOPT_URL, url);
            lr_free(url);
            if (c_rc != CURLE_OK) {
                ret = LRE_CURLDUP;
                DEBUGF(fprintf(stderr, "Cannot set CURLOPT_URL\n"));
                goto cleanup;
            }

            c_rc = curl_easy_setopt(c_h, CURLOPT_WRITEDATA, f);
            if (c_rc != CURLE_OK) {
                ret = LRE_CURLDUP;
                DEBUGF(fprintf(stderr, "Cannot set CURLOPT_WRITEDATA\n"));
                goto cleanup;
            }

            /* Prepare callback and its data */
            if (handle->user_cb) {
                lr_CallbackData data = &cb_data[x];
                data->id = x;
                data->downloaded = 0.0;
                data->scb_data = &shared_cb_data;
                c_rc = curl_easy_setopt(c_h, CURLOPT_PROGRESSFUNCTION, lr_progress_func);
                c_rc = curl_easy_setopt(c_h, CURLOPT_NOPROGRESS, 0);
                c_rc = curl_easy_setopt(c_h, CURLOPT_PROGRESSDATA, data);
            }

            cm_rc = curl_multi_add_handle(cm_h, c_h);
            if (cm_rc != CURLM_OK) {
                handle->last_curlm_error = cm_rc;
                ret = LRE_CURLM;
                DEBUGF(fprintf(stderr, "Cannot add curl_easy hadle to multi handle\n"));
                goto cleanup;
            }
            used++;
        }

        if (used == 0) {
            DEBUGF(fprintf(stderr, "All files were downloaded\n"));
            break;  /* Nothing to download */
        }

        /* Perform */
        DEBUGF(fprintf(stderr, "curl_multi_perform\n"));
        curl_multi_perform(cm_h, &still_running);

        do {
            int rc;
            int maxfd = -1;
            long curl_timeo = -1;
            fd_set fdread;
            fd_set fdwrite;
            fd_set fdexcep;
            struct timeval timeout;

            FD_ZERO(&fdread);
            FD_ZERO(&fdwrite);
            FD_ZERO(&fdexcep);

            /* Set timeout */
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            curl_multi_timeout(cm_h, &curl_timeo);
            if (curl_timeo >= 0) {
                timeout.tv_sec = curl_timeo / 1000;
                if (timeout.tv_sec > 1)
                    timeout.tv_sec = 1;
                else
                    timeout.tv_usec = (curl_timeo % 1000) * 1000;
            }

            /* Get filedescriptors from the transfers */
            cm_rc = curl_multi_fdset(cm_h, &fdread, &fdwrite, &fdexcep, &maxfd);
            if (cm_rc != CURLM_OK) {
                DEBUGF(fprintf(stderr, "curl_multi_fdset() error: %d\n", cm_rc));
                handle->last_curlm_error = cm_rc;
                ret = LRE_CURLM;
                goto cleanup;
            }

            rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);

            switch (rc) {
            case -1:
                break; /* select() error */

            case 0:
            default:
                /* Timeout or readable/writeable sockets */
                curl_multi_perform(cm_h, &still_running);
                break;
            }
        } while (still_running);

        /* Check download statuses */
        while ((msg = curl_multi_info_read(cm_h, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE) {
                int failed = 0;
                int idx, found = 0;
                lr_CurlTarget t = NULL;

                /* Find out which handle this message is about */
                for (idx=0; idx < not; idx++) {
                    found = (msg->easy_handle == curl_easy_interfaces[idx]);
                    t = lr_curltargetlist_get(targets, idx);
                    if(found)
                        break;
                }

                DEBUGF(fprintf(stderr, "Download status: %d (%s)\n",
                               msg->data.result,
                               t->path));

                if (msg->data.result == CURLE_OK) {
                    long code = 0; // HTTP or FTP code
                    curl_easy_getinfo (msg->easy_handle, CURLINFO_RESPONSE_CODE, &code);
                    //if (code == 200 && msg->data.result != CURLE_ABORTED_BY_CALLBACK) {
                    if (code == 200) {
                        // Succeeded
                        t->downloaded = 1;
                    } else {
                        DEBUGF(fprintf(stderr, "Bad HTTP/FTP code: %ld\n", code));
                        failed = 1;
                    }
                } else
                    failed = 1;

                if (failed) {
                    failed_downloads++;
                    /* Update total_to_download in  callback data */
                    shared_cb_data.counted[idx] = 0;
                }
            }
        }

        /* Cleanup */
        curl_multi_cleanup(cm_h);
        cm_h = NULL;
        for (int x = 0; x < not; x++) {
            if (curl_easy_interfaces[x]) {
                curl_easy_cleanup(curl_easy_interfaces[x]);
                curl_easy_interfaces[x] = NULL;
            }
            if (open_files[x]) {
                fclose(open_files[x]);
                open_files[x] = NULL;
            }
        }

        if (failed_downloads == 0)
            break;

    } /* End of iteration over mirrors */

cleanup:
    DEBUGF(fprintf(stderr, "Cleanup\n"));

    lr_free(shared_cb_data.counted);
    if (cm_h)
        curl_multi_cleanup(cm_h);
    for (int x = 0; x < not; x++) {
        if (curl_easy_interfaces[x])
            curl_easy_cleanup(curl_easy_interfaces[x]);
        if (open_files[x])
            fclose(open_files[x]);
    }

    return ret;
}

