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
#include "fastestmirror_internal.h"

/* Do NOT use resume on successfully downloaded files - download will fail */

LrPackageTarget *
lr_packagetarget_new(LrHandle *handle,
                     const char *relative_url,
                     const char *dest,
                     LrChecksumType checksum_type,
                     const char *checksum,
                     gint64 expectedsize,
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

    target->handle = handle;
    target->relative_url = lr_string_chunk_insert(target->chunk, relative_url);
    target->dest = lr_string_chunk_insert(target->chunk, dest);
    target->checksum_type = checksum_type;
    target->checksum = lr_string_chunk_insert(target->chunk, checksum);
    target->expectedsize = expectedsize;
    target->base_url = lr_string_chunk_insert(target->chunk, base_url);
    target->resume = resume;
    target->progresscb = progresscb;
    target->cbdata = cbdata;

    return target;
}

LrPackageTarget *
lr_packagetarget_new_v2(LrHandle *handle,
                        const char *relative_url,
                        const char *dest,
                        LrChecksumType checksum_type,
                        const char *checksum,
                        gint64 expectedsize,
                        const char *base_url,
                        gboolean resume,
                        LrProgressCb progresscb,
                        void *cbdata,
                        LrEndCb endcb,
                        LrMirrorFailureCb mirrorfailurecb,
                        GError **err)
{
    LrPackageTarget *target;

    target = lr_packagetarget_new(handle,
                                  relative_url,
                                  dest,
                                  checksum_type,
                                  checksum,
                                  expectedsize,
                                  base_url,
                                  resume,
                                  progresscb,
                                  cbdata,
                                  err);

    if (!target)
        return NULL;

    target->endcb = endcb;
    target->mirrorfailurecb = mirrorfailurecb;

    return target;
}

LrPackageTarget *
lr_packagetarget_new_v3(LrHandle *handle,
                        const char *relative_url,
                        const char *dest,
                        LrChecksumType checksum_type,
                        const char *checksum,
                        gint64 expectedsize,
                        const char *base_url,
                        gboolean resume,
                        LrProgressCb progresscb,
                        void *cbdata,
                        LrEndCb endcb,
                        LrMirrorFailureCb mirrorfailurecb,
                        gint64 byterangestart,
                        gint64 byterangeend,
                        GError **err)
{
    LrPackageTarget *target;

    target = lr_packagetarget_new_v2(handle,
                                     relative_url,
                                     dest,
                                     checksum_type,
                                     checksum,
                                     expectedsize,
                                     base_url,
                                     resume,
                                     progresscb,
                                     cbdata,
                                     endcb,
                                     mirrorfailurecb,
                                     err);

    if (!target)
        return NULL;

    target->byterangestart = byterangestart;
    target->byterangeend = byterangeend;

    return target;
}

void
lr_packagetarget_reset(LrPackageTarget *target)
{
    target->local_path = NULL;
    target->err = NULL;
}

void
lr_packagetarget_free(LrPackageTarget *target)
{
    if (!target)
        return;
    g_string_chunk_free(target->chunk);
    g_free(target);
}

