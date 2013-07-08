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
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "setup.h"
#include "types.h"
#include "util.h"
#include "curl.h"
#include "package_downloader.h"
#include "handle_internal.h"

/* Do NOT use resume on successfully downloaded files - download will fail */

int
lr_download_package(lr_Handle handle,
                    const char *relative_url,
                    const char *dest,
                    lr_ChecksumType checksum_type,
                    const char *checksum,
                    const char *base_url,
                    int resume)
{
    int rc = LRE_OK;
    int fd;
    long offset = 0;
    char *dest_path;
    char *file_basename;
    char *dest_basename;
    int open_flags = O_CREAT|O_TRUNC|O_RDWR;
    struct sigaction old_sigact;

    assert(handle);

    if (handle->interruptible) {
        /* Setup sighandler */
        DPRINTF("%s: Using own SIGINT handler\n", __func__);
        struct sigaction sigact;
        sigact.sa_handler = lr_sigint_handler;
        sigaddset(&sigact.sa_mask, SIGINT);
        sigact.sa_flags = SA_RESTART;
        if (sigaction(SIGINT, &sigact, &old_sigact) == -1)
            return LRE_SIGACTION;
    }


    if (handle->repotype == LR_YUMREPO)
        rc = lr_handle_prepare_internal_mirrorlist(handle);
    else {
        DPRINTF("%s: Bad repo type\n", __func__);
        assert(0);
    }

    if (rc != LRE_OK)
        return rc;

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
            dest_path = lr_strdup(dest);
        }
    } else {
        /* No path specified */
        if (!handle->destdir) {
            /* No destdir in handle - use current working dir */
            dest_path = lr_strdup(file_basename);
        } else {
            /* Use dir specified in handle */
            dest_path = lr_pathconcat(handle->destdir, file_basename, NULL);
        }
    }

    if (resume) {
        if (access(dest_path, R_OK) == 0) {
            /* Check checksum of existing file (if file exists).
             * If file exists and checksum is ok, then is pointless to
             * use a resume, because resume always fails in this case. */
            int fd_r = open(dest_path, O_RDONLY);
            if (fd_r != -1) {
                int ret = lr_checksum_fd_cmp(checksum_type, fd_r, checksum, 0);
                close(fd_r);
                if (ret == 0) {
                    lr_free(dest_path);
                    return LRE_ALREADYDOWNLOADED;
                }
            }
        }

        /* Enable autodetection for resume download */
        offset = -1;                /* Autodetect offset */
        open_flags &= ~O_TRUNC;     /* Do NOT truncate the dest file */
    }

    fd = open(dest_path, open_flags, 0666);
    if (fd < 0) {
        DPRINTF("%s: open(\"%s\"): %s\n", __func__, dest_path, strerror(errno));
        lr_free(dest_path);
        return LRE_IO;
    }

    if (!base_url) {
        /* Use internal mirrorlist to download */
        DPRINTF("%s: Trying to download package: [mirror]/%s to: %s (resume: %d)\n",
                __func__, relative_url, dest_path, resume);

        rc = lr_curl_single_mirrored_download_resume(handle,
                                                     relative_url,
                                                     fd,
                                                     checksum_type,
                                                     checksum,
                                                     offset,
                                                     1);
    } else {
        /* Use base url instead of mirrorlist */
        char *full_url;

        full_url = lr_pathconcat(base_url, relative_url, NULL);
        DPRINTF("%s: Trying to download package: %s to: %s (resume: %d)\n",
                __func__, full_url, dest_path, resume);

        rc = lr_curl_single_download_resume(handle, full_url, fd, offset, 1);
        lr_free(full_url);

        /* Check checksum */
        if (rc == LRE_OK && checksum && checksum_type != LR_CHECKSUM_UNKNOWN) {
            DPRINTF("%s: Checking checksum\n", __func__);
            lseek(fd, 0, SEEK_SET);
            if (lr_checksum_fd_cmp(checksum_type, fd, checksum, 0)) {
                DPRINTF("%s: Bad checksum\n", __func__);
                rc = LRE_BADCHECKSUM;
            }
        }
    }

    if (handle->interruptible) {
        /* Restore signal handler */
        DPRINTF("%s: Restoring an old SIGINT handler\n", __func__);
        if (sigaction(SIGINT, &old_sigact, NULL) == -1)
            return LRE_SIGACTION;
    }

    lr_free(dest_path);
    return rc;
}
