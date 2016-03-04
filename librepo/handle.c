/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2012  Tomas Mlcoch
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

#define _POSIX_C_SOURCE 200809L
#define _BSD_SOURCE

#include <glib.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "handle_internal.h"
#include "handle.h"
#include "result_internal.h"
#include "repomd.h"
#include "rcodes.h"
#include "util.h"
#include "yum.h"
#include "version.h"
#include "yum_internal.h"
#include "url_substitution.h"
#include "downloader.h"
#include "fastestmirror_internal.h"
#include "cleanup.h"

CURL *
lr_get_curl_handle()
{
    CURL *h;

    lr_global_init();

    h = curl_easy_init();
    curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(h, CURLOPT_MAXREDIRS, 6);
    curl_easy_setopt(h, CURLOPT_CONNECTTIMEOUT, LRO_CONNECTTIMEOUT_DEFAULT);
    curl_easy_setopt(h, CURLOPT_LOW_SPEED_TIME, LRO_LOWSPEEDTIME_DEFAULT);
    curl_easy_setopt(h, CURLOPT_LOW_SPEED_LIMIT, LRO_LOWSPEEDLIMIT_DEFAULT);
    curl_easy_setopt(h, CURLOPT_SSL_VERIFYHOST, 2);
    curl_easy_setopt(h, CURLOPT_SSL_VERIFYPEER, 1);
    curl_easy_setopt(h, CURLOPT_FTP_USE_EPSV, LRO_FTPUSEEPSV_DEFAULT);

    return h;
}

void
lr_handle_free_list(char ***list)
{
    if (!list || *list == NULL)
        return;
    for (int x=0; (*list)[x]; x++)
        lr_free((*list)[x]);
    lr_free(*list);
    *list = NULL;
}

LrHandle *
lr_handle_init()
{
    LrHandle *handle;
    CURL *curl = lr_get_curl_handle();

    if (!curl)
        return NULL;

    handle = lr_malloc0(sizeof(LrHandle));
    handle->curl_handle = curl;
    handle->fastestmirrormaxage = LRO_FASTESTMIRRORMAXAGE_DEFAULT;
    handle->mirrorlist_fd = -1;
    handle->metalink_fd = -1;
    handle->checks |= LR_CHECK_CHECKSUM;
    handle->maxparalleldownloads = LRO_MAXPARALLELDOWNLOADS_DEFAULT;
    handle->maxdownloadspermirror = LRO_MAXDOWNLOADSPERMIRROR_DEFAULT;
    handle->lowspeedtime = LRO_LOWSPEEDTIME_DEFAULT;
    handle->lowspeedlimit = LRO_LOWSPEEDLIMIT_DEFAULT;
    handle->sslverifypeer = 1;
    handle->sslverifyhost = 2;
    handle->ipresolve = LRO_IPRESOLVE_DEFAULT;
    handle->allowed_mirror_failures = LRO_ALLOWEDMIRRORFAILURES_DEFAULT;
    handle->adaptivemirrorsorting = LRO_ADAPTIVEMIRRORSORTING_DEFAULT;
    handle->gnupghomedir = g_strdup(LRO_GNUPGHOMEDIR_DEFAULT);
    handle->fastestmirrortimeout = LRO_FASTESTMIRRORTIMEOUT_DEFAULT;
    handle->offline = LRO_OFFLINE_DEFAULT;
    handle->httpauthmethods = LRO_HTTPAUTHMETHODS_DEFAULT;
    handle->proxyauthmethods = LRO_PROXYAUTHMETHODS_DEFAULT;
    handle->ftpuseepsv = LRO_FTPUSEEPSV_DEFAULT;

    return handle;
}

void
lr_handle_free(LrHandle *handle)
{
    if (!handle)
        return;
    if (handle->curl_handle)
        curl_easy_cleanup(handle->curl_handle);
    if (handle->mirrorlist_fd != -1)
        close(handle->mirrorlist_fd);
    if (handle->metalink_fd != -1)
        close(handle->metalink_fd);
    lr_handle_free_list(&handle->urls);
    lr_free(handle->fastestmirrorcache);
    lr_free(handle->mirrorlist);
    lr_free(handle->mirrorlisturl);
    lr_free(handle->metalinkurl);
    lr_free(handle->used_mirror);
    lr_free(handle->destdir);
    lr_free(handle->useragent);
    lr_free(handle->sslclientcert);
    lr_free(handle->sslclientkey);
    lr_free(handle->sslcacert);
    lr_lrmirrorlist_free(handle->internal_mirrorlist);
    lr_lrmirrorlist_free(handle->urls_mirrors);
    lr_lrmirrorlist_free(handle->mirrorlist_mirrors);
    lr_lrmirrorlist_free(handle->metalink_mirrors);
    lr_lrmirrorlist_free(handle->mirrors);
    lr_metalink_free(handle->metalink);
    lr_handle_free_list(&handle->yumdlist);
    lr_handle_free_list(&handle->yumblist);
    lr_urlvars_free(handle->urlvars);
    lr_free(handle->gnupghomedir);
    lr_handle_free_list(&handle->httpheader);
    lr_free(handle);
}

typedef enum {
    LR_REMOTESOURCE_URLS,
    LR_REMOTESOURCE_MIRRORLIST,
    LR_REMOTESOURCE_METALINK,
    LR_REMOTESOURCE_LOCAL_OPT,
    LR_REMOTESOURCE_OFFLINE_OPT,
    LR_REMOTESOURCE_OTHER,
} LrChangedRemoteSource;

