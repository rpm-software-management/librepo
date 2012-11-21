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

#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <curl/curl.h>

#include "handle_internal.h"
#include "handle.h"
#include "result_internal.h"
#include "repomd.h"
#include "setup.h"
#include "rcodes.h"
#include "util.h"
#include "yum.h"

lr_Handle
lr_handle_init()
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
lr_handle_free(lr_Handle handle)
{
    if (!handle)
        return;
    if (handle->curl_handle)
        curl_easy_cleanup(handle->curl_handle);
    lr_free(handle->baseurl);
    lr_free(handle->mirrorlist);
    lr_free(handle->used_mirror);
    lr_free(handle->destdir);
    lr_internalmirrorlist_free(handle->internal_mirrorlist);
    lr_free(handle);
}

int
lr_handle_setopt(lr_Handle handle, lr_HandleOption option, ...)
{
    lr_Rc ret = LRE_OK;
    va_list arg;
    CURLcode c_rc = CURLE_OK;
    CURL *c_h;

    if (!handle)
        return LRE_BADFUNCARG;

    c_h = handle->curl_handle;

    va_start(arg, option);

    switch (option) {
    case LRO_UPDATE:
        handle->update = va_arg(arg, long) ? 1 : 0;
        break;

    case LRO_URL:
        handle->baseurl = lr_strdup(va_arg(arg, char *));
        break;

    case LRO_MIRRORLIST:
        handle->mirrorlist = lr_strdup(va_arg(arg, char *));
        break;

    case LRO_LOCAL:
        handle->local = va_arg(arg, long) ? 1 : 0;
        break;

    case LRO_HTTPAUTH:
        if (va_arg(arg, long) ==  1)
            c_rc = curl_easy_setopt(c_h, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
        else
            c_rc = curl_easy_setopt(c_h, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
        break;

    case LRO_USERPWD:
        c_rc = curl_easy_setopt(c_h, CURLOPT_USERPWD, va_arg(arg, char *));
        break;

    case LRO_PROXY:
        c_rc = curl_easy_setopt(c_h, CURLOPT_PROXY, va_arg(arg, char *));
        break;

    case LRO_PROXYPORT:
        c_rc = curl_easy_setopt(c_h, CURLOPT_PROXYPORT, va_arg(arg, long));
        break;

    case LRO_PROXYSOCK:
        if (va_arg(arg, long) == 1)
            c_rc = curl_easy_setopt(c_h, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS4);
        else
            c_rc = curl_easy_setopt(c_h, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
        break;

    case LRO_PROXYAUTH:
        if (va_arg(arg, long) == 1)
            c_rc = curl_easy_setopt(c_h, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
        else
            c_rc = curl_easy_setopt(c_h, CURLOPT_PROXYAUTH, CURLAUTH_BASIC);
        break;

    case LRO_PROXYUSERPWD:
        c_rc = curl_easy_setopt(c_h, CURLOPT_PROXYUSERPWD, va_arg(arg, char *));
        break;

    case LRO_PROGRESSCB:
        handle->user_cb = va_arg(arg, lr_ProgressCb);
        break;

    case LRO_PROGRESSDATA:
        handle->user_data = va_arg(arg, void *);
        break;

    case LRO_RETRIES:
        handle->retries = va_arg(arg, long);
        if (handle->retries < 1) {
            ret = LRE_BADOPTARG;
            handle->retries = 1;
        }
        break;

    case LRO_MAXSPEED:
        c_rc = curl_easy_setopt(c_h, CURLOPT_MAX_RECV_SPEED_LARGE, va_arg(arg, lr_off_t));
        break;

    case LRO_DESTDIR:
        handle->destdir = lr_strdup(va_arg(arg, char *));
        break;

    case LRO_REPOTYPE:
        handle->repotype = va_arg(arg, lr_Repotype);
        assert(handle->repotype == LR_YUMREPO);
        break;

    case LRO_GPGCHECK:
        if (va_arg(arg, int))
            handle->checks |= LR_CHECK_GPG;
        else
            handle->checks &= ~LR_CHECK_GPG;
        break;

    case LRO_CHECKSUM:
        if (va_arg(arg, int))
            handle->checks |= LR_CHECK_CHECKSUM;
        else
            handle->checks &= ~LR_CHECK_CHECKSUM;
        break;

    case LRO_YUMREPOFLAGS:
        handle->yumflags = va_arg(arg, lr_YumRepoFlags);
        break;

    default:
        ret = LRE_UNKNOWNOPT;
        break;
    };

    /* Handle CURL error return code */
    if (c_rc != CURLE_OK) {
        handle->last_curl_error = c_rc;
        switch (c_rc) {
        case CURLE_FAILED_INIT:
            ret = LRE_CURLSETOPT;
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
lr_handle_last_curl_error(lr_Handle handle)
{
    assert(handle);
    return handle->last_curl_error;
}

int
lr_handle_last_curlm_error(lr_Handle handle)
{
    assert(handle);
    return handle->last_curlm_error;
}

int
lr_handle_perform(lr_Handle handle, lr_Result result)
{
    int rc;
    assert(handle);

    if (!result)
        return LRE_BADFUNCARG;

    if (!handle->baseurl && !handle->mirrorlist)
        return LRE_NOURL;

    if (handle->repotype != LR_YUMREPO)
        return LRE_BADFUNCARG;

    /* Setup destination directory */
    if (handle->update) {
        if (!result->destdir)
            return LRE_INCOMPLETERESULT;
        lr_free(handle->destdir);
        handle->destdir = lr_strdup(result->destdir);
    } else if (!handle->destdir && !handle->local) {
        handle->destdir = lr_strdup(TMP_DIR_TEMPLATE);
        if (!mkdtemp(handle->destdir))
            return LRE_CANNOTCREATETMP;
    }

    DEBUGF(fprintf(stderr, "Using dir: %s\n", handle->destdir));

    switch (handle->repotype) {
    case LR_YUMREPO:
        DEBUGF(fprintf(stderr, "Downloading/Locating yum repo\n"));
        rc = lr_yum_perform(handle, result);
        break;
    default:
        DEBUGF(fprintf(stderr, "Bad repo type\n"));
        assert(0);
        break;
    };

    return rc;
}
