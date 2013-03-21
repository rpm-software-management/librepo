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
#define _BSD_SOURCE

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <unistd.h>

#include "handle_internal.h"
#include "handle.h"
#include "result_internal.h"
#include "repomd.h"
#include "setup.h"
#include "rcodes.h"
#include "util.h"
#include "yum.h"
#include "version.h"
#include "curl.h"
#include "yum_internal.h"

void
lr_handle_free_list(char ***list)
{
    int x;
    if (!list || *list == NULL)
        return;

    x = 0;
    while ((*list)[x]) {
        lr_free((*list)[x]);
        x++;
    }

    lr_free(*list);
    *list = NULL;
}

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
    handle->last_curlm_error = CURLM_OK;
    handle->checks |= LR_CHECK_CHECKSUM;

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
    lr_metalink_free(handle->metalink);
    lr_handle_free_list(&handle->yumdlist);
    lr_handle_free_list(&handle->yumblist);
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
        if (handle->internal_mirrorlist) {
            lr_internalmirrorlist_free(handle->internal_mirrorlist);
            handle->internal_mirrorlist = NULL;
        }
        break;

    case LRO_MIRRORLIST:
        handle->mirrorlist = lr_strdup(va_arg(arg, char *));
        if (handle->internal_mirrorlist) {
            lr_internalmirrorlist_free(handle->internal_mirrorlist);
            handle->internal_mirrorlist = NULL;
        }
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

    case LRO_PROXYPORT: {
        c_rc = curl_easy_setopt(c_h, CURLOPT_PROXYPORT,va_arg(arg, long));
        break;
    }

    case LRO_PROXYTYPE: {
        long curl_proxy = -1;
        switch (va_arg(arg, long)) {
            case LR_PROXY_HTTP:     curl_proxy = CURLPROXY_HTTP;    break;
            case LR_PROXY_HTTP_1_0: curl_proxy = CURLPROXY_HTTP_1_0;break;
            case LR_PROXY_SOCKS4:   curl_proxy = CURLPROXY_SOCKS4;  break;
            case LR_PROXY_SOCKS5:   curl_proxy = CURLPROXY_SOCKS5;  break;
            case LR_PROXY_SOCKS4A:  curl_proxy = CURLPROXY_SOCKS4A; break;
            case LR_PROXY_SOCKS5_HOSTNAME: curl_proxy = CURLPROXY_SOCKS5_HOSTNAME; break;
            default: break;
        }
        if (curl_proxy == -1)
            c_rc = LRE_BADOPTARG;
        else
            c_rc = curl_easy_setopt(c_h, CURLOPT_PROXYTYPE, curl_proxy);
        break;
    }

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
        c_rc = curl_easy_setopt(c_h, CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t) va_arg(arg, unsigned long long));
        break;

    case LRO_DESTDIR:
        handle->destdir = lr_strdup(va_arg(arg, char *));
        break;

    case LRO_REPOTYPE:
        handle->repotype = va_arg(arg, lr_Repotype);
        if (handle->repotype != LR_YUMREPO)
            ret = LRE_BADOPTARG;
        break;

    case LRO_CONNECTTIMEOUT:
        c_rc = curl_easy_setopt(c_h, CURLOPT_CONNECTTIMEOUT, va_arg(arg, long));
        break;

    case LRO_IGNOREMISSING:
        handle->ignoremissing = va_arg(arg, long) ? 1 : 0;
        break;

    case LRO_INTERRUPTIBLE:
        handle->interruptible = va_arg(arg, long) ? 1 : 0;
        break;

    case LRO_USERAGENT:
        c_rc = curl_easy_setopt(c_h, CURLOPT_USERAGENT, va_arg(arg, char *));
        break;

    case LRO_GPGCHECK:
        if (va_arg(arg, long))
            handle->checks |= LR_CHECK_GPG;
        else
            handle->checks &= ~LR_CHECK_GPG;
        break;

    case LRO_CHECKSUM:
        if (va_arg(arg, long))
            handle->checks |= LR_CHECK_CHECKSUM;
        else
            handle->checks &= ~LR_CHECK_CHECKSUM;
        break;

    case LRO_YUMDLIST:
    case LRO_YUMBLIST: {
        int size = 0;
        char **list = va_arg(arg, char **);
        char ***handle_list = NULL;

        if (option == LRO_YUMDLIST)
            handle_list = &handle->yumdlist;
        else
            handle_list = &handle->yumblist;

        lr_handle_free_list(handle_list);
        if (!list)
            break;

        /* Copy list */
        while (list[size])
            size++;
        size++;
        *handle_list = lr_malloc0(size * sizeof(char *));
        for (int x = 0; x < size; x++)
            (*handle_list)[x] = lr_strdup(list[x]);
        break;
    }

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