static unsigned long curlauth_bitmask(LrAuth mask)
{
    unsigned long out_mask = 0UL;

    if (mask == LR_AUTH_NONE)
        return (unsigned long) CURLAUTH_NONE;

    if (mask & LR_AUTH_BASIC)
        out_mask |= CURLAUTH_BASIC;
    if (mask & LR_AUTH_DIGEST)
        out_mask |= CURLAUTH_DIGEST;
    if (mask & LR_AUTH_NEGOTIATE)
#ifdef CURLAUTH_NEGOTIATE
        out_mask |= CURLAUTH_NEGOTIATE;
#else
        out_mask |= CURLAUTH_GSSNEGOTIATE;
#endif
    if (mask & LR_AUTH_NTLM)
        out_mask |= CURLAUTH_NTLM;
    if (mask & LR_AUTH_DIGEST_IE)
        out_mask |= CURLAUTH_DIGEST_IE;
    if (mask & LR_AUTH_NTLM_WB)
        out_mask |= CURLAUTH_NTLM_WB;
    if (mask & LR_AUTH_ONLY)
        out_mask |= CURLAUTH_ONLY;
    if (mask == LR_AUTH_ANY)
        out_mask |= CURLAUTH_ANY;

    return out_mask;
}

static void
lr_handle_remote_sources_changed(LrHandle *handle, LrChangedRemoteSource type)
{
    // Called when options like:
    // LRO_URLS
    // LRO_MIRRORLIST
    // LRO_MIRRORLISTURL
    // LRO_METALINKURL
    // LRO_LOCAL
    // LRO_OFFLINE
    // are changed

    // Internal mirrorlist is no more valid
    lr_lrmirrorlist_free(handle->internal_mirrorlist);
    handle->internal_mirrorlist = NULL;

    // Mirrors reported via mirrors are no more valid too
    lr_lrmirrorlist_free(handle->mirrors);
    handle->mirrors = NULL;

    if (type == LR_REMOTESOURCE_URLS) {
        lr_lrmirrorlist_free(handle->urls_mirrors);
        handle->urls_mirrors = NULL;
    }

    if (type == LR_REMOTESOURCE_MIRRORLIST) {
        lr_lrmirrorlist_free(handle->mirrorlist_mirrors);
        handle->mirrorlist_mirrors = NULL;
        if (handle->mirrorlist_fd != -1)
            close(handle->mirrorlist_fd);
        handle->mirrorlist_fd = -1;
    }

    if (type == LR_REMOTESOURCE_METALINK) {
        lr_lrmirrorlist_free(handle->metalink_mirrors);
        handle->metalink_mirrors = NULL;
        if (handle->metalink_fd != -1)
            close(handle->metalink_fd);
        handle->metalink_fd = -1;
        lr_metalink_free(handle->metalink);
        handle->metalink = NULL;
    }
}

