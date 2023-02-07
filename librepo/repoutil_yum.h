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

#ifndef __LR_REPOUTIL_YUM_H__
#define __LR_REPOUTIL_YUM_H__

#include <glib.h>

#include <librepo/repomd.h>

G_BEGIN_DECLS

/** \defgroup   repoutil_yum      Yum repo high level function
 *  \addtogroup repoutil_yum
 *  @{
 */

/** Check checksum of selected repository.
 * @param path          Path to directory containing "repodata" subdir.
 * @param err           GError **
 * @return              TRUE is everything is ok, FALSE if err is set.
 */
gboolean
lr_repoutil_yum_check_repo(const char *path, GError **err);

/** Parse repomd.xml file.
 * @param path          Path to repository or to the repomd file
 * @param repomd        Empty repomd object
 * @param err           GError **
 * @return              TRUE is everything is ok, FALSE if err is set.
 */
gboolean
lr_repoutil_yum_parse_repomd(const char *path,
                             LrYumRepoMd *repomd,
                             GError **err);

/** Return age of the repomd.xml (based on mtime of the file
 * and the current time)
 * @param result    Result object
 * @return          Age of the file (number of seconds since last modification)
 */
double lr_yum_repomd_get_age(LrResult *result);

/** @} */

G_END_DECLS

#endif