long
lr_handle_last_bad_status_code(lr_Handle handle)
{
    return handle->status_code;
}

const char *
lr_handle_last_curl_strerror(lr_Handle handle)
{
    assert(handle);
    return curl_easy_strerror(handle->last_curl_error);
}

const char *
lr_handle_last_curlm_strerror(lr_Handle handle)
{
    assert(handle);
    return curl_multi_strerror(handle->last_curlm_error);
}

int
lr_handle_prepare_internal_mirrorlist(lr_Handle handle,
                                      const char *metalink_suffix)
{
    int rc = LRE_OK;
    lr_Metalink metalink = NULL;

    if (handle->internal_mirrorlist)
        return LRE_OK;  /* Internal mirrorlist already exists */

    if (!handle->baseurl && !handle->mirrorlist)
        return LRE_NOURL;

    /* Create internal mirrorlist */
    handle->internal_mirrorlist = lr_internalmirrorlist_new();

    if (handle->baseurl) {
        /* Repository URL specified by user insert it as the first element */

        if (strstr(handle->baseurl, "://")) {
            /* Base URL has specified protocol */
            lr_internalmirrorlist_append_url(handle->internal_mirrorlist,
                                             handle->baseurl);
        } else {
            /* No protocol specified - if local path => prepend file:// */
            if (handle->baseurl[0] == '/') {
                /* Base URL is absolute path */
                char *path_with_protocol;
                path_with_protocol = lr_strconcat("file://",
                                                  handle->baseurl,
                                                  NULL);
                lr_internalmirrorlist_append_url(handle->internal_mirrorlist,
                                                 path_with_protocol);
                lr_free(path_with_protocol);
            } else {
                /* Base URL is relative path */
                char *path_with_protocol;
                char *resolved_path = NULL;
                resolved_path = realpath(handle->baseurl, NULL);
                if (!resolved_path) {
                    DPRINTF("%s: realpath: %s\n", __func__, strerror(errno));
                    return LRE_BADURL;
                }
                path_with_protocol = lr_strconcat("file://",
                                                  resolved_path,
                                                  NULL);
                lr_internalmirrorlist_append_url(handle->internal_mirrorlist,
                                                 path_with_protocol);
                free(resolved_path);
                lr_free(path_with_protocol);
            }
        }
    }

    if (handle->mirrorlist) {
        /* Download and parse metalink or mirrorlist to internal mirrorlist */
        lr_Mirrorlist mirrorlist = NULL;
        int mirrors_fd = lr_gettmpfile();

        rc = lr_curl_single_download(handle, handle->mirrorlist, mirrors_fd);
        if (rc != LRE_OK)
            goto mirrorlist_error;

        if (lseek(mirrors_fd, 0, SEEK_SET) != 0) {
            rc = LRE_IO;
            goto mirrorlist_error;
        }

        if (strstr(handle->mirrorlist, "metalink")) {
            /* Metalink */
            DPRINTF("%s: Got metalink\n", __func__);

            /* Parse metalink */
            metalink = lr_metalink_init();
            rc = lr_metalink_parse_file(metalink, mirrors_fd, "repomd.xml");
            if (rc != LRE_OK) {
                DPRINTF("%s: Cannot parse metalink (%d)\n", __func__, rc);
                goto mirrorlist_error;
            }

            if (strcmp("repomd.xml", metalink->filename)) {
                DPRINTF("%s: No repomd.xml file in metalink\n", __func__);
                rc = LRE_MLBAD;
                goto mirrorlist_error;
            }

            if (metalink->nou <= 0) {
                DPRINTF("%s: No URLs in metalink\n", __func__);
                rc = LRE_MLBAD;
                goto mirrorlist_error;
            }
        } else {
            /* Mirrorlist */
            DPRINTF("%s: Got mirrorlist\n", __func__);

            mirrorlist = lr_mirrorlist_init();
            rc = lr_mirrorlist_parse_file(mirrorlist, mirrors_fd);
            if (rc != LRE_OK) {
                DPRINTF("%s: Cannot parse mirrorlist (%d)\n", __func__, rc);
                goto mirrorlist_error;
            }

            if (mirrorlist->nou <= 0) {
                DPRINTF("%s: No URLs in mirrorlist (%d)\n", __func__, rc);
                rc = LRE_MLBAD;
                goto mirrorlist_error;
            }
        }

        /* Set internal_mirrorlist into the handle  */
        if (rc == LRE_OK && metalink) {
            lr_internalmirrorlist_append_metalink(handle->internal_mirrorlist,
                                                  metalink,
                                                  metalink_suffix);
            handle->metalink = metalink;
        }

        if (rc == LRE_OK && mirrorlist)
            lr_internalmirrorlist_append_mirrorlist(handle->internal_mirrorlist,
                                                    mirrorlist);

mirrorlist_error:
        lr_mirrorlist_free(mirrorlist);
        close(mirrors_fd);
        if (rc != LRE_OK) {
            lr_metalink_free(metalink);
            return rc;
        }
    }

    return rc;
}