gboolean
lr_handle_setopt(LrHandle *handle,
                 GError **err,
                 LrHandleOption option,
                 ...)
{
    gboolean ret = TRUE;
    va_list arg;
    CURLcode c_rc = CURLE_OK;
    CURL *c_h;

    assert(!err || *err == NULL);

    // Variables for values from va_arg
    long val_long;
    gint64 val_gint64;

    if (!handle) {
        g_set_error(err, LR_HANDLE_ERROR, LRE_BADFUNCARG,
                    "No handle specified");
        return FALSE;
    }

    c_h = handle->curl_handle;

    va_start(arg, option);

    switch (option) {
    case LRO_UPDATE:
        handle->update = va_arg(arg, long) ? 1 : 0;
        break;

    case LRO_MIRRORLIST:
        // DEPRECATED!
        g_debug("%s: WARNING! Deprecated LRO_MIRRORLIST used", __func__);
        if (handle->mirrorlist) lr_free(handle->mirrorlist);
        handle->mirrorlist = g_strdup(va_arg(arg, char *));

        if (handle->mirrorlisturl)
            lr_free(handle->mirrorlisturl);
        handle->mirrorlisturl = NULL;
        if (handle->metalinkurl)
            lr_free(handle->metalinkurl);
        handle->metalinkurl = NULL;
        lr_handle_remote_sources_changed(handle, LR_REMOTESOURCE_MIRRORLIST);
        lr_handle_remote_sources_changed(handle, LR_REMOTESOURCE_METALINK);

        if (!handle->mirrorlist)
            break;

        if (strstr(handle->mirrorlist, "metalink"))
            handle->metalinkurl = g_strdup(handle->mirrorlist);
        else
            handle->mirrorlisturl = g_strdup(handle->mirrorlist);
        break;

    case LRO_MIRRORLISTURL:
        if (handle->mirrorlisturl)
            lr_free(handle->mirrorlisturl);
        handle->mirrorlisturl = g_strdup(va_arg(arg, char *));
        lr_handle_remote_sources_changed(handle, LR_REMOTESOURCE_MIRRORLIST);
        break;

    case LRO_METALINKURL:
        if (handle->metalinkurl)
            lr_free(handle->metalinkurl);
        handle->metalinkurl = g_strdup(va_arg(arg, char *));
        lr_handle_remote_sources_changed(handle, LR_REMOTESOURCE_METALINK);
        break;

    case LRO_LOCAL:
        lr_handle_remote_sources_changed(handle, LR_REMOTESOURCE_LOCAL_OPT);
        handle->local = va_arg(arg, long) ? 1 : 0;
        break;

    case LRO_HTTPAUTH:
        if (va_arg(arg, long) ==  0) {
            c_rc = curl_easy_setopt(c_h, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
            handle->httpauthmethods = LR_AUTH_BASIC;
        } else {
            c_rc = curl_easy_setopt(c_h, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
            handle->httpauthmethods = LR_AUTH_ANY;
        }
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
        if (curl_proxy == -1) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_BADOPTARG,
                    "Bad LRO_PROXYTYPE value");
            ret = FALSE;
        } else {
            c_rc = curl_easy_setopt(c_h, CURLOPT_PROXYTYPE, curl_proxy);
        }
        break;
    }

    case LRO_PROXYAUTH:
        if (va_arg(arg, long) == 0) {
            c_rc = curl_easy_setopt(c_h, CURLOPT_PROXYAUTH, CURLAUTH_BASIC);
            handle->proxyauthmethods = LR_AUTH_BASIC;
        } else {
            c_rc = curl_easy_setopt(c_h, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
            handle->proxyauthmethods = LR_AUTH_ANY;
        }
        break;

    case LRO_PROXYUSERPWD:
        c_rc = curl_easy_setopt(c_h, CURLOPT_PROXYUSERPWD, va_arg(arg, char *));
        break;

    case LRO_PROGRESSCB:
        handle->user_cb = va_arg(arg, LrProgressCb);
        break;

    case LRO_PROGRESSDATA:
        handle->user_data = va_arg(arg, void *);
        break;

    case LRO_MAXSPEED:
        val_gint64 = va_arg(arg, gint64);
        if (val_gint64 < 0) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_BADOPTARG,
                        "Bad value of LRO_MAXSPEED");
            ret = FALSE;
            break;
        } else if (val_gint64 != 0 && val_gint64 < handle->lowspeedlimit) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_BADOPTARG,
                        "LRO_MAXSPEED (%"G_GINT64_FORMAT") is lower than "
                        "LRO_LOWSPEEDLIMIT (%ld)", val_gint64,
                        handle->lowspeedlimit);
            ret = FALSE;
            break;
        }
        handle->maxspeed = val_gint64;
        break;

    case LRO_DESTDIR:
        if (handle->destdir) lr_free(handle->destdir);
        handle->destdir = g_strdup(va_arg(arg, char *));
        break;

    case LRO_REPOTYPE:
        handle->repotype = va_arg(arg, LrRepotype);
        if (handle->repotype != LR_YUMREPO) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_BADOPTARG,
                        "Bad value of LRO_REPOTYPE");
            ret = FALSE;
        }
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

    case LRO_USERAGENT: {
        char *useragent = va_arg(arg, char *);
        if (handle->useragent) lr_free(handle->useragent);
        handle->useragent = g_strdup(useragent);
        c_rc = curl_easy_setopt(c_h, CURLOPT_USERAGENT, useragent);
        break;
    }

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

    case LRO_URLS:
    case LRO_YUMDLIST:
    case LRO_YUMBLIST:
    {
        int size = 0;
        char **list = va_arg(arg, char **);
        char ***handle_list = NULL;

        if (option == LRO_URLS) {
            handle_list = &handle->urls;
            lr_handle_remote_sources_changed(handle, LR_REMOTESOURCE_URLS);
        } else if (option == LRO_YUMDLIST) {
            handle_list = &handle->yumdlist;
        } else if (option == LRO_YUMBLIST) {
            handle_list = &handle->yumblist;
        }

        lr_handle_free_list(handle_list);
        if (!list)
            break;

        // Get list length
        while (list[size])
            size++;
        size++;

        if (size == 1 && option == LRO_URLS) {
            // Only NULL present in list of URLs, keep handle->urls = NULL
            break;
        }

        // Copy the list
        *handle_list = lr_strv_dup(list);
        break;
    }

    case LRO_HTTPHEADER:
    {
        char **list = va_arg(arg, char **);
        lr_handle_free_list(&handle->httpheader);
        handle->httpheader = lr_strv_dup(list);
        break;
    }

    case LRO_FETCHMIRRORS:
        handle->fetchmirrors = va_arg(arg, long) ? 1 : 0;
        break;

    case LRO_MAXMIRRORTRIES:
        val_long = va_arg(arg, long);

        if (handle->maxmirrortries < LRO_MAXMIRRORTRIES_MIN) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_BADOPTARG,
                    "Value of LRO_MAXMIRRORTRIES is too low (use value > %ld)",
                    LRO_MAXMIRRORTRIES_MIN);
            ret = FALSE;
        } else {
            handle->maxmirrortries = val_long;
        }

        break;

    case LRO_MAXPARALLELDOWNLOADS:
        val_long = va_arg(arg, long);

        if (val_long < LRO_MAXPARALLELDOWNLOADS_MIN ||
            val_long > LRO_MAXPARALLELDOWNLOADS_MAX) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_BADOPTARG,
                        "Bad value of LRO_MAXPARALLELDOWNLOADS.");
            ret = FALSE;
        } else {
            handle->maxparalleldownloads = val_long;
        }

        break;

    case LRO_MAXDOWNLOADSPERMIRROR:
        val_long = va_arg(arg, long);

        if (val_long < LRO_MAXDOWNLOADSPERMIRROR_MIN) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_BADOPTARG,
                        "Value of LRO_MAXDOWNLOADSPERMIRROR is too low.");
            ret = FALSE;
        } else {
            handle->maxdownloadspermirror = val_long;
        }

        break;

    case LRO_VARSUB: {
        LrUrlVars *vars = va_arg(arg, LrUrlVars *);
        lr_urlvars_free(handle->urlvars);
        handle->urlvars = vars;

        /* Do not do copy
        for (LrUrlVars *elem = vars; elem; elem = lr_list_next(elem)) {
            LrVar *var = elem->data;
            handle->urlvars = lr_urlvars_set(handle->urlvars, var->var, var->val);
        }
        */

        break;
    }

    case LRO_FASTESTMIRROR:
        handle->fastestmirror = va_arg(arg, long) ? 1 : 0;
        break;

    case LRO_FASTESTMIRRORCACHE: {
        char *fastestmirrorcache = va_arg(arg, char *);
        if (handle->fastestmirrorcache) lr_free(handle->fastestmirrorcache);
        handle->fastestmirrorcache = g_strdup(fastestmirrorcache);
        break;
    }

    case LRO_FASTESTMIRRORMAXAGE:
        val_long = va_arg(arg, long);

        if (val_long < LRO_FASTESTMIRRORMAXAGE_MIN) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_BADOPTARG,
                        "Value of LRO_FASTESTMIRRORMAXAGE is too low.");
            ret = FALSE;
        } else {
            handle->fastestmirrormaxage = val_long;
        }

        break;

    case LRO_FASTESTMIRRORCB:
        handle->fastestmirrorcb = va_arg(arg, LrFastestMirrorCb);
        break;

    case LRO_FASTESTMIRRORDATA:
        handle->fastestmirrordata = va_arg(arg, void *);
        break;

    case LRO_LOWSPEEDTIME:
        val_long = va_arg(arg, long);

        if (val_long < LRO_LOWSPEEDTIME_MIN) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_BADOPTARG,
                        "Value of LRO_LOWSPEEDTIME is too low.");
            ret = FALSE;
        } else {
            curl_easy_setopt(c_h, CURLOPT_LOW_SPEED_TIME, val_long);
            handle->lowspeedtime = val_long;
        }

        break;

    case LRO_LOWSPEEDLIMIT:
        val_long = va_arg(arg, long);

        if (val_long < LRO_LOWSPEEDLIMIT_MIN) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_BADOPTARG,
                        "Value of LRO_LOWSPEEDLIMIT is too low.");
            ret = FALSE;
        } else if (handle->maxspeed != 0 && handle->maxspeed < val_long) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_BADOPTARG,
                        "Value of LRO_LOWSPEEDLIMIT (%ld) is higher than "
                        "LRO_MAXSPEED (%"G_GINT64_FORMAT")",
                        val_long, handle->maxspeed);
            ret = FALSE;
        } else {
            curl_easy_setopt(c_h, CURLOPT_LOW_SPEED_LIMIT, val_long);
            handle->lowspeedlimit = val_long;
        }

        break;

    case LRO_HMFCB:
        handle->hmfcb = va_arg(arg, LrHandleMirrorFailureCb);
        break;

    case LRO_SSLVERIFYPEER:
        handle->sslverifypeer = va_arg(arg, long) ? 1 : 0;
        c_rc = curl_easy_setopt(c_h, CURLOPT_SSL_VERIFYPEER, handle->sslverifypeer);
        break;

    case LRO_SSLVERIFYHOST:
        handle->sslverifyhost = va_arg(arg, long) ? 2 : 0;
        c_rc = curl_easy_setopt(c_h, CURLOPT_SSL_VERIFYPEER, handle->sslverifyhost);
        break;

    case LRO_SSLCLIENTCERT:
        if (handle->sslclientcert)
            lr_free(handle->sslclientcert);
        handle->sslclientcert = g_strdup(va_arg(arg, char *));
        c_rc = curl_easy_setopt(c_h, CURLOPT_SSLCERT, handle->sslclientcert);
        break;

    case LRO_SSLCLIENTKEY:
        if (handle->sslclientkey)
            lr_free(handle->sslclientkey);
        handle->sslclientkey = g_strdup(va_arg(arg, char *));
        c_rc = curl_easy_setopt(c_h, CURLOPT_SSLKEY, handle->sslclientkey);
        break;

    case LRO_SSLCACERT:
        if (handle->sslcacert)
            lr_free(handle->sslcacert);
        handle->sslcacert = g_strdup(va_arg(arg, char *));
        c_rc = curl_easy_setopt(c_h, CURLOPT_CAINFO, handle->sslcacert);
        break;

    case LRO_IPRESOLVE: {
        long type = -1;
        long lr_type = va_arg(arg, LrIpResolveType);
        switch (lr_type) {
            case LR_IPRESOLVE_WHATEVER: type = CURL_IPRESOLVE_WHATEVER; break;
            case LR_IPRESOLVE_V4:       type = CURL_IPRESOLVE_V4;       break;
            case LR_IPRESOLVE_V6:       type = CURL_IPRESOLVE_V6;       break;
            default: break;
        }
        if (type == -1) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_BADOPTARG,
                    "Bad LRO_IPRESOLVE value");
            ret = FALSE;
        } else {
            handle->ipresolve = lr_type;
            c_rc = curl_easy_setopt(c_h, CURLOPT_IPRESOLVE, type);
        }
        break;
    }

    case LRO_ALLOWEDMIRRORFAILURES:
        handle->allowed_mirror_failures = va_arg(arg, long);
        break;

    case LRO_ADAPTIVEMIRRORSORTING:
        handle->adaptivemirrorsorting = va_arg(arg, long);
        break;

    case LRO_GNUPGHOMEDIR: {
        char *gnupghomedir = va_arg(arg, char *);
        lr_free(handle->gnupghomedir);
        handle->gnupghomedir = g_strdup(gnupghomedir);
        break;
    }

    case LRO_FASTESTMIRRORTIMEOUT:
        handle->fastestmirrortimeout = va_arg(arg, double);
        break;

    case LRO_OFFLINE:
        lr_handle_remote_sources_changed(handle, LR_REMOTESOURCE_OFFLINE_OPT);
        handle->offline = va_arg(arg, long) ? 1 : 0;
        break;

    case LRO_HTTPAUTHMETHODS: {
        LrAuth in_bitmask = va_arg(arg, LrAuth);
        long bitmask = curlauth_bitmask(in_bitmask);
        handle->httpauthmethods = in_bitmask;
        c_rc = curl_easy_setopt(c_h, CURLOPT_HTTPAUTH, bitmask);
        break;
    }

    case LRO_PROXYAUTHMETHODS: {
        LrAuth in_bitmask = va_arg(arg, LrAuth);
        long bitmask = curlauth_bitmask(in_bitmask);
        handle->proxyauthmethods = in_bitmask;
        c_rc = curl_easy_setopt(c_h, CURLOPT_PROXYAUTH, bitmask);
        break;
    }

    case LRO_FTPUSEEPSV:
        handle->ftpuseepsv = va_arg(arg, long) ? 1 : 0;
        c_rc = curl_easy_setopt(c_h, CURLOPT_FTP_USE_EPSV, handle->ftpuseepsv);
        break;

    default:
        g_set_error(err, LR_HANDLE_ERROR, LRE_BADOPTARG,
                    "Unknown option");
        ret = FALSE;
        break;

    };

    /* Handle CURL error return code */
    if (ret == TRUE && c_rc != CURLE_OK) {
        ret = FALSE;
        switch (c_rc) {
        case CURLE_FAILED_INIT:
            g_set_error(err, LR_HANDLE_ERROR, LRE_CURLSETOPT,
                        "curl_easy_setopt error: %s",
                        curl_easy_strerror(c_rc));
            break;
        default:
            g_set_error(err, LR_HANDLE_ERROR, LRE_CURL,
                        "curl error: %s",
                        curl_easy_strerror(c_rc));
            break;
        };
    }

    va_end(arg);
    return ret;
}

