/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2016  Martin Hatina
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

#define _POSIX_SOURCE
#define _DEFAULT_SOURCE

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <zconf.h>

#include "librepo/librepo.h"

#include "handle_internal.h"
#include "librepo.h"

LrMetadataTarget *
lr_metadatatarget_new(LrHandle *handle,
                      LrYumRepo *repo,
                      LrYumRepoMd *repomd,
                      void *cbdata,
                      GError **err)
{
    LrMetadataTarget *target;

    assert(!err || *err == NULL);

    target = lr_malloc0(sizeof(*target));
    if (!target) {
        g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR, LRE_MEMORY,
                    "Out of memory");
        return NULL;
    }

    target->chunk = g_string_chunk_new(16);

    target->handle = handle;
    target->repo = repo;
    target->repomd = repomd;
    target->cbdata = cbdata;
    target->repomd_records_to_download = 0;
    target->repomd_records_downloaded = 0;
    target->download_target = NULL;
    target->gnupghomedir = NULL;

    return target;
}

LrMetadataTarget *
lr_metadatatarget_new2(LrHandle *handle,
                       void *cbdata,
                       LrProgressCb progresscb,
                       LrMirrorFailureCb mirrorfailure_cb,
                       LrEndCb endcb,
                       const char *gnupghomedir,
                       GError **err)
{
    LrMetadataTarget *target = lr_metadatatarget_new(handle, NULL, NULL, cbdata, err);
    target->progresscb = progresscb;
    target->mirrorfailurecb = mirrorfailure_cb;
    target->endcb = endcb;
    target->gnupghomedir = g_string_chunk_insert(target->chunk, gnupghomedir);

    return target;
}

void
lr_metadatatarget_free(LrMetadataTarget *target)
{
    if (!target)
        return;
    g_string_chunk_free(target->chunk);
    g_free(target);
}

static gboolean
lr_setup_sigaction(struct sigaction *old_sigact,
                   gboolean interruptible,
                   GError **err)
{
    // Setup sighandler
    if (interruptible) {
        struct sigaction sigact;
        g_debug("%s: Using own SIGINT handler", __func__);
        memset(&sigact, 0, sizeof(sigact));
        sigemptyset(&sigact.sa_mask);
        sigact.sa_handler = lr_sigint_handler;
        sigaddset(&sigact.sa_mask, SIGINT);
        sigact.sa_flags = SA_RESTART;
        if (sigaction(SIGINT, &sigact, old_sigact) == -1) {
            g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR, LRE_SIGACTION,
                        "Cannot set Librepo SIGINT handler");
            return FALSE;
        }
    }
    return TRUE;
}

static gboolean
lr_restore_sigaction(struct sigaction *old_sigact,
                     gboolean interruptible,
                     GError **err)
{
    // Restore original signal handler
    if (interruptible) {
        g_debug("%s: Restoring an old SIGINT handler", __func__);
        sigaction(SIGINT, old_sigact, NULL);
        if (lr_interrupt) {
            if (err && *err != NULL)
                g_clear_error(err);
            g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR, LRE_INTERRUPTED,
                        "Insterupted by a SIGINT signal");
            return FALSE;
        }
    }

    return TRUE;
}

static gboolean
lr_metadata_download_cleanup(GSList *download_targets,
                             GError **err)
{
    gboolean ret = TRUE;

    // Copy download statuses from download_targets to targets
    for (GSList *elem = download_targets; elem; elem = g_slist_next(elem)) {
        LrDownloadTarget *download_target = elem->data;
        LrMetadataTarget *target = download_target->userdata;
        if (download_target->err)
            target->err = g_string_chunk_insert(target->chunk,
                                                download_target->err);

        if (target->err != NULL) {
            ret = FALSE;
            g_set_error(err, LR_DOWNLOADER_ERROR, 1,
                        "Cannot download repomd.xml: %s",target->err);
        }

        lr_downloadtarget_free(download_target);
    }

    return ret;
}

