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

#define _XOPEN_SOURCE 600
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>

#include "rcodes.h"
#include "util.h"
#include "repomd.h"
#include "yum.h"
#include "handle.h"
#include "result.h"

int
lr_repoutil_yum_check_repo(const char *path)
{
    int rc;
    lr_Handle h;
    lr_Result result;

    h = lr_handle_init();
    result = lr_result_init();
    if (!h)
        return LRE_UNKNOWNERROR;

    if ((rc = lr_handle_setopt(h, LRO_REPOTYPE, LR_YUMREPO)) != LRE_OK)
        return rc;

    if ((rc = lr_handle_setopt(h, LRO_URL, path)) != LRE_OK)
        return rc;

    if ((rc = lr_handle_setopt(h, LRO_CHECKSUM, 1)) != LRE_OK)
        return rc;

    if ((rc = lr_handle_setopt(h, LRO_LOCAL, 1)) != LRE_OK)
        return rc;

    rc = lr_handle_perform(h, result);

    lr_result_free(result);
    lr_handle_free(h);
    return rc;
}

int
lr_repoutil_yum_parse_repomd(const char *path, lr_YumRepoMd repomd)
{
    int fd, rc;
    struct stat st;

    if (stat(path, &st) != 0)
        return LRE_IO;

    if (st.st_mode & S_IFDIR) {
        char *path2 = lr_pathconcat(path, "repodata/repomd.xml", NULL);
        fd = open(path2, O_RDONLY);
        lr_free(path2);
    } else
        fd = open(path, O_RDONLY);

    if (fd < 0)
        return LRE_IO;

    rc = lr_yum_repomd_parse_file(repomd, fd);
    close(fd);
    return rc;
}
