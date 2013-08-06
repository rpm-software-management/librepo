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

#define _GNU_SOURCE  // for GNU basename() implementation from string.h
#include <glib.h>
#include <glib/gstdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "types.h"
#include "util.h"
#include "package_downloader.h"
#include "handle_internal.h"
#include "downloader.h"

/* Do NOT use resume on successfully downloaded files - download will fail */

gboolean
lr_download_package(LrHandle *handle,
                    const char *relative_url,
                    const char *dest,
                    LrChecksumType checksum_type,
                    const char *checksum,
                    const char *base_url,
                    gboolean resume,
                    GError **err)
{
    gboolean ret = TRUE;
    int fd;
    char *dest_path;
    char *file_basename;
    char *dest_basename;
    int open_flags = O_CREAT|O_TRUNC|O_RDWR;
    struct sigaction old_sigact;
    GError *tmp_err = NULL;

    assert(handle);
    assert(!err || *err == NULL);

    // Check repotype
    if (handle->repotype != LR_YUMREPO) {
        g_debug("%s: Bad repo type", __func__);
        assert(0);
        g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR, LRE_BADFUNCARG,
                    "Bad repo type");
        return FALSE;
    }

    // Setup sighandler
    if (handle->interruptible) {
        g_debug("%s: Using own SIGINT handler", __func__);
        struct sigaction sigact;
        sigact.sa_handler = lr_sigint_handler;
        sigaddset(&sigact.sa_mask, SIGINT);
        sigact.sa_flags = SA_RESTART;
        if (sigaction(SIGINT, &sigact, &old_sigact) == -1) {
            g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR, LRE_SIGACTION,
                        "Cannot set Librepo SIGINT handler");
            return FALSE;
        }
    }

    ret = lr_handle_prepare_internal_mirrorlist(handle, err);
    if (!ret)
        // TODO: Restore original sighandler!!
        return FALSE;

    file_basename = basename(relative_url);
    dest_basename = (dest) ? basename(dest) : "";

    /* Get/Build path for destination file */
    if (dest) {
        /* Use path specified during function call */
        if (!dest_basename || dest_basename[0] == '\0') {
            /* Only directory is specified */
            dest_path = lr_pathconcat(dest, file_basename, NULL);
        } else {
            /* Whole path including filename is specified */
            dest_path = g_strdup(dest);
        }
    } else {
        /* No path specified */
        if (!handle->destdir) {
            /* No destdir in handle - use current working dir */
            dest_path = g_strdup(file_basename);
        } else {
            /* Use dir specified in handle */
            dest_path = lr_pathconcat(handle->destdir, file_basename, NULL);
        }
    }

    if (g_access(dest_path, R_OK) == 0
        && checksum
        && checksum_type != LR_CHECKSUM_UNKNOWN)
    {
        /* Check checksum of the existing file (if the file exists).
         * If the file exists and checksum is ok, then is pointless to
         * download the file again.
         * Moreover, if the resume is enabled and the file is already
         * completely downloaded, then the download is going to fail. */
        int fd_r = open(dest_path, O_RDONLY);
        if (fd_r != -1) {
            gboolean matches;
            ret = lr_checksum_fd_cmp(checksum_type,
                                     fd_r,
                                     checksum,
                                     0,
                                     &matches,
                                     NULL);
            close(fd_r);
            if (ret && matches) {
                g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR, LRE_ALREADYDOWNLOADED,
                            "Package: %s is already downloaded", dest_path);
                lr_free(dest_path);
                return FALSE;
            }
        }
    }

    if (resume)
        open_flags &= ~O_TRUNC;  // Do NOT truncate the dest file

    fd = open(dest_path, open_flags, 0666);
    if (fd < 0) {
        g_debug("%s: open(\"%s\"): %s", __func__, dest_path, strerror(errno));
        g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR, LRE_IO,
                    "Cannot open %s: %s", dest_path, strerror(errno));
        lr_free(dest_path);
        return FALSE;
    }
    lr_free(dest_path);

    // Download!
    g_debug("%s: Downloading package: %s", __func__, relative_url);
    LrDownloadTarget *target = lr_downloadtarget_new(relative_url,
                                                      base_url,
                                                      fd,
                                                      checksum_type,
                                                      checksum,
                                                      resume,
                                                      handle->user_cb,
                                                      handle->user_data,
                                                      NULL);
    ret = lr_download_target(handle, target, &tmp_err);
    assert((ret && !tmp_err) || (!ret && tmp_err));
    assert(!target->err || !ret);

    lr_downloadtarget_free(target);

    if (handle->interruptible) {
        /* Restore signal handler */
        g_debug("%s: Restoring an old SIGINT handler", __func__);
        sigaction(SIGINT, &old_sigact, NULL);
    }

    if (lr_interrupt) {
        if (tmp_err)
            g_error_free(tmp_err);

        g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR, LRE_INTERRUPTED,
                    "Insterupted by a SIGINT signal");
        return FALSE;
    }

    if (tmp_err) {
        g_propagate_error(err, tmp_err);
        return FALSE;
    }

    return TRUE;
}

