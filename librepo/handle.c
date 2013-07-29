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

CURL *
lr_get_curl_handle()
{
    CURL *h = curl_easy_init();
    curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(h, CURLOPT_MAXREDIRS, 6);
    return h;
}

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

LrHandle *
lr_handle_init()
{
    LrHandle *handle;
    CURL *curl = lr_get_curl_handle();

    if (!curl)
        return NULL;

    handle = lr_malloc0(sizeof(LrHandle));
    handle->curl_handle = curl;
    handle->retries = 1;
    handle->mirrorlist_fd = -1;
    handle->checks |= LR_CHECK_CHECKSUM;

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
    lr_free(handle->baseurl);
    lr_free(handle->mirrorlist);
    lr_free(handle->used_mirror);
    lr_free(handle->destdir);
    lr_free(handle->useragent);
    lr_lrmirrorlist_free(handle->internal_mirrorlist);
    lr_lrmirrorlist_free(handle->mirrors);
    lr_metalink_free(handle->metalink);
    lr_handle_free_list(&handle->yumdlist);
    lr_handle_free_list(&handle->yumblist);
    lr_urlvars_free(handle->urlvars);
    lr_free(handle);
}

int
lr_handle_setopt(LrHandle *handle, LrHandleOption option, ...)
{
    LrRc ret = LRE_OK;
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
        if (handle->baseurl) lr_free(handle->baseurl);
        handle->baseurl = g_strdup(va_arg(arg, char *));
        if (handle->internal_mirrorlist) {
            // Clear previous mirrorlist stuff
            lr_lrmirrorlist_free(handle->internal_mirrorlist);
            handle->internal_mirrorlist = NULL;
        }
        break;

    case LRO_MIRRORLIST:
        if (handle->mirrorlist) lr_free(handle->mirrorlist);
        handle->mirrorlist = g_strdup(va_arg(arg, char *));
        if (handle->internal_mirrorlist) {
            // Clear previous mirrorlist stuff
            lr_lrmirrorlist_free(handle->internal_mirrorlist);
            handle->internal_mirrorlist = NULL;
            lr_lrmirrorlist_free(handle->mirrors);
            handle->mirrors = NULL;
            lr_metalink_free(handle->metalink);
            handle->metalink = NULL;
            if (handle->mirrorlist_fd != -1)
                close(handle->mirrorlist_fd);
            handle->mirrorlist_fd = -1;
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
            ret = LRE_BADOPTARG;
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
        handle->user_cb = va_arg(arg, LrProgressCb);
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
        if (handle->destdir) lr_free(handle->destdir);
        handle->destdir = g_strdup(va_arg(arg, char *));
        break;

    case LRO_REPOTYPE:
        handle->repotype = va_arg(arg, LrRepotype);
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
            (*handle_list)[x] = g_strdup(list[x]);
        break;
    }

    case LRO_FETCHMIRRORS:
        handle->fetchmirrors = va_arg(arg, long) ? 1 : 0;
        break;

    case LRO_MAXMIRRORTRIES:
        handle->maxmirrortries = va_arg(arg, long);
        if (handle->maxmirrortries < 0)
            handle->maxmirrortries = 0;
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

    default:
        ret = LRE_UNKNOWNOPT;
        break;
    };

    /* Handle CURL error return code */
    if (ret == LRE_OK && c_rc != CURLE_OK) {
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

#define TYPE_METALINK   1
#define TYPE_MIRRORLIST 2

int
lr_handle_prepare_internal_mirrorlist(LrHandle *handle, GError **err)
{
    int rc = LRE_OK;
    char *metalink_suffix = NULL;
    char *local_path = NULL;

    assert(!err || *err == NULL);

    if (handle->internal_mirrorlist)
        return LRE_OK;  /* Internal mirrorlist already exists */

    if (!handle->baseurl && !handle->mirrorlist) {
        g_set_error(err, LR_HANDLE_ERROR, LRE_NOURL,
                    "No usable URL (no base url nor mirrorlist url)");
        return LRE_NOURL;
    }

    g_debug("Preparing internal mirrorlist");

    if (handle->repotype == LR_YUMREPO)
        metalink_suffix = "repodata/repomd.xml";

    /* Create internal mirrorlist */
    handle->internal_mirrorlist = NULL;

    /*
     * handle->baseurl (LRO_URL)
     */

    if (handle->baseurl) {
        /* Repository URL specified by user insert it as the first element */

        char *url = NULL;

        if (strstr(handle->baseurl, "://")) {
            /* Base URL has specified protocol */
            url = g_strdup(handle->baseurl);
            if (!strncmp(handle->baseurl, "file://", 7))
                local_path = handle->baseurl + 7;
        } else {
            /* No protocol specified - if local path => prepend file:// */
            local_path = handle->baseurl;
            if (handle->baseurl[0] == '/') {
                /* Base URL is absolute path */
                url = g_strconcat("file://", handle->baseurl, NULL);
            } else {
                /* Base URL is relative path */
                char *resolved_path = realpath(handle->baseurl, NULL);
                if (!resolved_path) {
                    g_debug("%s: realpath: %s", __func__, strerror(errno));
                    g_set_error(err, LR_HANDLE_ERROR, LRE_BADURL,
                                "realpath(%s) error: %s",
                                handle->baseurl, strerror(errno));
                    return LRE_BADURL;
                }
                url = g_strconcat("file://", resolved_path, NULL);
                free(resolved_path);
            }
        }

        if (url) {
            handle->internal_mirrorlist = lr_lrmirrorlist_append_url(
                                                handle->internal_mirrorlist,
                                                url,
                                                handle->urlvars);
            lr_free(url);
        }
    }

    /*
     * Mirrorlist (LRO_MIRRORLIST or local repo)
     */

    int include_in_internal_mirrorlist = (handle->mirrorlist) ? 1 : 0;
    LrMetalink *metalink = NULL;
    LrMirrorlist *mirrorlist = NULL;

    if (handle->mirrorlist_fd == -1) {
        /* If handle->mirrorlist_fd != -1 then we should have a mirrorlist
         * already parsed (and handle->mirrors filled). */

        int mirrors_fd = -1;
        int mirror_type = 0;

        if (!handle->mirrorlist && handle->baseurl && local_path) {
            /* We have a local repository and no mirrorlist specified,
             * just try to load local mirrorlist and do not include its
             * content into the internal_mirrorlist, just fill mirrors */

            char *full_path = NULL; // Path to local metalink.xml/mirrorlist file
            full_path = lr_pathconcat(local_path, "metalink.xml", NULL);
            if (access(full_path, F_OK) == 0) {
                g_debug("%s: Local metalink.xml found", __func__);
                mirror_type = TYPE_METALINK;
                //repo->mirrorlist = path;
            } else {
                lr_free(full_path);
                full_path = lr_pathconcat(local_path, "mirrorlist", NULL);
                if (access(full_path, F_OK) == 0) {
                    g_debug("%s: Local mirrorlist found", __func__);
                    mirror_type = TYPE_MIRRORLIST;
                    //repo->mirrorlist = path;
                } else {
                    lr_free(full_path);
                    full_path = NULL;
                }
            }

            if (full_path) {
                mirrors_fd = open(full_path, O_RDONLY);
                lr_free(full_path);
                if (mirrors_fd < 1) {
                    rc = LRE_IO;
                    g_debug("%s: Cannot open: %s", __func__, full_path);
                    g_set_error(err, LR_HANDLE_ERROR, LRE_IO,
                                "Cannot open %s: %s",
                                full_path, strerror(errno));
                }
            }
        } else if (handle->mirrorlist) {
            /* Download and parse remote metalink or mirrorlist */

            mirrors_fd = lr_gettmpfile();
            if (mirrors_fd < 1) {
                rc = LRE_IO;
                g_debug("%s: Cannot create a temporary file", __func__);
                g_set_error(err, LR_HANDLE_ERROR, LRE_IO,
                            "Cannot create a temporary file");
                goto mirrorlist_error;
            }

            char *prefixed_url =  lr_prepend_url_protocol(handle->mirrorlist);
            char *mirrorlist_url = lr_url_substitute(prefixed_url,
                                                     handle->urlvars);
            lr_free(prefixed_url);
            rc = lr_download_url(handle, mirrorlist_url, mirrors_fd, err);
            lr_free(mirrorlist_url);

            if (rc != LRE_OK)
                goto mirrorlist_error;

            if (lseek(mirrors_fd, 0, SEEK_SET) != 0) {
                rc = LRE_IO;
                g_set_error(err, LR_HANDLE_ERROR, LRE_IO,
                            "lseek(%d, 0, SEEK_SET) error: %s",
                            mirrors_fd, strerror(errno));
                goto mirrorlist_error;
            }

            // Determine mirrorlist type
            if (strstr(handle->mirrorlist, "metalink"))
                mirror_type = TYPE_METALINK;
            else
                mirror_type = TYPE_MIRRORLIST;
        }

        if (mirrors_fd > 0) {
            /* We got fd of mirrorlist - parse it and fill handle */

            g_debug("%s: Got fd", __func__);

            if (mirror_type == TYPE_METALINK) {
                /* Metalink */
                g_debug("%s: Got metalink", __func__);

                /* Parse metalink */
                GError *tmp_err = NULL;
                metalink = lr_metalink_init();
                rc = lr_metalink_parse_file(metalink,
                                            mirrors_fd,
                                            "repomd.xml",
                                            &tmp_err);
                if (rc != LRE_OK) {
                    assert(tmp_err);
                    g_debug("%s: Cannot parse metalink (%d)", __func__, rc);
                    g_propagate_error(err, tmp_err);
                    goto mirrorlist_error;
                }

                if (strcmp("repomd.xml", metalink->filename)) {
                    g_debug("%s: No repomd.xml file in metalink", __func__);
                    rc = LRE_MLBAD;
                    g_set_error(err, LR_HANDLE_ERROR, LRE_MLBAD,
                                "No repomd.xml file in metalink (%s)",
                                metalink->filename);
                    goto mirrorlist_error;
                }

                if (!metalink->urls) {
                    g_debug("%s: No URLs in metalink", __func__);
                    rc = LRE_MLBAD;
                    g_set_error(err, LR_HANDLE_ERROR, LRE_MLBAD,
                                "No URLs in metalink");
                    goto mirrorlist_error;
                }
            } else if (mirror_type == TYPE_MIRRORLIST) {
                /* Mirrorlist */
                g_debug("%s: Got mirrorlist", __func__);

                mirrorlist = lr_mirrorlist_init();

                GError *tmp_err = NULL;
                rc = lr_mirrorlist_parse_file(mirrorlist, mirrors_fd, &tmp_err);
                if (rc != LRE_OK) {
                    assert(tmp_err);
                    g_debug("%s: Cannot parse mirrorlist (%d)", __func__, rc);
                    g_propagate_error(err, tmp_err);
                    goto mirrorlist_error;
                }

                if (!mirrorlist->urls) {
                    rc = LRE_MLBAD;
                    g_debug("%s: No URLs in mirrorlist (%d)", __func__, rc);
                    g_set_error(err, LR_HANDLE_ERROR, LRE_MLBAD,
                                "No URLs in mirrorlist (%d)", rc);
                    goto mirrorlist_error;
                }
            } else {
                assert(0); // This shoudn't happend
            }

            handle->mirrorlist_fd = mirrors_fd;
        } // end of if (mirrors_fd > 0)

        if (rc == LRE_OK) {
            /* Set fill mirrorlist stuff at the handle  */

            assert(!err || *err == NULL);
            assert(handle->mirrors == NULL);

            if (metalink) {
                handle->metalink = metalink;
                handle->mirrors = lr_lrmirrorlist_append_metalink(
                                                            handle->mirrors,
                                                            metalink,
                                                            metalink_suffix,
                                                            handle->urlvars);
            }

            if (mirrorlist) {
                handle->mirrors = lr_lrmirrorlist_append_mirrorlist(
                                                            handle->mirrors,
                                                            mirrorlist,
                                                            handle->urlvars);
            }
        }

mirrorlist_error:

        if (rc != LRE_OK) {
            assert(!err || *err);

            if (mirrors_fd > 0) {
                close(mirrors_fd);
                mirrors_fd = -1;
            }
            lr_metalink_free(metalink);
            metalink = NULL;
        }

        lr_mirrorlist_free(mirrorlist);
        mirrorlist = NULL;
    } // end of if (handle->mirrorlist_fd == -1)


    // Append mirrorlist from handle->mirrors to the internal mirrorlist
    if (include_in_internal_mirrorlist && handle->mirrors) {
        g_debug("%s: Mirrorlist will be used for downloading if needed", __func__);
        handle->internal_mirrorlist = lr_lrmirrorlist_append_lrmirrorlist(
                                                handle->internal_mirrorlist,
                                                handle->mirrors);
    }

    return rc;
}

int
lr_handle_perform(LrHandle *handle, LrResult *result, GError **err)
{
    int rc = LRE_OK;
    GError *tmp_err = NULL;

    assert(handle);
    assert(!err || *err == NULL);

    if (!result) {
        g_set_error(err, LR_HANDLE_ERROR, LRE_BADFUNCARG,
                    "No result argument passed");
        return LRE_BADFUNCARG;
    }

    if (!handle->baseurl && !handle->mirrorlist) {
        g_set_error(err, LR_HANDLE_ERROR, LRE_NOURL,
                    "No LRO_URL nor LRO_MIRRORLIST specified");
        return LRE_NOURL;
    }

    if (handle->repotype != LR_YUMREPO) {
        g_set_error(err, LR_HANDLE_ERROR, LRE_BADFUNCARG,
                    "Bad LRO_REPOTYPE specified");
        return LRE_BADFUNCARG;
    }

    /* Setup destination directory */
    if (handle->update) {
        if (!result->destdir) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_INCOMPLETERESULT,
                        "Incomplete result object, destdir is missing");
            return LRE_INCOMPLETERESULT;
        }
        lr_free(handle->destdir);
        handle->destdir = g_strdup(result->destdir);
    } else if (!handle->destdir && !handle->local) {
        handle->destdir = g_strdup(TMP_DIR_TEMPLATE);
        if (!mkdtemp(handle->destdir)) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_CANNOTCREATETMP,
                        "Cannot create tmpdir: %s", strerror(errno));
            return LRE_CANNOTCREATETMP;
        }
    }

    g_debug("%s: Using dir: %s", __func__, handle->destdir);

    struct sigaction old_sigact;
    if (handle->interruptible) {
        /* Setup sighandler */
        g_debug("%s: Using own SIGINT handler", __func__);
        struct sigaction sigact;
        sigact.sa_handler = lr_sigint_handler;
        sigaddset(&sigact.sa_mask, SIGINT);
        sigact.sa_flags = 0;
        if (sigaction(SIGINT, &sigact, &old_sigact) == -1) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_SIGACTION,
                        "sigaction(SIGINT,,) error");
            return LRE_SIGACTION;
        }
    }

    rc = lr_handle_prepare_internal_mirrorlist(handle, NULL);

    if (handle->fetchmirrors) {
        /* Only download and parse mirrorlist */
        g_debug("%s: Only fetching mirrorlist/metalink", __func__);
    } else {
        /* Do the other stuff */
        switch (handle->repotype) {
        case LR_YUMREPO:
            g_debug("%s: Downloading/Locating yum repo", __func__);
            rc = lr_yum_perform(handle, result, &tmp_err);
            break;
        default:
            g_debug("%s: Bad repo type", __func__);
            assert(0);
            break;
        };
    }

    if (handle->interruptible) {
        /* Restore signal handler */
        g_debug("%s: Restoring an old SIGINT handler", __func__);
        if (sigaction(SIGINT, &old_sigact, NULL) == -1) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_SIGACTION,
                        "sigaction(SIGINT,,) error");
            g_error_free(tmp_err);
            return LRE_SIGACTION;
        }

        if (lr_interrupt) {
            g_set_error(err, LR_HANDLE_ERROR, LRE_INTERRUPTED,
                        "Librepo was interrupted by a signal");
            g_error_free(tmp_err);
            return LRE_INTERRUPTED;
        }
    }

    assert((rc == LRE_OK) || tmp_err);

    if (tmp_err)
        g_propagate_error(err, tmp_err);

    return rc;
}