/*
 * Internal mirrorlist stuff
 */

static gboolean
download_non_cached_url(LrHandle *lr_handle, const char *url, int fd, GError **err)
{
    // This function is almost 1:1 copy of lr_download_url

    gboolean ret;
    LrDownloadTarget *target;
    GError *tmp_err = NULL;

    assert(url);
    assert(!err || *err == NULL);

    // Prepare target
    target = lr_downloadtarget_new(lr_handle,
                                   url, NULL, fd, NULL,
                                   NULL, 0, 0, NULL, NULL,
                                   NULL, NULL, NULL, 0, 0, TRUE);

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


static gboolean
lr_handle_prepare_urls(LrHandle *handle, GError **err)
{
    assert(!handle->urls_mirrors);

    for (int x=0; handle->urls[x]; x++) {
        gchar *url = handle->urls[x];
        _cleanup_free_ gchar *final_url = NULL;

        // Make sure that url has protocol specified
        final_url = lr_prepend_url_protocol(url);
        if (!final_url) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_BADURL,
                        "Cannot resolve path for: \"%s\"", url);
            return FALSE;
        }

        // Append the url into internal list of urls specified by LRO_URLS
        handle->urls_mirrors = lr_lrmirrorlist_append_url(
                                            handle->urls_mirrors,
                                            final_url,
                                            handle->urlvars);
    }

    return TRUE;
}

