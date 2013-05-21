/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2013  Tomas Mlcoch
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

#ifndef LR_LIST_H
#define LR_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

/** \defgroup   list    Doubly linked list
 *  \addtogroup list
 *  @{
 */

typedef struct _lr_List lr_List;
struct _lr_List {
    lr_List *next;
    lr_List *prev;
    void *data;
};

typedef void (*lr_DestroyFunc) (void *data);

#define lr_list_next(list)  ((list) ? (((lr_List *)(list))->next) : NULL)
#define lr_list_prev(list)  ((list) ? (((lr_List *)(list))->prev) : NULL)
#define lr_list_data(list)  ((list) ? (((lr_List *)(list))->data) : NULL)

void lr_list_free(lr_List *list);
void lr_list_free_full(lr_List *list, lr_DestroyFunc free_func);
lr_List *lr_list_first(lr_List *list);
lr_List *lr_list_last(lr_List *list);
lr_List *lr_list_append(lr_List *list, void *data);
lr_List *lr_list_prepend(lr_List *list, void *data);
size_t lr_list_length(lr_List *list);
lr_List *lr_list_remove(lr_List *list, void *data);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