GSList *
appendFdValue(GSList *fd_list, int fd)
{
    int *fd_allocated = malloc(sizeof(int));
    *fd_allocated = fd;
    fd_list = g_slist_append(fd_list, fd_allocated);
    return fd_list;
}

GSList *
appendPath(GSList *paths, const char *path)
{
    paths = g_slist_append(paths, g_strdup(path));
    return paths;
}

void fillInvalidationValues(GSList **fd_list, GSList **paths)
{
    (*fd_list) = appendFdValue((*fd_list), -1);
    (*paths) = appendPath((*paths), "");
}

void
create_repomd_xml_download_targets(GSList *targets,
                                   GSList **download_targets,
                                   GSList **fd_list,
                                   GSList **paths,
                                   GError **err)
{
    GError *repo_error = NULL;

    for (GSList *elem = targets;; elem = g_slist_next(elem)) {
        if (repo_error != NULL) {
            g_propagate_error(err, repo_error);
            repo_error = NULL;
        }

        if (elem == NULL)
            break;

        LrMetadataTarget *target = elem->data;
        LrDownloadTarget *download_target;
        GSList *checksums = NULL;
        LrHandle *handle;
        char *path = NULL;
        int fd = -1;

        if (!target->handle) {
            /* every target must have fd and path, even invalid */
            fillInvalidationValues(fd_list, paths);
            continue;
        }

        handle = target->handle;

        if (!handle->urls && !handle->mirrorlisturl && !handle->metalinkurl) {
            g_set_error(&repo_error, LR_HANDLE_ERROR, LRE_NOURL,
                        "No LRO_URLS, LRO_MIRRORLISTURL nor LRO_METALINKURL specified");
            /* every target must have fd and path, even invalid */
            fillInvalidationValues(fd_list, paths);
            continue;
        }

        if (handle->repotype != LR_YUMREPO) {
            g_set_error(&repo_error, LR_HANDLE_ERROR, LRE_BADFUNCARG,
                        "Bad LRO_REPOTYPE specified");
            /* every target must have fd and path, even invalid */
            fillInvalidationValues(fd_list, paths);
            continue;
        }

        if (target->repo == NULL) {
            target->repo = lr_yum_repo_init();
        }
        if (target->repomd == NULL) {
            target->repomd = lr_yum_repomd_init();
        }

        if (!lr_handle_prepare_internal_mirrorlist(handle,
                                                   handle->fastestmirror,
                                                   &repo_error)) {
            g_debug("Cannot prepare internal mirrorlist: %s", repo_error->message);
            fillInvalidationValues(fd_list, paths);
            continue;
        }

        if (mkdir(handle->destdir, S_IRWXU) == -1 && errno != EEXIST) {
            g_set_error(&repo_error, LR_HANDLE_ERROR, LRE_CANNOTCREATETMP,
                        "Cannot create tmpdir: %s %s", handle->destdir, g_strerror(errno));
            /* every target must have fd and path, even invalid */
            fillInvalidationValues(fd_list, paths);
            continue;
        }

        if (!lr_prepare_repodata_dir(handle, &repo_error)) {
            /* every target must have fd and path, even invalid */
            fillInvalidationValues(fd_list, paths);
            continue;
        }

        if (!handle->update) {
            if (!lr_store_mirrorlist_files(handle, target->repo, &repo_error)) {
                /* every target must have fd and path, even invalid */
                fillInvalidationValues(fd_list, paths);
                continue;
            }

            if (!lr_copy_metalink_content(handle, target->repo, &repo_error)) {
                /* every target must have fd and path, even invalid */
                fillInvalidationValues(fd_list, paths);
                continue;
            }

            if ((fd = lr_prepare_repomd_xml_file(handle, &path, &repo_error)) == -1) {
                /* every target must have fd and path, even invalid */
                fillInvalidationValues(fd_list, paths);
                continue;
            }
        }

        if (handle->metalink && (handle->checks & LR_CHECK_CHECKSUM)) {
            lr_get_best_checksum(handle->metalink, &checksums);
        }

        CbData *cbdata = lr_get_repomd_download_callbacks(handle);

        download_target = lr_downloadtarget_new(target->handle,
                                                "repodata/repomd.xml",
                                                NULL,
                                                fd,
                                                NULL,
                                                checksums,
                                                0,
                                                0,
                                                NULL,
                                                cbdata,
                                                NULL,
                                                (cbdata) ? hmfcb : NULL,
                                                (cbdata) ? starttranscb : NULL,
                                                target,
                                                0,
                                                0,
                                                TRUE);

        target->download_target = download_target;
        (*download_targets) = g_slist_append((*download_targets), download_target);

        (*fd_list) = appendFdValue((*fd_list), fd);
        (*paths) = appendPath((*paths), path);
    }
}

