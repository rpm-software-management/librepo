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

#include <glib.h>
#include <stdlib.h>
#include <stdarg.h>

G_BEGIN_DECLS

/** \defgroup   util    Utility functions and macros
 *  \addtogroup util
 *  @{
 */

/** Macro for unused function params (removes compiler warnings).
 * @param x     Unused parameter.
 */
#define LR_UNUSED(x) (void)(x)

/** Print "Out of memory" message to stderr and abort program execution.
 * This function is used when malloc call fails.
 */
void lr_out_of_memory();

/** Allocate len bytes of memory.
 * @param len           Number of bytes to be allocated.
 * @return              Pointer to allocated memory.
 */
void *lr_malloc(size_t len);

/** Allocate len bytes of memory. The allocated memory is set to zero.
 * @param len           Number of bytes to be allocated.
 * @return              Pointer to allocated memory.
 */
void *lr_malloc0(size_t len);

/** Change size of block memory pointed by ptr to the new len.
 * @param ptr           Pointer to block of memory or NULL.
 * @param len           New len of the block.
 * @return              New pointer to the reallocated memory.
 */
void *lr_realloc(void *ptr, size_t len);

/** Free the memory block.
 * @param mem           Pointer to block of memory.
 */
void lr_free(void *mem);

/** Create temporary librepo file in /tmp directory.
 * @return              File descriptor.
 */
int lr_gettmpfile();

/** Create temporary directory in /tmp directory.
 * @return              Path to directory.
 */
char *lr_gettmpdir();

/** Concatenate all of given part of path.
 * If last chunk is "" then separator will be appended to the result.
 * @param str           First part of the path.
 * @param ...           NULL terminated list of strings.
 * @return              Concatenated path.
 */
char *lr_pathconcat(const char *str, ...);

/** Recursively remove directory.
 * @param path          Path to the directory.
 * @return              0 on succes, -1 on error.
 */
int lr_remove_dir(const char *path);

/** Copy content from source file descriptor to the dest file descriptor.
 * @param source        Source opened file descriptor
 * @param dest          Destination openede file descriptor
 * @return              0 on succes, -1 on error
 */
int lr_copy_content(int source, int dest);

/** If protocol is specified ("http://foo") return copy of path.
 * If path is absolute ("/foo/bar/") return path with "file://" prefix.
 * If path is relative ("bar/") return absolute path with "file://" prefix.
 * @param               path
 * @return              url with protocol
 */
char *lr_prepend_url_protocol(const char *path);

/** Same as g_string_chunk_insert, but allows NULL as string.
 * If the string is NULL, then returns NULL and do nothing.
 * @param chunk         String chunk
 * @param string        String or NULL
 * @return              a pointer to the copy of string within the chunk
 */
gchar *
lr_string_chunk_insert(GStringChunk *chunk, const gchar *string);

/** @} */

G_END_DECLS

#endif
