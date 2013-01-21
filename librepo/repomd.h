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

#ifndef LR_REPOMD_H
#define LR_REPOMD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

/** \defgroup repomd        Repomd parser
 */

/** \ingroup repomd
 * Create new empty repomd object.
 * @return              New repomd object.
 */
lr_YumRepoMd lr_yum_repomd_init();

/** \ingroup repomd
 * Free all repomd content.
 * @param repomd        Repomd object.
 */
void lr_yum_repomd_clear(lr_YumRepoMd repomd);

/** \ingroup repomd
 * Free repomd content and repomd object itself.
 * @param repomd        Repomd object.
 */
void lr_yum_repomd_free(lr_YumRepoMd repomd);

/** \ingroup repomd
 * Parse repomd.xml file.
 * @param repomd        Empty repomd object.
 * @param fd            File descriptor.
 * @return              Librepo return code ::lr_Rc.
 */
int lr_yum_repomd_parse_file(lr_YumRepoMd repomd, int fd);

/** \ingroup repomd
 * Get repomd record from the repomd object.
 * @param repomd        Repomd record.
 * @param type          Type of record e.g. "primary", "filelists", ...
 * @return              Record of desired type or NULL.
 */
lr_YumRepoMdRecord lr_yum_repomd_get_record(lr_YumRepoMd repomd,
                                            const char *type);

#ifdef __cplusplus
}
#endif

#endif
