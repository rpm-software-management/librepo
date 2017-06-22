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

#ifndef __LR_RESULT_H__
#define __LR_RESULT_H__

#include <glib.h>

#include "types.h"

G_BEGIN_DECLS

/** \defgroup   result      Result object
 *  \addtogroup result
 *  @{
 */

/** Result object
 */
typedef struct _LrResult LrResult;

/** Result options for ::lr_result_getinfo.
 */
typedef enum {
    LRR_YUM_REPO,       /*!< (LrYumRepo *)
        Reference to ::LrYumRepo in result */

    LRR_YUM_REPOMD,     /*!< (LrYumRepoMd *)
        Reference to ::LrYumRepoMd in result */

    LRR_YUM_TIMESTAMP,  /*!< (gint64)
        The highest timestamp from repomd.xml.
        See: https://github.com/Tojaj/librepo/issues/25
        See: http://yum.baseurl.org/gitweb?p=yum.git;a=commitdiff;h=59d3d67f */

    LRR_RPMMD_REPO,      /*!< In C same as LRR_YUM_REPO */
    LRR_RPMMD_REPOMD,    /*!< In C same as LRR_YUM_REPOMD */
    LRR_RPMMD_TIMESTAMP, /*!< In C same as LRR_YUM_TIMESTAMP */
    LRR_SENTINEL,
} LrResultInfoOption;

/** Return new allocated ::LrResult object
 * @return          New allocated ::LrResult object
 */
LrResult *
lr_result_init(void);

/** Clean result object.
 * @param result    Result object.
 */
void
lr_result_clear(LrResult *result);

/** Free result object.
 * @param result    Result object.
 */
void
lr_result_free(LrResult *result);

/** Get information about downloaded/localised repository from result object.
 * @param result    Result object.
 * @param err       GError **
 * @param option    Option from ::LrResultInfoOption enum.
 * @param ...       Appropriate variable for the selected option.
 * @return          TRUE if everything is ok, false if err is set.
 */
gboolean
lr_result_getinfo(LrResult *result,
                  GError **err,
                  LrResultInfoOption option,
                  ...);

/** @} */

G_END_DECLS

#endif