LrPackageTarget *
lr_packagetarget_new(const char *relative_url,
                     const char *dest,
                     LrChecksumType checksum_type,
                     const char *checksum,
                     const char *base_url,
                     gboolean resume,
                     LrProgressCb progresscb,
                     void *cbdata,
                     GError **err)
{
    LrPackageTarget *target;

    assert(relative_url);
    assert(!err || *err == NULL);

    target = lr_malloc0(sizeof(*target));
    if (!target) {
        g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR, LRE_MEMORY,
                    "Out of memory");
        return NULL;
    }

    target->chunk = g_string_chunk_new(16);

    target->relative_url = lr_string_chunk_insert(target->chunk, relative_url);
    target->dest = lr_string_chunk_insert(target->chunk, dest);
    target->checksum_type = checksum_type;
    target->checksum = lr_string_chunk_insert(target->chunk, checksum);
    target->base_url = lr_string_chunk_insert(target->chunk, base_url);
    target->resume = resume;
    target->progresscb = progresscb;
    target->cbdata = cbdata;

    return target;
}

void
lr_packagetarget_free(LrPackageTarget *target)
{
    g_string_chunk_free(target->chunk);
    g_free(target);
}

gboolean
lr_download_packages(LrHandle *handle,
                     GSList *targets,
                     gboolean failfast,
                     GError **err)
{
    gboolean ret;
    struct sigaction old_sigact;
    GSList *downloadtargets = NULL;

    assert(handle);
    assert(!err || *err == NULL);

    if (!targets)
        return TRUE;

    // Check repotype
    if (handle->repotype != LR_YUMREPO) {
        g_debug("%s: Bad repo type", __func__);
        assert(0);
        g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR, LRE_BADFUNCARG,
                    "Bad repo type");
        return FALSE;
    }

    // Setup sighandler
    if (handle->interruptible) {
        g_debug("%s: Using own SIGINT handler", __func__);
        struct sigaction sigact;
        sigact.sa_handler = lr_sigint_handler;
        sigaddset(&sigact.sa_mask, SIGINT);
        sigact.sa_flags = SA_RESTART;
        if (sigaction(SIGINT, &sigact, &old_sigact) == -1) {
            g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR, LRE_SIGACTION,
                        "Cannot set Librepo SIGINT handler");
            return FALSE;
        }
    }

    ret = lr_handle_prepare_internal_mirrorlist(handle, err);
    if (!ret)
        goto cleanup;


    for (GSList *elem = targets; elem; elem = g_slist_next(elem)) {
        int fd;
        gchar *local_path;
        LrPackageTarget *packagetarget = elem->data;
        LrDownloadTarget *downloadtarget;

        // Prepare destination filename
        if (packagetarget->dest) {
            if (g_str_has_suffix(packagetarget->dest, ""G_DIR_SEPARATOR_S)) {
                // Dir specified (nobasename specified)
                gchar *file_basename = g_path_get_basename(packagetarget->relative_url);
                local_path = g_build_filename(packagetarget->dest,
                                              file_basename,
                                              NULL);
                g_free(file_basename);
            } else {
                local_path = g_strdup(packagetarget->dest);
            }
        } else {
            // No destination path specified
            local_path = g_path_get_basename(packagetarget->relative_url);
        }

        packagetarget->local_path = g_string_chunk_insert(packagetarget->chunk,
                                                          local_path);
        g_free(local_path);

        // If the file exists and checksum is passed, check if we need to
        // download the file again
        if (g_access(packagetarget->local_path, R_OK) == 0
            && packagetarget->checksum
            && packagetarget->checksum_type != LR_CHECKSUM_UNKNOWN)
        {
            /* If the file exists and checksum is ok, then is pointless to
             * download the file again.
             * Moreover, if the resume is enabled and the file is already
             * completely downloaded, then the download is going to fail.
             */
            int fd_r = open(packagetarget->local_path, O_RDONLY);
            if (fd_r != -1) {
                gboolean matches;
                ret = lr_checksum_fd_cmp(packagetarget->checksum_type,
                                         fd_r,
                                         packagetarget->checksum,
                                         0,
                                         &matches,
                                         NULL);
                close(fd_r);
                if (ret && matches) {
                    g_debug("%s: Package %s is already downloaded (checksum matches)",
                            __func__, packagetarget->local_path);
                    packagetarget->err = g_string_chunk_insert(packagetarget->chunk,
                                         "Already downloaded");
                    continue;
                }
            }
        }

        // Open fd
        int open_flags = O_CREAT|O_TRUNC|O_RDWR;

        if (packagetarget->resume)
            open_flags &= ~O_TRUNC;

        fd = open(packagetarget->local_path, open_flags, 0666);
        if (fd < 0) {
            ret = FALSE;
            g_debug("%s: Error while opening %s: %s",
                    __func__, packagetarget->local_path, strerror(errno));
            g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR, LRE_IO,
                        "Cannot open %s: %s",
                        packagetarget->local_path, strerror(errno));
            goto cleanup;
        }

        downloadtarget = lr_downloadtarget_new(packagetarget->relative_url,
                                               packagetarget->base_url,
                                               fd,
                                               packagetarget->checksum_type,
                                               packagetarget->checksum,
                                               packagetarget->resume,
                                               packagetarget->progresscb,
                                               packagetarget->cbdata,
                                               packagetarget);

        downloadtargets = g_slist_append(downloadtargets, downloadtarget);
    }

    // Start downloading
    ret = lr_download(handle, downloadtargets, failfast, err);

cleanup:

    // Copy download statuses from downloadtargets to targets
    for (GSList *elem = downloadtargets; elem; elem = g_slist_next(elem)) {
        LrDownloadTarget *downloadtarget = elem->data;
        LrPackageTarget *packagetarget = downloadtarget->userdata;

        if (downloadtarget->err)
            packagetarget->err = g_string_chunk_insert(packagetarget->chunk,
                                                       downloadtarget->err);
    }

    // Free downloadtargets list
    g_slist_free_full(downloadtargets, (GDestroyNotify)lr_downloadtarget_free);

    // Restore original signal handler
    if (handle->interruptible) {
        g_debug("%s: Restoring an old SIGINT handler", __func__);
        sigaction(SIGINT, &old_sigact, NULL);
        if (lr_interrupt) {
            if (err && *err != NULL)
                g_clear_error(err);
            g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR, LRE_INTERRUPTED,
                        "Insterupted by a SIGINT signal");
            return FALSE;
        }
    }

    return ret;
}
