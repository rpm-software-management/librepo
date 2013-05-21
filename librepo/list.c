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

#include "setup.h"
#include "list.h"
#include "util.h"

lr_List *
lr_list_new()
{
    return lr_malloc(sizeof(struct _lr_List));
}

void
lr_list_free(lr_List *list)
{
    for (lr_List *elem = lr_list_first(list); elem;) {
        lr_List *next = lr_list_next(elem);
        lr_free(elem);
        elem = next;
    }
}

void
lr_list_free_full(lr_List *list, lr_DestroyFunc free_func)
{
    for (lr_List *elem = lr_list_first(list); elem;) {
        lr_List *next = lr_list_next(elem);
        free_func(elem->data);
        lr_free(elem);
        elem = next;
    }
}

lr_List *
lr_list_first(lr_List *list)
{
    if (!list) return NULL;
    for(; list->prev; list = lr_list_prev(list));
    return list;
}

lr_List *
lr_list_last(lr_List *list)
{
    if (!list) return NULL;
    for (; list->next; list = lr_list_next(list)) ;
    return list;
}

lr_List *
lr_list_append(lr_List *list, void *data)
{
    lr_List *new;

    new = lr_list_new();
    new->next = NULL;
    new->data = data;

    if (list) {
        lr_List *last;
        last = lr_list_last(list);
        last->next = new;
        new->prev = last;
        return list;
    } else {
        new->prev = NULL;
        return new;
    }
}

lr_List *
lr_list_prepend(lr_List *list, void *data)
{
    lr_List *new;

    new = lr_list_new();
    new->prev = NULL;
    new->next = list;
    new->data = data;
    if (list)
        list->prev = new;
    return new;
}

size_t
lr_list_length(lr_List *list)
{
    size_t len = 0;
    for (list = lr_list_first(list); list; ++len, list = lr_list_next(list)) ;
    return len;
}

lr_List *
lr_list_remove(lr_List *list, void *data)
{
    if (!list) return NULL;  // Empty list

    if (list->data == data) {
        // First element will be removed
        lr_List *new_begin = lr_list_next(list);
        if (new_begin)
            new_begin->prev = NULL;
        lr_free(list);
        return new_begin;
    }

    // Find first suitable item and remove it
    for (lr_List *elem = lr_list_next(list); elem; elem = lr_list_next(elem)) {
        if (data != elem->data)
            continue;
        elem->prev->next = elem->next;
        if (elem->next)
            elem->next->prev = elem->prev;
        lr_free(elem);
        break;
    }

    return list;
}
