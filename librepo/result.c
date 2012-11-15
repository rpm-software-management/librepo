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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "setup.h"
#include "librepo.h"
#include "util.h"
#include "result.h"
#include "result_internal.h"
#include "yum.h"
#include "repomd.h"

lr_Result
lr_result_init()
{
    return lr_malloc0(sizeof(struct _lr_Result));
}

void
lr_result_clear(lr_Result result)
{
    if (!result)
        return;
    lr_yum_repomd_free(result->yum_repomd);
    lr_yum_repo_free(result->yum_repo);
    memset(result, 0, sizeof(struct _lr_Result));
}

void
lr_result_free(lr_Result result)
{
    if (!result)
        return;
    lr_result_clear(result);
    lr_free(result);
}

int
lr_result_getinfo(lr_Result result, lr_ResultInfoOption option, ...)
{
    int rc = LRE_OK;
    va_list arg;

    if (!result)
        return LRE_BADFUNCARG;

    va_start(arg, option);

    switch (option) {
    case LRR_YUM_REPO: {
        lr_YumRepo *repo;
        repo = va_arg(arg, lr_YumRepo *);
        *repo = result->yum_repo;
        break;
    }

    case LRR_YUM_REPOMD: {
        lr_YumRepoMd *repomd;
        repomd = va_arg(arg, lr_YumRepoMd *);
        *repomd = result->yum_repomd;
        break;
    }

    default:
        rc = LRE_UNKNOWNOPT;
        break;
    }

    va_end(arg);
    return rc;
}