gboolean
lr_download_packages(GSList *targets,
                     LrPackageDownloadFlag flags,
                     GError **err)
{
    gboolean ret;
    gboolean failfast = flags & LR_PACKAGEDOWNLOAD_FAILFAST;
    struct sigaction old_sigact;
    GSList *downloadtargets = NULL;
    gboolean interruptible = FALSE;

    assert(!err || *err == NULL);

    if (!targets)
        return TRUE;

    // Check targets
    for (GSList *elem = targets; elem; elem = g_slist_next(elem)) {
        LrPackageTarget *packagetarget = elem->data;

        if (!packagetarget->handle) {
            continue;
            /*
            g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR, LRE_BADFUNCARG,
                        "Package target %s doesn't have specified a handle",
                        packagetarget->relative_url);
            return FALSE;
            */
        }

        if (packagetarget->handle->interruptible)
            interruptible = TRUE;

        // Check repotype
        // Note: Checked because lr_handle_prepare_internal_mirrorlist
        // support only LR_YUMREPO yet
        if (packagetarget->handle->repotype != LR_YUMREPO) {
            g_debug("%s: Bad repo type", __func__);
            g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR, LRE_BADFUNCARG,
                        "Bad repo type");
            return FALSE;
        }
    }

    // Setup sighandler
    if (interruptible) {
        struct sigaction sigact;
        g_debug("%s: Using own SIGINT handler", __func__);
        memset(&sigact, 0, sizeof(old_sigact));
        memset(&sigact, 0, sizeof(sigact));
        sigemptyset(&sigact.sa_mask);
        sigact.sa_handler = lr_sigint_handler;
        sigaddset(&sigact.sa_mask, SIGINT);
        sigact.sa_flags = SA_RESTART;
        if (sigaction(SIGINT, &sigact, &old_sigact) == -1) {
            g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR, LRE_SIGACTION,
                        "Cannot set Librepo SIGINT handler");
            return FALSE;
        }
    }

    // List of handles for fastest mirror resolving
    GSList *fmr_handles = NULL;

    // Prepare targets
    for (GSList *elem = targets; elem; elem = g_slist_next(elem)) {
        gchar *local_path;
        LrPackageTarget *packagetarget = elem->data;
        LrDownloadTarget *downloadtarget;
        gint64 realsize = -1;
        gboolean doresume = packagetarget->resume;

        // Reset output attributes of the handle
        lr_packagetarget_reset(packagetarget);

        // Prepare destination filename
        if (packagetarget->dest) {
            if (g_file_test(packagetarget->dest, G_FILE_TEST_IS_DIR)) {
                // Dir specified
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

        // Check expected size and real size if the file exists
        if (doresume
            && g_access(packagetarget->local_path, R_OK) == 0
            && packagetarget->expectedsize > 0)
        {
            struct stat buf;
            if (stat(packagetarget->local_path, &buf)) {
                g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR, LRE_IO,
                        "Cannot stat %s: %s", packagetarget->local_path,
                        g_strerror(errno));
                return FALSE;
            }

            realsize = buf.st_size;

            if (packagetarget->expectedsize < realsize)
                // Existing file is bigger then the one that is expected,
                // disable resuming
                doresume = FALSE;
        }

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
                                         1,
                                         &matches,
                                         NULL);
                close(fd_r);
                if (ret && matches) {
                    // Checksum calculation was ok and checksum matches
                    g_debug("%s: Package %s is already downloaded (checksum matches)",
                            __func__, packagetarget->local_path);

                    packagetarget->err = g_string_chunk_insert(
                                                packagetarget->chunk,
                                                "Already downloaded");

                    // Call end callback
                    LrEndCb end_cb = packagetarget->endcb;
                    if (end_cb)
                        end_cb(packagetarget->cbdata,
                               LR_TRANSFER_ALREADYEXISTS,
                               "Already downloaded");

                    continue;
                } else if (ret) {
                    // Checksum calculation was ok but checksum doesn't match
                    if (realsize != -1 && realsize == packagetarget->expectedsize)
                        // File size is the same as the expected one
                        // Don't try to resume
                        doresume = FALSE;
                }
            }
        }

        if (doresume && realsize != -1 && realsize == packagetarget->expectedsize) {
            // File's size matches the expected one, the resume is enabled and
            // no checksum is known => expect that the file is
            // the one the user wants
            g_debug("%s: Package %s is already downloaded (size matches)",
                    __func__, packagetarget->local_path);

            packagetarget->err = g_string_chunk_insert(
                                        packagetarget->chunk,
                                        "Already downloaded");

            // Call end callback
            LrEndCb end_cb = packagetarget->endcb;
            if (end_cb)
                end_cb(packagetarget->cbdata,
                       LR_TRANSFER_ALREADYEXISTS,
                       "Already downloaded");

            continue;
        }

        if (packagetarget->handle) {
            ret = lr_handle_prepare_internal_mirrorlist(packagetarget->handle,
                                                        FALSE,
                                                        err);
            if (!ret)
                goto cleanup;

            if (packagetarget->handle->fastestmirror) {
                if (!g_slist_find(fmr_handles, packagetarget->handle))
                    fmr_handles = g_slist_prepend(fmr_handles,
                                                  packagetarget->handle);
            }
        }

        GSList *checksums = NULL;
        LrDownloadTargetChecksum *checksum;
        checksum = lr_downloadtargetchecksum_new(packagetarget->checksum_type,
                                                 packagetarget->checksum);
        checksums = g_slist_prepend(checksums, checksum);

        downloadtarget = lr_downloadtarget_new(packagetarget->handle,
                                               packagetarget->relative_url,
                                               packagetarget->base_url,
                                               -1,
                                               packagetarget->local_path,
                                               checksums,
                                               packagetarget->expectedsize,
                                               doresume,
                                               packagetarget->progresscb,
                                               packagetarget->cbdata,
                                               packagetarget->endcb,
                                               packagetarget->mirrorfailurecb,
                                               packagetarget,
                                               packagetarget->byterangestart,
                                               packagetarget->byterangeend,
                                               FALSE);

        downloadtargets = g_slist_append(downloadtargets, downloadtarget);
    }

    // Do Fastest Mirror resolving for all handles in one shot
    if (fmr_handles) {
        fmr_handles = g_slist_reverse(fmr_handles);
        ret = lr_fastestmirror_sort_internalmirrorlists(fmr_handles, err);
        g_slist_free(fmr_handles);

        if (!ret) {
            return FALSE;
        }
    }

    // Start downloading
    ret = lr_download(downloadtargets, failfast, err);

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
    if (interruptible) {
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

gboolean
lr_download_package(LrHandle *handle,
                    const char *relative_url,
                    const char *dest,
                    LrChecksumType checksum_type,
                    const char *checksum,
                    gint64 expectedsize,
                    const char *base_url,
                    gboolean resume,
                    GError **err)
{
    LrPackageTarget *target;

    assert(handle);
    assert(!err || *err == NULL);

    // XXX: Maybe remove in future
    if (!dest)
        dest = handle->destdir;

    // XXX: Maybe remove usage of handle callback in future

    target = lr_packagetarget_new(handle, relative_url, dest, checksum_type,
                                  checksum, expectedsize, base_url, resume,
                                  handle->user_cb, handle->user_data, err);
    if (!target)
        return FALSE;

    GSList *targets = NULL;
    targets = g_slist_append(targets, target);

    gboolean ret = lr_download_packages(targets,
                                        LR_PACKAGEDOWNLOAD_FAILFAST,
                                        err);

    g_slist_free_full(targets, (GDestroyNotify)lr_packagetarget_free);

    return ret;
}


gboolean
lr_check_packages(GSList *targets,
                  LrPackageCheckFlag flags,
                  GError **err)
{
    gboolean ret = TRUE;
    gboolean failfast = flags & LR_PACKAGECHECK_FAILFAST;
    struct sigaction old_sigact;
    gboolean interruptible = FALSE;

    assert(!err || *err == NULL);

    if (!targets)
        return TRUE;

    // Check targets
    for (GSList *elem = targets; elem; elem = g_slist_next(elem)) {
        LrPackageTarget *packagetarget = elem->data;

        if (packagetarget->handle->interruptible)
            interruptible = TRUE;

        if (!packagetarget->checksum
                || packagetarget->checksum_type == LR_CHECKSUM_UNKNOWN)
        {
            g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR, LRE_BADOPTARG,
                        "Target %s doesn't have specified "
                         "checksum value or checksum type!",
                         packagetarget->relative_url);
            return FALSE;
        }
    }

    // Setup sighandler
    if (interruptible) {
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

    for (GSList *elem = targets; elem; elem = g_slist_next(elem)) {
        gchar *local_path;
        LrPackageTarget *packagetarget = elem->data;

        // Prepare destination filename
        if (packagetarget->dest) {
            if (g_file_test(packagetarget->dest, G_FILE_TEST_IS_DIR)) {
                // Dir specified
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

        if (g_access(packagetarget->local_path, R_OK) == 0) {
            // If the file exists check its checksum
            int fd_r = open(packagetarget->local_path, O_RDONLY);
            if (fd_r != -1) {
                // File was successfully opened
                gboolean matches;
                ret = lr_checksum_fd_cmp(packagetarget->checksum_type,
                                         fd_r,
                                         packagetarget->checksum,
                                         1,
                                         &matches,
                                         NULL);
                close(fd_r);
                if (ret && matches) {
                    // Checksum is ok
                    packagetarget->err = NULL;
                    g_debug("%s: Package %s is already downloaded (checksum matches)",
                            __func__, packagetarget->local_path);
                } else {
                    // Checksum doesn't match or checksuming error
                    packagetarget->err = g_string_chunk_insert(
                                                packagetarget->chunk,
                                                "Checksum of doesn't match");
                    if (failfast) {
                        ret = FALSE;
                        g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR,
                                    LRE_BADCHECKSUM,
                                    "File with nonmatching checksum found");
                        break;
                    }
                }
            } else {
                // Cannot open the file
                packagetarget->err = g_string_chunk_insert(packagetarget->chunk,
                                       "Cannot be opened");
                if (failfast) {
                    ret = FALSE;
                    g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR, LRE_IO,
                                "Cannot open %s", packagetarget->local_path);
                    break;
                }
            }
        } else {
            // File doesn't exists
            packagetarget->err = g_string_chunk_insert(packagetarget->chunk,
                                       "Doesn't exist");
            if (failfast) {
                ret = FALSE;
                g_set_error(err, LR_PACKAGE_DOWNLOADER_ERROR, LRE_IO,
                            "File %s doesn't exists", packagetarget->local_path);
                break;
            }
        }
    }

    // Restore original signal handler
    if (interruptible) {
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
