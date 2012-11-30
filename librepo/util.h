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

#ifndef LR_UTIL_H
#define LR_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#define LR_UNUSED(x) (void)(x)

void lr_out_of_memory();
void *lr_malloc(size_t len);
void *lr_malloc0(size_t len);
void *lr_realloc(void *ptr, size_t len);
void lr_free(void *mem);
char *lr_strdup(const char *str);
char *lr_strconcat(const char *str, ...);
int lr_gettmpfile();
int lr_ends_with(const char *str, const char *suffix);
char *lr_pathconcat(const char *str, ...);

#ifdef __cplusplus
}
#endif

#endif
