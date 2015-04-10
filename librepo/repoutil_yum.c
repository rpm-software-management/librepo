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

#define _XOPEN_SOURCE 600
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "rcodes.h"
#include "util.h"
#include "repomd.h"
#include "yum.h"
#include "handle.h"
#include "result.h"
#include "result_internal.h"

gboolean
lr_repoutil_yum_check_repo(const char *path, GError **err)
{
    gboolean ret;
    LrHandle *h;
    LrResult *result;
    const char *urls[] = { path, NULL };

    assert(path);
    assert(!err || *err == NULL);

    h = lr_handle_init();

    if (!lr_handle_setopt(h, err, LRO_REPOTYPE, LR_YUMREPO)) {
        lr_handle_free(h);
        return FALSE;
    }

    if (!lr_handle_setopt(h, err, LRO_URLS, urls)) {
        lr_handle_free(h);
        return FALSE;
    }

    if (!lr_handle_setopt(h, err, LRO_CHECKSUM, 1)) {
        lr_handle_free(h);
        return FALSE;
    }

    if (!lr_handle_setopt(h, err, LRO_LOCAL, 1)) {
        lr_handle_free(h);
        return FALSE;
    }

    result = lr_result_init();
    ret = lr_handle_perform(h, result, err);
    lr_result_free(result);
    lr_handle_free(h);

    return ret;
}

gboolean
lr_repoutil_yum_parse_repomd(const char *in_path,
                             LrYumRepoMd *repomd,
                             GError **err)
{
    int fd;
    gboolean ret;
    struct stat st;
    char *path;

    assert(in_path);
    assert(!err || *err == NULL);

    if (stat(in_path, &st) != 0) {
        g_set_error(err, LR_REPOUTIL_YUM_ERROR, LRE_IO,
                    "stat(%s,) error: %s", in_path, g_strerror(errno));
        return FALSE;
    }

    if (st.st_mode & S_IFDIR)
        path = lr_pathconcat(in_path, "repodata/repomd.xml", NULL);
    else
        path = g_strdup(in_path);

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        g_set_error(err, LR_REPOUTIL_YUM_ERROR, LRE_IO,
                    "open(%s, O_RDONLY) error: %s", path, g_strerror(errno));
        lr_free(path);
        return FALSE;
    }

    lr_free(path);

    ret = lr_yum_repomd_parse_file(repomd, fd, NULL, NULL, err);
    close(fd);

    return ret;
}

double
lr_yum_repomd_get_age(LrResult *result)
{
    assert(result);

    if (!result->yum_repo || !result->yum_repo->repomd)
        return 0.0;

    int rc;
    struct stat st;

    rc = stat(result->yum_repo->repomd, &st);
    if (rc != 0)
        return 0.0;

   return difftime(time(NULL), st.st_mtime);
}
