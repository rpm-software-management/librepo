#define _POSIX_SOURCE

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <curl/curl.h>

#include "setup.h"
#include "curl.h"
#include "librepo.h"
#include "util.h"
#include "handle_internal.h"

/* Callback stuff */

struct _lr_SharedCallbackData {
    short *counted;     /*!< wich downloads have already been included into
                             the total_size*/
    int count;          /*!< number of downloaded files*/
    int advertise;      /*!< advertise total_size -
                             0 if at least one download has unknown total size */
    double downloaded;  /*!< yet downloaded  */
    double total_size;  /*!< total size to download */

    lr_progress_cb cb;  /*!< pointer to user callback */
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
                        int fd,
                        char *mandatory_suffix)
{
    CURLcode c_rc = CURLE_OK;
    CURL *c_h = NULL;
    FILE *f = NULL;
    char *full_url = NULL;
    long status_code = 0;

    if (!url)
        return LRE_NOURL;

    c_h = curl_easy_duphandle(handle->curl_handle);
    if (!c_h)
        return LRE_CURL_DUP;

    /* Append mandatory_suffix_if_necessary */
    if (mandatory_suffix && !lr_ends_with(url, mandatory_suffix)) {
        size_t url_len = strlen(url);
        size_t len = url_len + strlen(mandatory_suffix) + 2;

        full_url = lr_malloc(sizeof(char) * len);
        strcpy(full_url, url);
        full_url[url_len] = '/';
        strcpy(full_url + url_len + 1, mandatory_suffix);
    } else
        full_url = (char *) url;

    c_rc = curl_easy_setopt(c_h, CURLOPT_URL, full_url);
    if (c_rc != CURLE_OK) {
        curl_easy_cleanup(c_h);
        handle->last_curl_error = c_rc;
        return LRE_CURL;
    }

    if (full_url != url)
        lr_free(full_url);

    f = fdopen(dup(fd), "w+");
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
        return LRE_BAD_STATUS;
    }

    curl_easy_cleanup(c_h);

    return LRE_OK;
}

lr_CurlTarget
lr_target_create()
{
    return lr_malloc0(sizeof(struct _lr_CurlTarget));
}

void
lr_target_free(lr_CurlTarget target)
{
    if (!target)
        return;
    lr_free(target);
}

int
lr_curl_multi_download(lr_Handle handle, lr_CurlTarget targets[], int not)
{
    int ret = LRE_OK;
    CURL *curl_easy_interfaces[not];
    FILE *open_files[not];
    CURLMcode cm_rc;
    CURLM *cm_h = curl_multi_init();    /* Curl Multi Handle */
    int still_running;                  /* Number of still running downloads */

    struct _lr_SharedCallbackData shared_cb_data;
    struct _lr_CallbackData cb_data[not];

    if (!cm_h)
        return LRE_CURL_DUP;

    if (not == 0) {
        /* Maybe user callback shoud be called here */
        return LRE_OK;
    }

    /* Init variables */
    for (int x = 0; x < not; x++) {
        curl_easy_interfaces[x] = NULL;
        open_files[x] = NULL;
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

    /* Prepare curl_easy_handlers for every file */
    for (int x = 0; x < not; x++) {
        FILE *f;
        CURLcode c_rc;
        lr_CurlTarget t = targets[x];
        CURL *c_h = curl_easy_duphandle(handle->curl_handle);

        if (!c_h) {
            ret = LRE_CURL_DUP;
            goto cleanup;
        }
        curl_easy_interfaces[x] = c_h;

        f = fdopen(dup(t->fd), "w+");
        if (!f) {
            ret = LRE_IO;
            goto cleanup;
        }
        open_files[x] = f;

        c_rc = curl_easy_setopt(c_h, CURLOPT_URL, t->url);
        if (c_rc != CURLE_OK) {
            ret = LRE_CURL_DUP;
            goto cleanup;
        }

        c_rc = curl_easy_setopt(c_h, CURLOPT_WRITEDATA, f);
        if (c_rc != CURLE_OK) {
            ret = LRE_CURL_DUP;
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
            goto cleanup;
        }
    }

    /* Perform */
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

cleanup:
    lr_free(shared_cb_data.counted);
    curl_multi_cleanup(cm_h);
    for (int x = 0; x < not; x++) {
        if (curl_easy_interfaces[x])
            curl_easy_cleanup(curl_easy_interfaces[x]);
        if (open_files[x])
            fclose(open_files[x]);
    }

    return ret;
}