static gboolean
lr_handle_prepare_mirrorlist(LrHandle *handle, gchar *localpath, GError **err)
{
    assert(handle->mirrorlist_fd == -1);
    assert(!handle->mirrorlist_mirrors);

    int fd = -1;

    // Get file descriptor with content

    if (!localpath && !handle->mirrorlisturl) {
        // Nothing to do
        return TRUE;
    } else if (localpath && !handle->mirrorlisturl) {
        // Just try to use mirrorlist of the local repository
        gchar *path = lr_pathconcat(localpath, "mirrorlist", NULL);
        if (g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
            g_debug("%s: Local mirrorlist found at %s", __func__, path);
            fd = open(path, O_RDONLY);
            if (fd < 0) {
                g_set_error(err, LR_HANDLE_ERROR, LRE_IO,
                            "Cannot open %s: %s",
                            path, g_strerror(errno));
                g_free(path);
                return FALSE;
            }
            g_free(path);
        } else {
            // No local mirrorlist
            g_free(path);
            return TRUE;
        }
    } else if (!handle->mirrorlisturl) {
        // Nothing to do
        return TRUE;
    } else if (handle->offline && !lr_is_local_path(handle->mirrorlisturl)) {
        // We should work offline, ignore remote mirrorlist
        g_debug("%s: LRO_OFFLINE used, remote mirrorlist ignored: %s",
                __func__, handle->mirrorlisturl);
        return TRUE;
    } else if (handle->local && !lr_is_local_path(handle->mirrorlisturl)) {
        // We should work only locally, ignore remote mirrorlist
        g_debug("%s: LRO_LOCAL used, remote mirrorlist ignored: %s",
                __func__, handle->mirrorlisturl);
        return TRUE;
    } else if (handle->mirrorlisturl) {
        // Download remote mirrorlist
        _cleanup_free_ gchar *url = NULL;

        fd = lr_gettmpfile();
        if (fd < 0) {
            g_debug("%s: Cannot create a temporary file", __func__);
            g_set_error(err, LR_HANDLE_ERROR, LRE_IO,
                        "Cannot create a temporary file");
            return FALSE;
        }

        url = lr_prepend_url_protocol(handle->mirrorlisturl);
        if (!download_non_cached_url(handle, url, fd, err)) {
            close(fd);
            return FALSE;
        }

        if (lseek(fd, 0, SEEK_SET) != 0) {
            g_debug("%s: Seek error: %s", __func__, g_strerror(errno));
            g_set_error(err, LR_HANDLE_ERROR, LRE_IO,
                        "lseek(%d, 0, SEEK_SET) error: %s",
                        fd, g_strerror(errno));
            close(fd);
            return FALSE;
        }
    }

    assert(fd >= 0);

    // Parse the file descriptor content

    g_debug("%s: Parsing mirrorlist", __func__);

    LrMirrorlist *ml = lr_mirrorlist_init();
    gboolean ret = lr_mirrorlist_parse_file(ml, fd, err);
    if (!ret) {
        g_debug("%s: Error while parsing mirrorlist", __func__);
        close(fd);
        lr_mirrorlist_free(ml);
        return FALSE;
    }

    if (!ml->urls) {
        g_debug("%s: No URLs in mirrorlist", __func__);
        g_set_error(err, LR_HANDLE_ERROR, LRE_MLBAD, "No URLs in mirrorlist");
        close(fd);
        lr_mirrorlist_free(ml);
        return FALSE;
    }

    // List parsed mirrors
    g_debug("%s: Mirrors from mirrorlist:", __func__);
    for (GSList *elem = ml->urls; elem; elem = g_slist_next(elem))
        g_debug("  %s", (gchar *) elem->data);

    // Convert mirrorlist to internal mirrorlist format

    handle->mirrorlist_mirrors = lr_lrmirrorlist_append_mirrorlist(
                                            NULL,
                                            ml,
                                            handle->urlvars);
    handle->mirrorlist_fd = fd;

    lr_mirrorlist_free(ml);

    g_debug("%s: Mirrorlist parsed", __func__);
    return TRUE;
}

