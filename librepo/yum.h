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

#ifndef LR_YUMREPO_H
#define LR_YUMREPO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rcodes.h"
#include "types.h"

/** \defgroup yum       Yum repo manipulation
 */

/** \ingroup yum
 * Allocate new yum repo object.
 * @return              New yum repo object.
 */
lr_YumRepo lr_yum_repo_init();

/** \ingroup yum
 * Clear yum repo - free its item.
 * @param repo          Yum repo object.
 */
void lr_yum_repo_clear(lr_YumRepo repo);

/** \ingroup yum
 * Free yum repo - free its item and the repo itself.
 * @param repo          Yum repo object.
 */
void lr_yum_repo_free(lr_YumRepo repo);

/** \ingroup yum
 * Retruns path for the file from repository.
 * @param repo          Yum repo object.
 * @param type          Type of path. E.g. "primary", "filelists", ...
 * @return              Path or NULL.
 */
char *lr_yum_repo_path(lr_YumRepo repo, const char *type);

/** \ingroup yum
 * Append path to the repository object.
 * @param repo          Yum repo object.
 * @param type          Type of file. E.g. "primary", "filelists", ...
 * @param path          Path to the file.
 */
void lr_yum_repo_append(lr_YumRepo repo, const char *type, const char *path);

#ifdef __cplusplus
}
#endif

#endif