void
process_repomd_xml(GSList *targets,
                   GSList *fd_list,
                   GSList *paths,
                   GError **err)
{
    GError *repo_error = NULL;

    for (GSList *elem = targets, *fd = fd_list, *path = paths;;
         elem = g_slist_next(elem), fd = g_slist_next(fd), path = g_slist_next(path)) {
        if (repo_error != NULL) {
            g_propagate_error(err, repo_error);
            repo_error = NULL;
        }

        if (elem == NULL)
            break;

        LrMetadataTarget *target = elem->data;
        LrHandle *handle;
        gboolean ret;
        int fd_value = *((int *) fd->data);

        if (!target->handle || fd_value == -1) {
            continue;
        }

        handle = target->handle;
        handle->used_mirror =  g_strdup(target->download_target->usedmirror);
        handle->gnupghomedir = g_strdup(target->gnupghomedir);

        if (target->download_target->rcode != LRE_OK) {
            g_set_error(err, LR_YUM_ERROR, target->download_target->rcode,
                        "%s", lr_strerror(target->download_target->rcode));
            g_debug("%s: %s", __func__, lr_strerror(target->download_target->rcode));
            continue;
        }

        if (!lr_check_repomd_xml_asc_availability(handle, target->repo, fd_value, path->data, &repo_error)) {
            continue;
        }

        lseek(fd_value, SEEK_SET, 0);
        ret = lr_yum_repomd_parse_file(target->repomd, fd_value, lr_xml_parser_warning_logger,
                                       "Repomd xml parser", &repo_error);
        close(fd_value);
        if (!ret) {
            g_debug("%s: Parsing unsuccessful: %s", __func__, repo_error->message);
            lr_free(path->data);
            continue;
        }

        target->repo->destdir = g_strdup(handle->destdir);
        target->repo->repomd = path->data;
    }

    if (repo_error != NULL) {
        g_propagate_error(err, repo_error);
        repo_error = NULL;
    }
}

gboolean
lr_download_metadata(GSList *targets,
                     GError **err)
{
    struct sigaction old_sigact;
    GSList *download_targets = NULL;
    GSList *fd_list = NULL;
    GSList *paths = NULL;
    gboolean interruptible = FALSE;
    GError *repo_error = NULL;

    assert(!err || *err == NULL);

    if (!targets)
        return TRUE;

    if (!lr_setup_sigaction(&old_sigact, interruptible, err)) {
        return FALSE;
    }

    create_repomd_xml_download_targets(targets, &download_targets, &fd_list, &paths, err);

    // Start downloading
    if (!lr_download(download_targets, FALSE, &repo_error)) {
        g_propagate_error(err, repo_error);
        goto cleanup;
    }

    process_repomd_xml(targets, fd_list, paths, err);

    if (!lr_yum_download_repos(targets, &repo_error)) {
        g_propagate_error(err, repo_error);
        return FALSE;
    }

    cleanup:
    lr_metadata_download_cleanup(download_targets, &repo_error);

    if (!lr_restore_sigaction(&old_sigact, interruptible, &repo_error)) {
        g_propagate_error(err, repo_error);
        return FALSE;
    }

    return *err == NULL;
}


