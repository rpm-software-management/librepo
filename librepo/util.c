#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string.h>
#include <unistd.h>

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
