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
#include <unistd.h>
#include <sys/stat.h>

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
    if (gnupghomedir) {
        target->gnupghomedir = g_string_chunk_insert(target->chunk, gnupghomedir);
    }

    return target;
}

void
lr_metadatatarget_free(LrMetadataTarget *target)
{
    if (!target)
        return;
    g_string_chunk_free(target->chunk);
    g_list_free_full(target->err, g_free);
    lr_yum_repo_free(target->repo);
    lr_yum_repomd_free(target->repomd);
    g_free(target);
}

void
lr_metadatatarget_append_error(LrMetadataTarget *target, char *format, ...)
{
    va_list valist;
    va_start(valist, format);
    gchar *error_message = g_strdup_vprintf(format, valist);
    va_end(valist);

    target->err = g_list_append(target->err, error_message);
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
    lr_metadatatarget_append_error(target, err->message);
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
            lr_metadatatarget_append_error(target, "No LRO_URLS, LRO_MIRRORLISTURL nor LRO_METALINKURL specified");
            fillInvalidationValues(fd_list, paths);
            continue;
        }

        if (handle->repotype != LR_YUMREPO) {
            lr_metadatatarget_append_error(target, "Bad LRO_REPOTYPE specified");
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
            lr_metadatatarget_append_error(target, "Cannot prepare internal mirrorlist: %s", err->message);
            fillInvalidationValues(fd_list, paths);
            g_error_free(err);
            continue;
        }

        if (mkdir(handle->destdir, S_IRWXU) == -1 && errno != EEXIST) {
            lr_metadatatarget_append_error(target, "Cannot create tmpdir: %s %s", handle->destdir, g_strerror(errno));
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

        download_target = lr_downloadtarget_new(target->handle,
                                                "repodata/repomd.xml",
                                                NULL,
                                                fd,
                                                NULL,
                                                checksums,
                                                0,
                                                0,
                                                target->progresscb,
                                                target->cbdata,
                                                NULL,
                                                target->mirrorfailurecb,
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
        lr_free(path);
    }
}

// If it returns FALSE then err is set
gboolean
process_repomd_xml(GSList *targets,
                   GSList *fd_list,
                   GSList *paths,
                   GError **err)
{
    GError *error = NULL;

    for (GSList *elem = targets, *fd = fd_list, *path = paths; elem;
         elem = g_slist_next(elem), fd = g_slist_next(fd), path = g_slist_next(path)) {

        LrMetadataTarget *target = elem->data;
        LrHandle *handle;
        gboolean ret;
        int fd_value = *((int *) fd->data);
        const char * err_msg = NULL;

        if (!target->handle || fd_value == -1) {
            goto fail;
        }

        handle = target->handle;
        handle->used_mirror =  g_strdup(target->download_target->usedmirror);
        handle->gnupghomedir = g_strdup(target->gnupghomedir);

        if (target->download_target->rcode != LRE_OK) {
            lr_metadatatarget_append_error(target, (char *) lr_strerror(target->download_target->rcode));
            err_msg = g_list_last(target->err)->data;
            goto fail;
        }

        if (!lr_check_repomd_xml_asc_availability(handle, target->repo, fd_value, path->data, &error)) {
            lr_metadatatarget_append_error(target, error->message);
            err_msg = g_list_last(target->err)->data;
            g_clear_error(&error);
            goto fail;
        }

        lseek(fd_value, SEEK_SET, 0);
        ret = lr_yum_repomd_parse_file(target->repomd, fd_value, lr_xml_parser_warning_logger,
                                       "Repomd xml parser", &error);
        if (!ret) {
            lr_metadatatarget_append_error(target, "Parsing unsuccessful: %s", error->message);
            err_msg = g_list_last(target->err)->data;
            g_clear_error(&error);
            goto fail;
        }

        close(fd_value);
        lr_free(fd->data);
        target->repo->destdir = g_strdup(handle->destdir);
        target->repo->repomd = path->data;
        continue;
    fail:
        if (fd_value != -1) {
            close(fd_value);
        }
        lr_free(path->data);
        lr_free(fd->data);

        // If we failed to download/verify/parse repomd of this LrMetadataTarget call its endcb
        // because we cannot continue and the target is finished.
        if (target->endcb) {
            int ret = target->endcb(target->cbdata, LR_TRANSFER_ERROR,
                                    err_msg ? err_msg : "Unknown error processing repomd.");
            if (ret == LR_CB_ERROR) {
                g_debug("%s: Downloading was aborted by LR_CB_ERROR from end callback", __func__);
                g_set_error(err, LR_DOWNLOADER_ERROR,
                            LRE_CBINTERRUPTED,
                            "Interrupted by LR_CB_ERROR from end callback");
                return FALSE;
            }
        }
    }

    return TRUE;
}

