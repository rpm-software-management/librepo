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

#ifndef __LR_YUMREPO_H__
#define __LR_YUMREPO_H__

#include <glib.h>

#include "rcodes.h"
#include "result.h"

G_BEGIN_DECLS

/** \defgroup   yum       Yum repo manipulation
 *  \addtogroup yum
 *  @{
 */

/** Path to single metadata file from repomd.xml. */
typedef struct {
    char *type;  /*!< Type of record (e.g. "primary") */
    char *path;  /*!< Path to the file (e.g. foo/bar/repodata/primary.xml) */
} LrYumRepoPath;

/** Yum repository */
typedef struct {
    GSList *paths;      /*!< Paths to repo files. List of ::LrYumRepoPath*s */
    char *repomd;       /*!< Path to repomd.xml */
    char *url;          /*!< URL from where repo was downloaded */
    char *destdir;      /*!< Local path to the repo */
    char *signature;    /*!< Path to signature if available and
                             signature was downloaded (GPG check
                             was enabled during repo downloading) */
    char *mirrorlist;   /*!< Mirrolist filename */
    char *metalink;     /*!< Metalink filename */
} LrYumRepo;

/** Allocate new yum repo object.
 * @return              New yum repo object.
 */
LrYumRepo *
lr_yum_repo_init();

/** Free yum repo - free its item and the repo itself.
 * @param repo          Yum repo object.
 */
void
lr_yum_repo_free(LrYumRepo *repo);

/** Retruns path for the file from repository.
 * @param repo          Yum repo object.
 * @param type          Type of path. E.g. "primary", "filelists", ...
 * @return              Path or NULL.
 */
const char *
lr_yum_repo_path(LrYumRepo *repo, const char *type);

/** @} */

G_END_DECLS

#endif