int
lr_handle_perform(lr_Handle handle, lr_Result result)
{
    int rc = LRE_OK;
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

    DPRINTF("%s: Using dir: %s\n", __func__, handle->destdir);

    struct sigaction old_sigact;
    if (handle->interruptible) {
        /* Setup sighandler */
        struct sigaction sigact;
        sigact.sa_handler = lr_sigint_handler;
        sigaddset(&sigact.sa_mask, SIGINT);
        sigact.sa_flags = 0;
        if (sigaction(SIGINT, &sigact, &old_sigact) == -1)
            return LRE_SIGACTION;
    }

    switch (handle->repotype) {
    case LR_YUMREPO:
        DPRINTF("%s: Downloading/Locating yum repo\n", __func__);
        rc = lr_yum_perform(handle, result);
        break;
    default:
        DPRINTF("%s: Bad repo type\n", __func__);
        assert(0);
        break;
    };

    if (handle->interruptible) {
        /* Restore signal handler */
        if (sigaction(SIGINT, &old_sigact, NULL) == -1)
            return LRE_SIGACTION;
    }

    return rc;
}

int
lr_handle_getinfo(lr_Handle handle, lr_HandleOption option, ...)
{
    int rc = LRE_OK;
    va_list arg;
    char **str;
    long *lnum;

    if (!handle)
        return LRE_BADFUNCARG;

    va_start(arg, option);

    switch (option) {

    case LRI_UPDATE:
        lnum = va_arg(arg, long *);
        *lnum = (long) handle->update;
        break;

    case LRI_URL:
        str = va_arg(arg, char **);
        *str = handle->baseurl;
        break;

    case LRI_MIRRORLIST:
        str = va_arg(arg, char **);
        *str = handle->mirrorlist;
        break;

    case LRI_LOCAL:
        lnum = va_arg(arg, long *);
        *lnum = handle->local;
        break;

    case LRI_PROGRESSCB: {
        lr_ProgressCb *cb= va_arg(arg, lr_ProgressCb *);
        *cb = handle->user_cb;
        break;
    }

    case LRI_PROGRESSDATA: {
        void **data = va_arg(arg, void **);
        *data = handle->user_data;
        break;
    }

    case LRI_DESTDIR:
        str = va_arg(arg, char **);
        *str = handle->destdir;
        break;

    case LRI_REPOTYPE:
        lnum = va_arg(arg, long *);
        *lnum = (long) handle->repotype;
        break;

    case LRI_YUMDLIST: {
        char ***strlist = va_arg(arg, char ***);
        *strlist = handle->yumdlist;
        break;
    }

    case LRI_YUMBLIST: {
        char ***strlist = va_arg(arg, char ***);
        *strlist = handle->yumblist;
        break;
    }

    case LRI_LASTCURLERR:
        lnum = va_arg(arg, long *);
        *lnum = handle->last_curl_error;
        break;

    case LRI_LASTCURLMERR:
        lnum = va_arg(arg, long *);
        *lnum = handle->last_curlm_error;
        break;

    case LRI_LASTCURLSTRERR:
        str = va_arg(arg, char **);
        *str = (char *) curl_easy_strerror(handle->last_curl_error);
        break;

    case LRI_LASTCURLMSTRERR:
        str = va_arg(arg, char **);
        *str = (char *) curl_multi_strerror(handle->last_curlm_error);
        break;

    case LRI_LASTBADSTATUSCODE:
        lnum = va_arg(arg, long *);
        *lnum = handle->status_code;
        break;

    default:
        rc = LRE_UNKNOWNOPT;
        break;
    }

    va_end(arg);
    return rc;
}