static gboolean
lr_handle_prepare_metalink(LrHandle *handle, gchar *localpath, GError **err)
{
    assert(handle->metalink_fd == -1);
    assert(!handle->metalink_mirrors);
    assert(!handle->metalink);

    int fd = -1;

    // Get file descriptor with content

    if (!localpath && !handle->metalinkurl) {
        // Nothing to do
        return TRUE;
    } else if (localpath && !handle->metalinkurl) {
        // Just try to use metalink of the local repository
        gchar *path = lr_pathconcat(localpath, "metalink.xml", NULL);
        if (g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
            g_debug("%s: Local metalink.xml found at %s", __func__, path);
            fd = open(path, O_RDONLY);
            if (fd < 0) {
                g_set_error(err, LR_HANDLE_ERROR, LRE_IO,
                            "Cannot open %s: %s",
                            path, g_strerror(errno));
                g_free(path);
                return FALSE;
            }
            g_free(path);
        } else {
            // No local metalink
            g_free(path);
            return TRUE;
        }
    } else if (!handle->metalinkurl) {
        // Nothing to do
        return TRUE;
    } else if (handle->offline && !lr_is_local_path(handle->metalinkurl)) {
        // We should work offline, ignore remote mirrorlist
        g_debug("%s: LRO_OFFLINE used, remote metalink ignored: %s",
                __func__, handle->metalinkurl);
        return TRUE;
    } else if (handle->local && !lr_is_local_path(handle->metalinkurl)) {
        // We should work only locally, ignore remote mirrorlist
        g_debug("%s: LRO_LOCAL used, remote metalink ignored: %s",
                __func__, handle->metalinkurl);
        return TRUE;
    } else if (handle->metalinkurl) {
        // Download remote metalink
        _cleanup_free_ gchar *url = NULL;

        fd = lr_gettmpfile();
        if (fd < 0) {
            g_debug("%s: Cannot create a temporary file", __func__);
            g_set_error(err, LR_HANDLE_ERROR, LRE_IO,
                        "Cannot create a temporary file");
            return FALSE;
        }

        url = lr_prepend_url_protocol(handle->metalinkurl);
        if (!download_non_cached_url(handle, url, fd, err)) {
            close(fd);
            return FALSE;
        }

        if (lseek(fd, 0, SEEK_SET) != 0) {
            g_debug("%s: Seek error: %s", __func__, g_strerror(errno));
            g_set_error(err, LR_HANDLE_ERROR, LRE_IO,
                        "lseek(%d, 0, SEEK_SET) error: %s",
                        fd, g_strerror(errno));
            close(fd);
            return FALSE;
        }
    }

    assert(fd >= 0);

    // Parse the file descriptor content

    g_debug("%s: Parsing metalink.xml", __func__);

    gchar *metalink_file = "";
    gchar *metalink_suffix = NULL;
    if (handle->repotype == LR_YUMREPO) {
        metalink_file = "repomd.xml";
        metalink_suffix = "repodata/repomd.xml";
    }

    LrMetalink *ml = lr_metalink_init();
    gboolean ret = lr_metalink_parse_file(ml,
                                          fd,
                                          metalink_file,
                                          lr_xml_parser_warning_logger,
                                          "Metalink xml parser",
                                          err);
    if (!ret) {
        g_debug("%s: Error while parsing metalink", __func__);
        close(fd);
        lr_metalink_free(ml);
        return FALSE;
    }

    if (!ml->urls) {
        g_debug("%s: No URLs in metalink", __func__);
        g_set_error(err, LR_HANDLE_ERROR, LRE_MLBAD, "No URLs in metalink");
        close(fd);
        lr_metalink_free(ml);
        return FALSE;
    }

    // List parsed mirrors
    g_debug("%s: Mirrors from metalink:", __func__);
    for (GSList *elem = ml->urls; elem; elem = g_slist_next(elem))
        g_debug("  %s", ((LrMetalinkUrl *) elem->data)->url);

    // Convert metalink to internal mirrorlist format

    handle->metalink_mirrors = lr_lrmirrorlist_append_metalink(
                                            NULL,
                                            ml,
                                            metalink_suffix,
                                            handle->urlvars);
    handle->metalink_fd = fd;
    handle->metalink = ml;

    g_debug("%s: Metalink parsed", __func__);
    return TRUE;
}

gboolean
lr_handle_prepare_internal_mirrorlist(LrHandle *handle,
                                      gboolean usefastestmirror,
                                      GError **err)
{
    assert(!err || *err == NULL);

    if (handle->internal_mirrorlist)
        return TRUE;  // Internal mirrorlist already exists

    // Create internal mirrorlist

    g_debug("%s: Preparing internal mirrorlist", __func__);

    // Get local path in case of local repository
    gchar *local_path = NULL;
    if (handle->urls && handle->urls[0]) {
        // If first base URL is local path, get that path
        // (without file: or file:// prefix if specified)
        gchar *url = handle->urls[0];
        if (g_str_has_prefix(url, "file://"))
            local_path = url + 7;
        else if (g_str_has_prefix(url, "file:"))  // RFC 3986
            local_path = url + 5;
        else if (!strstr(url, "://"))
            local_path = url;
    }

    gboolean ret;

    // LRO_URLS
    if (!handle->urls_mirrors && handle->urls) {
        ret = lr_handle_prepare_urls(handle, err);
        if (!ret) {
            assert(!err || *err);
            g_debug("%s: LRO_URLS processing failed", __func__);
            return FALSE;
        }
    }

    // LRO_MIRRORLISTURL
    if (!handle->mirrorlist_mirrors && (handle->mirrorlisturl || local_path)) {
        ret = lr_handle_prepare_mirrorlist(handle, local_path, err);
        if (!ret) {
            assert(!err || *err);
            g_debug("%s: LRO_MIRRORLISTURL processing failed", __func__);
            return FALSE;
        }
    }

    // LRO_METALINKURL
    if (!handle->metalink_mirrors && (handle->metalinkurl || local_path)) {
        ret = lr_handle_prepare_metalink(handle, local_path, err);
        if (!ret) {
            assert(!err || *err);
            g_debug("%s: LRO_METALINKURL processing failed", __func__);
            return FALSE;
        }
    }

    // Append all the mirrorlist to the single internal mirrorlist
    // This internal mirrorlist is used for downloading
    // Note: LRO_MIRRORLISTURL and LRO_METALINKURL lists are included
    // to this list only if they are explicitly specified (lists
    // implicitly loaded from a local repository are not included)

    g_debug("%s: Finalizing internal mirrorlist", __func__);

    // Mirrorlist from the LRO_URLS
    handle->internal_mirrorlist = lr_lrmirrorlist_append_lrmirrorlist(
                                            handle->internal_mirrorlist,
                                            handle->urls_mirrors);

    // Mirrorlist from the LRO_MIRRORLISTURL
    if (handle->mirrorlisturl)
        handle->internal_mirrorlist = lr_lrmirrorlist_append_lrmirrorlist(
                                                handle->internal_mirrorlist,
                                                handle->mirrorlist_mirrors);

    // Mirrorlist from the LRO_METALINKURL
    if (handle->metalinkurl)
        handle->internal_mirrorlist = lr_lrmirrorlist_append_lrmirrorlist(
                                                handle->internal_mirrorlist,
                                                handle->metalink_mirrors);

    // If enabled, sort internal mirrorlist by the connection
    // speed (the LRO_FASTESTMIRROR option)
    if (usefastestmirror) {
        g_debug("%s: Sorting internal mirrorlist by connection speed",
                __func__);
        gboolean ret = lr_fastestmirror_sort_internalmirrorlist(handle, err);
        if (!ret)
            return FALSE;
    }

    // Prepare mirrors (the list that is reported via LRI_MIRRORS)
    // This list contains mirrors from mirrorlist and/or metalink
    // even if they are not explicitly specified, but are available
    // in a local repo (that is specified at the first url of LRO_URLS)

    g_debug("%s: Finalizing mirrors reported via LRI_MIRRORS", __func__);

    handle->mirrors = lr_lrmirrorlist_append_lrmirrorlist(
                                            handle->mirrors,
                                            handle->mirrorlist_mirrors);
    handle->mirrors = lr_lrmirrorlist_append_lrmirrorlist(
                                            handle->mirrors,
                                            handle->metalink_mirrors);

    return TRUE;
}

