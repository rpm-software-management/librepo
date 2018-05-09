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

#define _GNU_SOURCE
#define RESERVE 128

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
    target->err = NULL;

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
    if (target->err != NULL)
        g_list_free(target->err);
    g_free(target);
}

void
lr_metadatatarget_append_error(LrMetadataTarget *target, char *format, ...)
{
    va_list valist;
    size_t length = strlen(format);
    char *error_message = NULL;

    va_start(valist, format);
    while (1) {
        char *arg = va_arg(valist, char*);
        if (arg == NULL)
            break;

        length += strlen(arg);
    }
    length += RESERVE;
    va_end(valist);

    va_start(valist, format);
    error_message = malloc(length * sizeof(char));
    vsnprintf(error_message, length, format, valist);
    va_end(valist);

    target->err = g_list_append(target->err, (gpointer) error_message);
}

static gboolean
lr_setup_sigaction(struct sigaction *old_sigact,
                   gboolean interruptible,
                   GError **err)
{
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
                        "Interrupted by a SIGINT signal");
            return FALSE;
        }
    }

    return TRUE;
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

void
fillInvalidationValues(GSList **fd_list, GSList **paths)
{
    (*fd_list) = appendFdValue((*fd_list), -1);
    (*paths) = appendPath((*paths), "");
}

void
handle_failure(LrMetadataTarget *target,
               GSList **fd_list,
               GSList **paths,
               GError *err)
{
    lr_metadatatarget_append_error(target, err->message, NULL);
    fillInvalidationValues(fd_list, paths);
    g_error_free(err);
}

void
create_repomd_xml_download_targets(GSList *targets,
                                   GSList **download_targets,
                                   GSList **fd_list,
                                   GSList **paths)
{
    for (GSList *elem = targets; elem; elem = g_slist_next(elem)) {
        LrMetadataTarget *target = elem->data;
        LrDownloadTarget *download_target;
        GSList *checksums = NULL;
        GError *err = NULL;
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
            lr_metadatatarget_append_error(target, "No LRO_URLS, LRO_MIRRORLISTURL nor LRO_METALINKURL specified", NULL);
            fillInvalidationValues(fd_list, paths);
            continue;
        }

        if (handle->repotype != LR_YUMREPO) {
            lr_metadatatarget_append_error(target, "Bad LRO_REPOTYPE specified", NULL);
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
                                                   &err)) {
            lr_metadatatarget_append_error(target, "Cannot prepare internal mirrorlist: %s", err->message, NULL);
            fillInvalidationValues(fd_list, paths);
            g_error_free(err);
            continue;
        }

        if (mkdir(handle->destdir, S_IRWXU) == -1 && errno != EEXIST) {
            lr_metadatatarget_append_error(target, "Cannot create tmpdir: %s %s", handle->destdir, g_strerror(errno), NULL);
            fillInvalidationValues(fd_list, paths);
            g_error_free(err);
            continue;
        }

        if (!lr_prepare_repodata_dir(handle, &err)) {
            handle_failure(target, fd_list, paths, err);
            continue;
        }

        if (!handle->update) {
            if (!lr_store_mirrorlist_files(handle, target->repo, &err)) {
                handle_failure(target, fd_list, paths, err);
                continue;
            }

            if (!lr_copy_metalink_content(handle, target->repo, &err)) {
                handle_failure(target, fd_list, paths, err);
                continue;
            }

            if ((fd = lr_prepare_repomd_xml_file(handle, &path, &err)) == -1) {
                handle_failure(target, fd_list, paths, err);
                continue;
            }
        }

        if (handle->metalink && (handle->checks & LR_CHECK_CHECKSUM)) {
            lr_get_best_checksum(handle->metalink, &checksums);
        }

        CbData *cbdata = lr_get_metadata_failure_callback(handle);

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
                                                target,
                                                0,
                                                0,
                                                NULL,
                                                TRUE,
                                                FALSE);

        target->download_target = download_target;
        (*download_targets) = g_slist_append((*download_targets), download_target);

        (*fd_list) = appendFdValue((*fd_list), fd);
        (*paths) = appendPath((*paths), path);
    }
}

void
process_repomd_xml(GSList *targets,
                   GSList *fd_list,
                   GSList *paths)
{
    GError *error = NULL;

    for (GSList *elem = targets, *fd = fd_list, *path = paths; elem;
         elem = g_slist_next(elem), fd = g_slist_next(fd), path = g_slist_next(path)) {

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
            lr_metadatatarget_append_error(target, (char *) lr_strerror(target->download_target->rcode), NULL);
            continue;
        }

        if (!lr_check_repomd_xml_asc_availability(handle, target->repo, fd_value, path->data, &error)) {
            lr_metadatatarget_append_error(target, error->message, NULL);
            g_error_free(error);
            continue;
        }

        lseek(fd_value, SEEK_SET, 0);
        ret = lr_yum_repomd_parse_file(target->repomd, fd_value, lr_xml_parser_warning_logger,
                                       "Repomd xml parser", &error);
        close(fd_value);
        if (!ret) {
            lr_metadatatarget_append_error(target, "Parsing unsuccessful: %s", error->message, NULL);
            lr_free(path->data);
            g_error_free(error);
            continue;
        }

        target->repo->destdir = g_strdup(handle->destdir);
        target->repo->repomd = path->data;
    }
}

static gboolean
lr_metadata_download_cleanup(GSList *download_targets)
{
    gboolean ret = TRUE;

    for (GSList *elem = download_targets; elem; elem = g_slist_next(elem)) {
        LrDownloadTarget *download_target = elem->data;
        LrMetadataTarget *target = download_target->userdata;
        if (download_target->err)
            lr_metadatatarget_append_error(target, download_target->err, NULL);

        if (target->err != NULL) {
            ret = FALSE;
        }

        lr_downloadtarget_free(download_target);
    }

    return ret;
}

gboolean cleanup(GSList *download_targets, GError **err)
{
    struct sigaction old_sigact;
    GError *repo_error = NULL;

    lr_metadata_download_cleanup(download_targets);

    if (!lr_restore_sigaction(&old_sigact, FALSE, &repo_error)) {
        g_propagate_error(err, repo_error);
        return FALSE;
    }

    return *err == NULL;
}

gboolean
lr_download_metadata(GSList *targets,
                     GError **err)
{
    struct sigaction old_sigact;
    GSList *download_targets = NULL;
    GSList *fd_list = NULL;
    GSList *paths = NULL;

    assert(!err || *err == NULL);

    if (!targets)
        return TRUE;

    if (!lr_setup_sigaction(&old_sigact, FALSE, err)) {
        return FALSE;
    }

    create_repomd_xml_download_targets(targets, &download_targets, &fd_list, &paths);

    if (!lr_download(download_targets, FALSE, err)) {
        return cleanup(download_targets, err);
    }

    process_repomd_xml(targets, fd_list, paths);
    lr_yum_download_repos(targets, err);

    return cleanup(download_targets, err);
}


