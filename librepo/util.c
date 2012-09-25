#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include "util.h"

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

int
lr_ends_with(const char *str, const char *suffix)
{
    int str_len;
    int suffix_len;

    assert(str);
    assert(suffix);

    str_len = strlen(str);
    suffix_len = strlen(suffix);

    if (str_len < suffix_len)
        return 0;

    return strcmp(str + str_len - suffix_len, suffix) == 0;
}