gboolean
lr_handle_perform(LrHandle *handle, LrResult *result, GError **err)
{
    int ret = TRUE;
    GError *tmp_err = NULL;

    assert(handle);
    assert(!err || *err == NULL);

    if (!result) {
        g_set_error(err, LR_HANDLE_ERROR, LRE_BADFUNCARG,
                    "No result argument passed");
        return FALSE;
    }

    if (!handle->urls && !handle->mirrorlisturl && !handle->metalinkurl) {
        g_set_error(err, LR_HANDLE_ERROR, LRE_NOURL,
                "No LRO_URLS, LRO_MIRRORLISTURL nor LRO_METALINKURL specified");
        return FALSE;
    }

    if (handle->repotype != LR_YUMREPO) {
        g_set_error(err, LR_HANDLE_ERROR, LRE_BADFUNCARG,
                    "Bad LRO_REPOTYPE specified");
        return FALSE;
    }

    /* Setup destination directory */
    if (handle->update) {
        if (!result->destdir) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_INCOMPLETERESULT,
                        "Incomplete result object, destdir is missing");
            return FALSE;
        }
        lr_free(handle->destdir);
        handle->destdir = g_strdup(result->destdir);
    } else if (!handle->destdir && !handle->local) {
        handle->destdir = g_strdup(TMP_DIR_TEMPLATE);
        if (!mkdtemp(handle->destdir)) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_CANNOTCREATETMP,
                        "Cannot create tmpdir: %s", g_strerror(errno));
            return FALSE;
        }
    }

    g_debug("%s: Using dir: %s", __func__, handle->destdir);

    struct sigaction old_sigact;
    if (handle->interruptible) {
        /* Setup sighandler */
        struct sigaction sigact;
        g_debug("%s: Using own SIGINT handler", __func__);
        memset(&sigact, 0, sizeof(old_sigact));
        memset(&sigact, 0, sizeof(sigact));
        sigemptyset(&sigact.sa_mask);
        sigact.sa_handler = lr_sigint_handler;
        sigaddset(&sigact.sa_mask, SIGINT);
        sigact.sa_flags = 0;
        if (sigaction(SIGINT, &sigact, &old_sigact) == -1) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_SIGACTION,
                        "sigaction(SIGINT,,) error");
            return FALSE;
        }
    }

    ret = lr_handle_prepare_internal_mirrorlist(handle,
                                                handle->fastestmirror,
                                                &tmp_err);
    if (!ret) {
        g_debug("Cannot prepare internal mirrorlist: %s", tmp_err->message);
        g_propagate_prefixed_error(err, tmp_err,
                                   "Cannot prepare internal mirrorlist: ");
        return FALSE;
    }

    if (handle->fetchmirrors) {
        /* Only download and parse mirrorlist */
        g_debug("%s: Only fetching mirrorlist/metalink", __func__);
    } else {
        /* Do the other stuff */
        switch (handle->repotype) {
        case LR_YUMREPO:
            g_debug("%s: Downloading/Locating yum repo", __func__);
            ret = lr_yum_perform(handle, result, &tmp_err);
            break;
        default:
            g_debug("%s: Bad repo type", __func__);
            assert(0);
            ret = FALSE;
            g_set_error(err, LR_HANDLE_ERROR, LRE_BADFUNCARG,
                        "Bad repo type: %d", handle->repotype);
            break;
        }
    }

    if (handle->interruptible) {
        /* Restore signal handler */
        g_debug("%s: Restoring an old SIGINT handler", __func__);
        sigaction(SIGINT, &old_sigact, NULL);

        if (lr_interrupt) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_INTERRUPTED,
                        "Librepo was interrupted by a signal");
            if (tmp_err)
                g_error_free(tmp_err);
            return FALSE;
        }
    }

    assert((ret && !tmp_err) || (!ret && tmp_err));

    if (tmp_err)
        g_propagate_error(err, tmp_err);

    return ret;
}

