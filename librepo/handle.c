#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <curl/curl.h>

#include "handle_internal.h"
#include "setup.h"
#include "librepo.h"
#include "util.h"
#include "yum.h"

#define TMP_DIR_TEMPLATE    "librepo-XXXXXX"

lr_Handle
lr_init_handle()
{
    lr_Handle handle;
    CURL *curl = curl_easy_init();

    if (!curl)
        return NULL;

    handle = lr_malloc0(sizeof(struct _lr_Handle));
    handle->curl_handle = curl;
    handle->retries = 1;
    handle->last_curl_error = CURLE_OK;
    handle->yumflags = LR_YUM_FULL;

    /* Default options */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 6);

    return handle;
}

void
lr_free_handle(lr_Handle handle)
{
    if (!handle)
        return;
    if (handle->curl_handle)
        curl_easy_cleanup(handle->curl_handle);
    lr_free(handle->baseurl);
    lr_free(handle->mirrorlist);
    lr_free(handle->used_mirror);
    lr_free(handle->destdir);
    lr_free(handle);
}

int
lr_setopt(lr_Handle handle, lr_Option option, ...)
{
    lr_Rc ret = LRE_OK;
    va_list arg;
    CURLcode c_rc = CURLE_OK;
    CURL *c_h = handle->curl_handle;

    if (!handle)
        return LRE_BAD_FUNCTION_ARGUMENT;

    va_start(arg, option);

    switch (option) {
    case LR_URL:
        handle->baseurl = lr_strdup(va_arg(arg, char *));
        break;

    case LR_MIRRORLIST:
        handle->mirrorlist = lr_strdup(va_arg(arg, char *));
        break;

    case LR_DONTDUP:
        handle->dontdup = va_arg(arg, long);
        break;

    case LR_HTTPAUTH:
        c_rc = curl_easy_setopt(c_h, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
        break;

    case LR_USERPWD:
        c_rc = curl_easy_setopt(c_h, CURLOPT_USERPWD, va_arg(arg, char *));
        break;

    case LR_PROXY:
        c_rc = curl_easy_setopt(c_h, CURLOPT_PROXY, va_arg(arg, char *));
        break;

    case LR_PROXYPORT:
        c_rc = curl_easy_setopt(c_h, CURLOPT_PROXYPORT, va_arg(arg, long));
        break;

    case LR_PROXYSOCK:
        c_rc = curl_easy_setopt(c_h, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS4);
        break;

    case LR_PROXYAUTH:
        c_rc = curl_easy_setopt(c_h, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
        break;

    case LR_PROXYUSERPWD:
        c_rc = curl_easy_setopt(c_h, CURLOPT_PROXYUSERPWD, va_arg(arg, char *));
        break;

    case LR_PROGRESSCB:
        handle->user_cb = va_arg(arg, lr_progress_cb);
        break;

    case LR_PROGRESSDATA:
        handle->user_data = va_arg(arg, void *);
        break;

    case LR_RETRIES:
        handle->retries = va_arg(arg, int);
        if (handle->retries < 1) {
            ret = LRE_BAD_OPTION_ARGUMENT;
            handle->retries = 1;
        }
        break;

    case LR_MAXSPEED:
        c_rc = curl_easy_setopt(c_h, CURLOPT_MAX_RECV_SPEED_LARGE, va_arg(arg, lr_off_t));
        break;

    case LR_DESTDIR:
        handle->destdir = lr_strdup(va_arg(arg, char *));
        break;

    case LR_REPOTYPE:
        handle->repotype = va_arg(arg, lr_Repotype);
        assert(handle->repotype == LR_YUMREPO);
        break;

    case LR_GPGCHECK:
        if (va_arg(arg, int))
            handle->checks |= LR_CHECK_GPG;
        else
            handle->checks &= ~LR_CHECK_GPG;
        break;

    case LR_CHECKSUM:
        if (va_arg(arg, int))
            handle->checks |= LR_CHECK_CHECKSUM;
        else
            handle->checks &= ~LR_CHECK_CHECKSUM;
        break;

    case LR_YUMDOWNLOADFLAGS:
        handle->yumflags = va_arg(arg, lr_YumRepoFlags);
        break;

    default:
        ret = LRE_UNKNOWN_OPTION;
        break;
    };

    /* Handle CURL error return code */
    if (c_rc != CURLE_OK) {
        handle->last_curl_error = c_rc;
        switch (c_rc) {
        case CURLE_FAILED_INIT:
            ret = LRE_CURL_SETOPT;
            break;
        default:
            ret = LRE_CURL;
            break;
        };
    }

    va_end(arg);
    return ret;
}

int
lr_last_curl_error(lr_Handle handle)
{
    assert(handle);
    return handle->last_curl_error;
}

int
lr_last_curlm_error(lr_Handle handle)
{
    assert(handle);
    return handle->last_curlm_error;
}

int
lr_perform(lr_Handle handle, void *repo_ptr)
{
    int rc;
    assert(handle);

    if (!repo_ptr)
        return LRE_BAD_FUNCTION_ARGUMENT;

    if (!handle->baseurl && !handle->mirrorlist)
        return LRE_NOURL;

    if (!handle->destdir && !handle->dontdup) {
        handle->destdir = lr_strdup(TMP_DIR_TEMPLATE);
        if (!mkdtemp(handle->destdir))
            return LRE_CANNOT_CREATE_TMP;
    }

    DEBUGF(fprintf(stderr, "Using dir: %s\n", handle->destdir));

    switch (handle->repotype) {
    case LR_YUMREPO:
        DEBUGF(fprintf(stderr, "Downloading/Locating yum repo\n"));
        rc = lr_yum_perform(handle, repo_ptr);
        break;
    default:
        DEBUGF(fprintf(stderr, "Bad repo type\n"));
        assert(0);
        break;
    };

    return rc;
}

