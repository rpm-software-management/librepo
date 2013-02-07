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

#ifndef LR_CURLTARGETLIST_H
#define LR_CURLTARGETLIST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "checksum.h"

/**
 * Target for download via ::lr_curl_multi_download
 */
struct _lr_CurlTarget {
    char *path;      /*!< Relative path for URL
                        (URL: "http://foo.bar/stuff", path: "somestuff.xml") */
    int fd;          /*!< Opened file descriptor where data will be written */
    lr_ChecksumType checksum_type;  /*!< Checksum type */
    char *checksum;  /*!< Expected checksum value or NULL */
    int downloaded;  /*!< 1 target was downloaded successfully, 0 otherwise */
};

typedef struct _lr_CurlTarget * lr_CurlTarget;

/**
 * List of targets to download used in ::lr_curl_multi_download
 */
struct _lr_CurlTargetList {
    int size;                           /*!< Number of allocated elements */
    int used;                           /*!< Number of used elements */
    struct _lr_CurlTarget **targets;    /*!< List of targets */
};

typedef struct _lr_CurlTargetList * lr_CurlTargetList;

/**
 * Create new empty ::lr_CurlTarget.
 * @return              New allocated target.
 */
lr_CurlTarget lr_curltarget_new();

/**
 * Free a ::lr_CurlTarget element and its content.
 * Note: Each char* element of ::lr_CurlTarget structure will be freed!
 * So each such element have to be a malloced (not static) string or NULL!
 * @param target        Target to free.
 */
void lr_curltarget_free(lr_CurlTarget target);

/**
 * Create new empty curl list of targets.
 * @return              New empty list of targets.
 */
lr_CurlTargetList lr_curltargetlist_new();

/**
 * Free a ::lr_CurlTargetList and all its content.
 * Note: Keep in mind that each char* element in all ::lr_CurlTarget items
 * will be freed! So each that element have to be NULL or points to malloced
 * string.
 * @param list          List.
 */
void lr_curltargetlist_free(lr_CurlTargetList list);

/**
 * Append curl target to list of curl targets.
 * @param list          List of curl targets.
 * @param target        Curl target.
 */
void lr_curltargetlist_append(lr_CurlTargetList list, lr_CurlTarget target);

/**
 * Return number of elements of the list.
 * @param list          List of curl targets.
 * @return              Number of targets in the list.
 */
int lr_curltargetlist_len(lr_CurlTargetList list);

/**
 * Return target on the position selected by index. Items in the list
 * are indexed from 0. If index < 0 or index >= lr_curltargetlist_len(list)
 * then NULL is returned.
 * @param list          List of curl targets.
 * @param index         Index of desired curl target.
 * @return              Curl target or NULL.
 */
lr_CurlTarget lr_curltargetlist_get(lr_CurlTargetList list, int index);

#ifdef __cplusplus
}
#endif

#endif
