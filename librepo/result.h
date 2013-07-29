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

#ifndef LR_RESULT_H
#define LR_RESULT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

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
    LRR_YUM_REPO,       /*!< (LrYumRepo *) Reference to ::LrYumRepo in result */
    LRR_YUM_REPOMD,     /*!< (LrYumRepoMd *) Reference to ::LrYumRepoMd in result */
    LRR_SENTINEL,
} LrResultInfoOption;

/** Return new allocated ::LrResult object
 * @return          New allocated ::LrResult object
 */
LrResult *lr_result_init();

/** Clean result object.
 * @param result    Result object.
 */
void lr_result_clear(LrResult *result);

/** Free result object.
 * @param result    Result object.
 */
void lr_result_free(LrResult *result);

/** Get information about downloaded/localised repository from result object.
 * @param result    Result object.
 * @param option    Option from ::LrResultInfoOption enum.
 * @param ...       Apropriate variable for the selected option.
 * @return          Librepo return code ::LrRc.
 */
int lr_result_getinfo(LrResult *result, LrResultInfoOption option, ...);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
