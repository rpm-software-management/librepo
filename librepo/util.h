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

#include <stdlib.h>
#include <stdarg.h>

/** \defgroup   util    Utility functions and macros
 */

/** \ingroup util
 * Macro for unused function params (removes compiler warnings).
 * @param x     Unused parameter.
 */
#define LR_UNUSED(x) (void)(x)

/** \ingroup util
 * Print "Out of memory" message to stderr and abort program execution.
 * This function is used when malloc call fails.
 */
void lr_out_of_memory();

/** \ingroup util
 * Allocate len bytes of memory.
 * @param len           Number of bytes to be allocated.
 * @return              Pointer to allocated memory.
 */
void *lr_malloc(size_t len);

/** \ingroup util
 * Allocate len bytes of memory. The allocated memory is set to zero.
 * @param len           Number of bytes to be allocated.
 * @return              Pointer to allocated memory.
 */
void *lr_malloc0(size_t len);

/** \ingroup util
 * Change size of block memory pointed by ptr to the new len.
 * @param ptr           Pointer to block of memory or NULL.
 * @param len           New len of the block.
 * @return              New pointer to the reallocated memory.
 */
void *lr_realloc(void *ptr, size_t len);

/** \ingroup util
 * Free the memory block.
 * @param mem           Pointer to block of memory.
 */
void lr_free(void *mem);

/** \ingroup util
 * Return new allocated memory containing copy of the string.
 * @param str           String.
 * @return              New allocated memory with copy of the string.
 */
char *lr_strdup(const char *str);

/** \ingroup util
 * Concatenate all of given string into one long string. Variable argument
 * list must end with NULL.
 * @param str           First string param.
 * @param ...           NULL terminated list of strings.
 * @return              One long string.
 */
char *lr_strconcat(const char *str, ...);

/** \ingroup util
 * Create temporary librepo file in /tmp directory.
 * @return              File descriptor.
 */
int lr_gettmpfile();

/** \ingroup util
 * Create temporary directory in /tmp directory.
 * @return              Path to directory.
 */
char *lr_gettmpdir();

/** \ingroup util
 * Looks whether string ends with suffix.
 * @param str           String.
 * @param suffix        Suffix.
 * @returns             0 if str don't end with suffix, other if end.
 */
int lr_ends_with(const char *str, const char *suffix);

/** \ingroup util
 * Concatenate all of given part of path.
 * If last chunk is "" then separator will be appended to the result.
 * @param str           First part of the path.
 * @param ...           NULL terminated list of strings.
 * @return              Concatenated path.
 */
char *lr_pathconcat(const char *str, ...);

/** \ingroup util
 * Recursively remove directory.
 * @param path          Path to the directory.
 * @return              0 on succes, -1 on error.
 */
int lr_remove_dir(const char *path);

/** \ingroup util
 * Print to allocated string.
 * @param strp          Location for the newly allocated string.
 * @param format        A standard printf() format string.
 * @param ap            The list of arguments to insert in the output.
 * @return              The number of bytes printed
 */
int lr_vasprintf(char **strp, const char *format, va_list ap);

#ifdef __cplusplus
}
#endif

#endif