static gboolean
lr_metadata_download_cleanup(GSList *download_targets)
{
    gboolean ret = TRUE;

    for (GSList *elem = download_targets; elem; elem = g_slist_next(elem)) {
        LrDownloadTarget *download_target = elem->data;
        LrMetadataTarget *target = download_target->userdata;
        if (download_target->err)
            lr_metadatatarget_append_error(target, download_target->err);

        if (target->err != NULL) {
            ret = FALSE;
        }

        lr_downloadtarget_free(download_target);
    }
    g_slist_free(download_targets);

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

// metadata is unused here because the LrHandle hmfcb callback is different to the LrMetadataTarget callback
static int
hmfcb_metadata_target_wrapper(void * clientp, const char *msg, const char *url, G_GNUC_UNUSED const char *metadata) {
    LrMetadataTarget *target = clientp;
    if (target->mirrorfailurecb) {
        return target->mirrorfailurecb(target->cbdata, msg, url);
    }

    return LR_CB_OK;
}

static int
usercb_metadata_target_wrapper(void * clientp, double total_to_download, double now_downloaded) {
    LrMetadataTarget *target = clientp;
    if (target->progresscb) {
        return target->progresscb(target->cbdata, total_to_download, now_downloaded);
    }

    return LR_CB_OK;
}

typedef struct {
    LrProgressCb user_cb;
    void *user_data;
    LrHandleMirrorFailureCb hmfcb;
} HandleCallbacksBackup;

static void
restore_handle_callbacks(GSList *targets, GSList *handle_callbacks_backups)
{
    assert(g_slist_length(targets) == g_slist_length(handle_callbacks_backups));

    GSList *elem = targets;
    GSList *backup_handle_elem = handle_callbacks_backups;

    for (; elem; elem = g_slist_next(elem), backup_handle_elem = g_slist_next(backup_handle_elem)) {
        LrMetadataTarget *metadata_target = elem->data;

        // Restore original handle callbacks
        HandleCallbacksBackup *backup = backup_handle_elem->data;
        LrHandle *handle = metadata_target->handle;
        if (handle) {
            handle->user_cb = backup->user_cb;
            handle->user_data = backup->user_data;
            handle->hmfcb = backup->hmfcb;
        }
        lr_free(backup);

    }
    g_slist_free(handle_callbacks_backups);
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

    // Override handle callbacks with callbacks set from LrMetadataTargets.
    // We want to consistently use LrMetadataTarget callbacks for everything.
    // Store them so we can put them back once finished.
    GSList *handle_callbacks_backups = NULL;
    for (GSList *elem = targets; elem; elem = g_slist_next(elem)) {
        LrMetadataTarget *target = elem->data;
        LrHandle *handle = target->handle;
        if (handle) {
            HandleCallbacksBackup * backup = lr_malloc0(sizeof(HandleCallbacksBackup));
            backup->user_cb = handle->user_cb;
            backup->user_data = handle->user_data;
            backup->hmfcb = handle->hmfcb;

            // Use indirect callback wrappers because handle->hmfcb and target->mirrorfailurecb have different types
            handle->user_data = target;
            handle->user_cb = usercb_metadata_target_wrapper;
            handle->hmfcb = hmfcb_metadata_target_wrapper;

            handle_callbacks_backups = g_slist_append(handle_callbacks_backups, backup);
        } else {
            // In case there is no handle add NULL to ensure both targets and handle_callbacks_backups have
            // the same number of elements.
            handle_callbacks_backups = g_slist_append(handle_callbacks_backups, NULL);
        }
    }

    create_repomd_xml_download_targets(targets, &download_targets, &fd_list, &paths);

    if (!lr_download(download_targets, FALSE, err)) {
        restore_handle_callbacks(targets, handle_callbacks_backups);
        return cleanup(download_targets, err);
    }

    if (!process_repomd_xml(targets, fd_list, paths, err)) {
        restore_handle_callbacks(targets, handle_callbacks_backups);
        return cleanup(download_targets, err);
    }

    g_slist_free(fd_list);
    g_slist_free(paths);

    lr_yum_download_repos(targets, err);

    restore_handle_callbacks(targets, handle_callbacks_backups);
    return cleanup(download_targets, err);
}


