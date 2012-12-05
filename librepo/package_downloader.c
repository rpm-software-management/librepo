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
                    lr_ChecksumType checksum_type,
                    const char *checksum,
                    const char *dest,
                    int resume)
{
    int rc = LRE_OK;
    int fd;
    long offset = 0;
    char *dest_path;
    char *file_basename;
    char *dest_basename;
    int open_flags = O_CREAT|O_TRUNC|O_RDWR;

    // TODO: download resume

    assert(handle);

    if (handle->repotype == LR_YUMREPO)
        rc = lr_handle_prepare_internal_mirrorlist(handle, "repodata/repomd.xml");
    else {
        DPRINTF("%s: Bad repo type\n", __func__);
        assert(0);
    }

    if (rc != LRE_OK)
        return rc;

    file_basename = basename(relative_url);
    dest_basename = (dest) ? basename(dest) : "";

    if (dest) {
        if (!dest_basename || dest_basename[0] == '\0')
            dest_path = lr_pathconcat(dest, file_basename);
        else
            dest_path = lr_strdup(dest);
    } else
        dest_path = lr_strdup(file_basename);

    if (resume) {
        /* Enable autodetection for resume download */
        offset = -1;                /* Autodetect offset */
        open_flags &= ~O_TRUNC;     /* Do NOT truncate the dest file */
    }

    fd = open(dest_path, open_flags, 0660);
    if (fd < 0) {
        DPRINTF("%s: open(\"%s\"): %s\n", __func__, dest_path, strerror(errno));
        lr_free(dest_path);
        return LRE_IO;
    }

    DPRINTF("%s: Trying to download package: [mirror]/%s to: %s (resume: %d)\n",
                __func__, relative_url, dest_path, resume);

    rc = lr_curl_single_mirrored_download_resume(handle,
                                                relative_url,
                                                fd,
                                                checksum_type,
                                                checksum,
                                                offset);

    lr_free(dest_path);
    return rc;
}
