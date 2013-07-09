/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2013  Tomas Mlcoch
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

#include <glib.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "downloadtarget.h"

lr_DownloadTarget *
lr_downloadtarget_new(const char *path,
                      const char *baseurl,
                      int fd,
                      lr_ChecksumType checksumtype,
                      const char *checksum,
                      int resume)
{
    lr_DownloadTarget *target;

    assert(path);
    assert(fd > 0);

    target = lr_malloc0(sizeof(*target));

    target->chunk        = g_string_chunk_new(0);
    target->path         = g_string_chunk_insert(target->chunk, path);
    target->baseurl      = lr_string_chunk_insert(target->chunk, baseurl);
    target->fd           = fd;
    target->checksumtype = checksumtype;
    target->checksum     = lr_string_chunk_insert(target->chunk, checksum);
    target->resume       = resume;

    return target;
}

void
lr_downloadtarget_free(lr_DownloadTarget *target)
{
    if (!target)
        return;

    g_string_chunk_free(target->chunk);
    lr_free(target);
}
