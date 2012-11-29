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

struct _lr_CurlTarget {
    char *path;      // Path for URL (URL: "http://foo.bar/stuff", path: "somestuff.rar")
    int fd;          // File descriptor
    int downloaded;  // Was target downloaded successfully? 0 - no, 1 - yes
};

typedef struct _lr_CurlTarget * lr_CurlTarget;

struct _lr_CurlTargetList {
    int size;   // Number of allocated elements
    int used;   // Number of used elements
    struct _lr_CurlTarget **targets;
};

typedef struct _lr_CurlTargetList * lr_CurlTargetList;

lr_CurlTarget lr_curltarget_new();
void lr_curltarget_free(lr_CurlTarget);

lr_CurlTargetList lr_curltargetlist_new();
void lr_curltargetlist_free(lr_CurlTargetList);
void lr_curltargetlist_append(lr_CurlTargetList, lr_CurlTarget);
int lr_curltargetlist_len(lr_CurlTargetList);
lr_CurlTarget lr_curltargetlist_get(lr_CurlTargetList, int);

#ifdef __cplusplus
}
#endif

#endif
