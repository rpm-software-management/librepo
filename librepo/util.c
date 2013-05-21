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

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 500
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdarg.h>
#include <ftw.h>

#include "setup.h"
#include "util.h"

#define DIR_SEPARATOR   "/"

void
lr_out_of_memory()
{
    fprintf(stderr, "Out of memory\n");
    abort();
    exit(1);
}

void *
lr_malloc(size_t len)
{
    void *m = malloc(len);
    if (!m) lr_out_of_memory();
    return m;
}

void *
lr_malloc0(size_t len)
{
    void *m = calloc(1, len);
    if (!m) lr_out_of_memory();
    return m;
}

void *
lr_realloc(void *ptr, size_t len)
{
    void *m = realloc(ptr, len);
    if (!m && len) lr_out_of_memory();
    return m;
}

void
lr_free(void *m)
{
    if (m) free(m);
}

char *
lr_strdup(const char *str)
{
    char *new;
    if (!str)
        return NULL;
    new = strdup(str);
    if (!new) lr_out_of_memory();
    return new;
}

char *
lr_strndup(const char *str, size_t n)
{
    char *new;
    if (!str)
        return NULL;
    new = strndup(str, n);
    if (!new) lr_out_of_memory();
    return new;
}

char *
lr_strconcat(const char *str, ...)
{
    va_list arg;
    char *chunk, *res;
    size_t offset, total_len;

    if (!str)
        return NULL;

    offset = strlen(str);
    total_len = offset;

    va_start(arg, str);
    while ((chunk = va_arg(arg, char *)))
        total_len += strlen(chunk);
    va_end(arg);

    res = lr_malloc(total_len + 1);

    strcpy(res, str);
    va_start(arg, str);
    while ((chunk = va_arg(arg, char *))) {
        strcpy(res + offset, chunk);
        offset += strlen(chunk);
    }
    va_end(arg);

    return res;
}

int
lr_gettmpfile()
{
    char template[] = "/tmp/librepo-tmp-XXXXXX";
    int fd = mkstemp(template);
    if (fd < 0) {
        perror("Cannot create temporary file - mkstemp");
        exit(1);
    }
    unlink(template);
    return fd;
}

char *
lr_gettmpdir()
{
    char *template = lr_strdup("/tmp/librepo-tmpdir-XXXXXX");
    char *dir = mkdtemp(template);
    if (!dir)
        lr_free(template);
    return dir;
}

int
lr_ends_with(const char *str, const char *suffix)
{
    int str_len;
    int suffix_len;

    if (!str || !suffix)
        return 0;

    str_len = strlen(str);
    suffix_len = strlen(suffix);

    if (str_len < suffix_len)
        return 0;

    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

char *
lr_pathconcat(const char *first, ...)
{
    va_list args;
    const char *next;
    char *separator = DIR_SEPARATOR;
    char *chunk, *res = NULL;
    size_t separator_len = strlen(DIR_SEPARATOR);
    size_t total_len;  // Maximal len of result
    size_t offset = 0;
    int is_first = 1;
    int previous_was_empty = 0; // If last chunk was "" then separator will be
                                // appended to the result

    if (!first)
        return NULL;

    total_len = strlen(first);

    va_start(args, first);
    while ((chunk = va_arg(args, char *)))
        total_len += (strlen(chunk) + separator_len);
    va_end(args);

    if (total_len == 0)
        return lr_strdup("");

    res = lr_malloc(total_len + separator_len + 1);

    next = first;
    va_start(args, first);
    while (1) {
        const char *current, *start, *end;
        size_t current_len;

        if (next) {
            current = next;
            next = va_arg(args, char *);
        } else
            break;

        current_len = strlen(current);

        if (!current_len) {
            previous_was_empty = 1;
            continue;   /* Skip empty element */
        } else
            previous_was_empty = 0;

        start = current;
        end = start + current_len;

        /* Skip leading separators - except first element */
        if (separator_len && is_first == 0) {
            while (!strncmp(start, separator, separator_len))
                start += separator_len;
        }

        /* Skip trailing separators */
        if (separator_len) {
            while (start + separator_len <= end &&
                   !strncmp(end-separator_len, separator, separator_len))
                end -= separator_len;
        }

        if (start >= end) {
            /* Element is filled only by separators */
            if (is_first)
                is_first = 0;
            continue;
        }

        /* Prepend separator - except first element */
        if (is_first == 0) {
            strncpy(res + offset, separator, separator_len);
            offset += separator_len;
        } else
            is_first = 0;

        strncpy(res + offset, start, end - start);
        offset += end - start;
    }
    va_end(args);

    DEBUGASSERT(offset <= total_len);

    if (offset == 0) {
        lr_free(res);
        return lr_strdup(first);
    }

    /* If last element was emtpy string, append separator to the end */
    if (previous_was_empty && is_first == 0) {
        strncpy(res + offset, separator, separator_len);
        offset += separator_len;
    }

    DEBUGASSERT(offset <= total_len);

    res[offset] = '\0';

    return res;
}

int
lr_remove_dir_cb(const char *fpath,
                 const struct stat *sb,
                 int typeflag,
                 struct FTW *ftwbuf)
{
    LR_UNUSED(sb);
    LR_UNUSED(typeflag);
    LR_UNUSED(ftwbuf);
    int rv = remove(fpath);
    if (rv)
        DPRINTF("%s: Cannot remove: %s: %s", __func__, fpath, strerror(errno));
    return rv;
}

int
lr_remove_dir(const char *path)
{
    return nftw(path, lr_remove_dir_cb, 64, FTW_DEPTH | FTW_PHYS);
}

int
lr_copy_content(int source, int dest)
{
    const int bufsize = 2048;
    char buf[bufsize];
    ssize_t size;

    lseek(source, 0, SEEK_SET);
    lseek(dest, 0, SEEK_SET);

    while ((size = read(source, buf, bufsize)) > 0)
        write(dest, buf, size);

    return (size < 0) ? -1 : 0;
}

char *
lr_prepend_url_protocol(const char *path)
{
    if (!path)
        return NULL;

    if (strstr(path, "://"))  // Protocol was specified
        return lr_strdup(path);

    if (path[0] == '/')  // Path is absolute path
        return lr_strconcat("file://", path, NULL);

    char *path_with_protocol, *resolved_path = realpath(path, NULL);
    if (!resolved_path) {
        DPRINTF("%s: %s - realpath: %s ", __func__, path, strerror(errno));
        return NULL;
    }
    path_with_protocol = lr_strconcat("file://", resolved_path, NULL);
    free(resolved_path);
    return path_with_protocol;
}

int
lr_vasprintf(char **strp, const char *format, va_list va)
{
    int size;
#ifdef HAVE_VASPRINTF
    size = vasprintf(strp, format, va);
    if (size == -1)
        lr_out_of_memory();
    return size;
#else
    va_list va2;

    va_copy(va2, va);
    size = vsnprintf(NULL, 0, format, va2);
    va_end(va2);
    *strp = lr_malloc(size + 1);
    vsnprintf(*strp, size + 1, format, va);
    (*strp)[size] = 0;
    return size;
#endif
}