gboolean
lr_handle_getinfo(LrHandle *handle,
                  GError **err,
                  LrHandleInfoOption option,
                  ...)
{
    gboolean rc = TRUE;
    va_list arg;
    char **str;
    long *lnum;
    double *dnum;

    assert(!err || *err == NULL);

    if (!handle) {
        g_set_error(err, LR_HANDLE_ERROR, LRE_BADFUNCARG,
                    "No handle specified");
        return FALSE;
    }

    va_start(arg, option);

    switch (option) {

    case LRI_UPDATE:
        lnum = va_arg(arg, long *);
        *lnum = (long) handle->update;
        break;

    case LRI_MIRRORLIST:
        str = va_arg(arg, char **);
        *str = handle->mirrorlist;
        break;

    case LRI_MIRRORLISTURL:
        str = va_arg(arg, char **);
        *str = handle->mirrorlisturl;
        break;

    case LRI_METALINKURL:
        str = va_arg(arg, char **);
        *str = handle->metalinkurl;
        break;

    case LRI_LOCAL:
        lnum = va_arg(arg, long *);
        *lnum = (long) handle->local;
        break;

    case LRI_PROGRESSCB: {
        LrProgressCb *cb= va_arg(arg, LrProgressCb *);
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

    case LRI_USERAGENT:
        str = va_arg(arg, char **);
        *str = handle->useragent;
        break;

    case LRI_URLS:
    case LRI_YUMDLIST:
    case LRI_YUMBLIST:
    case LRI_HTTPHEADER: {
        char **source_list = NULL;
        char ***strlist = va_arg(arg, char ***);

        if (option == LRI_URLS)
            source_list = handle->urls;
        else if (option == LRI_YUMDLIST)
            source_list = handle->yumdlist;
        else if (option == LRI_YUMBLIST)
            source_list = handle->yumblist;
        else if (option == LRI_HTTPHEADER)
            source_list = handle->httpheader;

        if (!source_list) {
            *strlist = NULL;
            break;
        }

        *strlist = lr_strv_dup(source_list);
        break;
    }

    case LRI_FETCHMIRRORS:
        lnum = va_arg(arg, long *);
        *lnum = (long) handle->fetchmirrors;
        break;

    case LRI_MAXMIRRORTRIES:
        lnum = va_arg(arg, long *);
        *lnum = (long) handle->maxmirrortries;
        break;

    case LRI_VARSUB: {
        LrUrlVars **vars = va_arg(arg, LrUrlVars **);
        *vars = handle->urlvars;
        break;
    }

    case LRI_MIRRORS: {
        int x;
        char ***list = va_arg(arg, char ***);
        *list = NULL;
        LrInternalMirrorlist *ml = handle->mirrors;

        if (!ml)
            // lr_handle_perform() or lr_download_package() was not called yet
            break;

        /* Make list of urls from internal mirrorlist */
        x = 0;
        *list = lr_malloc((g_slist_length(ml) + 1) * sizeof(char *));
        for (LrInternalMirrorlist *elem = ml; elem; elem = g_slist_next(elem)) {
            LrInternalMirror *mirror = elem->data;
            (*list)[x] = g_strdup(mirror->url);
            x++;
        }
        (*list)[x] = NULL;
        break;
    }

    case LRI_METALINK: {
        LrMetalink **metalink = va_arg(arg, LrMetalink **);
        *metalink = handle->metalink;
        break;
    }

    case LRI_FASTESTMIRROR:
        lnum = va_arg(arg, long *);
        *lnum = (long) handle->fastestmirror;
        break;

    case LRI_FASTESTMIRRORCACHE:
        str = va_arg(arg, char **);
        *str = handle->fastestmirrorcache;
        break;

    case LRI_FASTESTMIRRORMAXAGE:
        lnum = va_arg(arg, long *);
        *lnum = (long) handle->fastestmirrormaxage;
        break;

    case LRI_HMFCB: {
        LrHandleMirrorFailureCb *cb= va_arg(arg, LrHandleMirrorFailureCb *);
        *cb = handle->hmfcb;
        break;
    }

    case LRI_SSLVERIFYPEER:
        lnum = va_arg(arg, long *);
        *lnum = (long) handle->sslverifypeer;
        break;

    case LRI_SSLVERIFYHOST:
        lnum = va_arg(arg, long *);
        *lnum = (long) (handle->sslverifyhost ? 1 : 0);
        break;

    case LRI_SSLCLIENTCERT:
        str = va_arg(arg, char **);
        *str = handle->sslclientcert;
        break;

    case LRI_SSLCLIENTKEY:
        str = va_arg(arg, char **);
        *str = handle->sslclientkey;
        break;

    case LRI_SSLCACERT:
        str = va_arg(arg, char **);
        *str = handle->sslcacert;
        break;

    case LRI_IPRESOLVE: {
        LrIpResolveType *type = va_arg(arg, LrIpResolveType *);
        *type = (LrIpResolveType) (handle->ipresolve);
        break;
    }

    case LRI_ALLOWEDMIRRORFAILURES:
        lnum = va_arg(arg, long *);
        *lnum = (long) (handle->allowed_mirror_failures);
        break;

    case LRI_ADAPTIVEMIRRORSORTING:
        lnum = va_arg(arg, long *);
        *lnum = (long) (handle->adaptivemirrorsorting);
        break;

    case LRI_GNUPGHOMEDIR:
        str = va_arg(arg, char **);
        *str = handle->gnupghomedir;
        break;

    case LRI_FASTESTMIRRORTIMEOUT:
        dnum = va_arg(arg, double *);
        *dnum = (double) handle->fastestmirrortimeout;
        break;

    case LRI_OFFLINE:
        lnum = va_arg(arg, long *);
        *lnum = (long) handle->offline;
        break;

    case LRI_LOWSPEEDTIME:
        lnum = va_arg(arg, long *);
        *lnum = (long) (handle->lowspeedtime);
        break;

    case LRI_LOWSPEEDLIMIT:
        lnum = va_arg(arg, long *);
        *lnum = (long) (handle->lowspeedlimit);
        break;

    case LRI_HTTPAUTHMETHODS: {
        LrAuth *auth = va_arg(arg, LrAuth *);
        *auth = handle->httpauthmethods;
        break;
    }

    case LRI_PROXYAUTHMETHODS: {
        LrAuth *auth = va_arg(arg, LrAuth *);
        *auth = handle->proxyauthmethods;
        break;
    }

    case LRI_FTPUSEEPSV:
        lnum = va_arg(arg, long *);
        *lnum = (long) handle->ftpuseepsv;
        break;

    default:
        rc = FALSE;
        g_set_error(err, LR_HANDLE_ERROR, LRE_UNKNOWNOPT,
                    "Unknown option");
        break;
    }

    va_end(arg);
    return rc;
}