int
lr_handle_getinfo(LrHandle *handle, LrHandleOption option, ...)
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

    case LRI_YUMDLIST: {
        int x;
        guint length;
        char ***strlist = va_arg(arg, char ***);

        if (!handle->yumdlist) {
            *strlist = NULL;
            break;
        }

        x = 0;
        length = g_strv_length(handle->yumdlist);
        *strlist = lr_malloc((length + 1) * sizeof(char *));
        for (char *item = handle->yumdlist[x]; item;) {
            (*strlist)[x] = g_strdup(item);
            x++;
            item = handle->yumdlist[x];
        }
        (*strlist)[x] = NULL;
        break;
    }

    case LRI_YUMBLIST: {
        int x;
        guint length;
        char ***strlist = va_arg(arg, char ***);

        if (!handle->yumblist) {
            *strlist = NULL;
            break;
        }

        x = 0;
        length = g_strv_length(handle->yumblist);
        *strlist = lr_malloc((length + 1) * sizeof(char *));
        for (char *item = handle->yumblist[x]; item;) {
            (*strlist)[x] = g_strdup(item);
            x++;
            item = handle->yumblist[x];
        }
        (*strlist)[x] = NULL;
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

    default:
        rc = LRE_UNKNOWNOPT;
        break;
    }

    va_end(arg);
    return rc;
}
