/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2012  Tomas Mlcoch
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 500
#include <glib.h>
#include <glib/gprintf.h>
#include <curl/curl.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdarg.h>
#include <ftw.h>

#include "util.h"
#include "version.h"
#include "metalink.h"
#include "cleanup.h"
#include "yum.h"

#define DIR_SEPARATOR   "/"
#define ENV_DEBUG       "LIBREPO_DEBUG"

#ifdef CURL_GLOBAL_ACK_EINTR
#define EINTR_SUPPORT " with CURL_GLOBAL_ACK_EINTR support"
#define CURL_GLOBAL_INIT_FLAGS  CURL_GLOBAL_ALL|CURL_GLOBAL_ACK_EINTR
#else
#define EINTR_SUPPORT ""
#define CURL_GLOBAL_INIT_FLAGS  CURL_GLOBAL_ALL
#endif

static void
lr_log_handler(G_GNUC_UNUSED const gchar *log_domain,
               G_GNUC_UNUSED GLogLevelFlags log_level,
               const gchar *message,
               G_GNUC_UNUSED gpointer user_data)
{
    g_fprintf(stderr, "%s\n", message);
}

static void
lr_init_debugging(void)
{
    if (!g_getenv(ENV_DEBUG))
        return;

    g_log_set_handler("librepo", G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
                                 | G_LOG_FLAG_RECURSION, lr_log_handler, NULL);
}

void
lr_log_librepo_summary(void)
{
    _cleanup_free_ gchar *time = NULL;
    _cleanup_date_time_unref_ GDateTime *datetime = NULL;

    g_debug("Librepo version: %d.%d.%d%s (%s)", LR_VERSION_MAJOR,
                                                LR_VERSION_MINOR,
                                                LR_VERSION_PATCH,
                                                EINTR_SUPPORT,
                                                curl_version());

    datetime = g_date_time_new_now_local();
    // Date+Time in ISO 8601 format
    time = g_date_time_format(datetime, "%Y-%m-%dT%H:%M:%S%z");
    g_debug("Current date: %s", time);
}

static gpointer
lr_init_once_cb(gpointer user_data G_GNUC_UNUSED)
{
    curl_global_init((long) CURL_GLOBAL_INIT_FLAGS);
    lr_init_debugging();
    lr_log_librepo_summary();
    return GINT_TO_POINTER(1);
}

void
lr_global_init(void)
{
    static GOnce init_once = G_ONCE_INIT;
    g_once(&init_once, lr_init_once_cb, NULL);
}


/*
void
lr_global_cleanup()
{
    curl_global_cleanup();
}
*/

void
lr_out_of_memory(void)
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

int
lr_gettmpfile(void)
{
    int fd;
    _cleanup_free_ char *template = NULL;
    template = g_build_filename(g_get_tmp_dir(), "librepo-tmp-XXXXXX", NULL);
    fd = mkstemp(template);
    if (fd < 0) {
        perror("Cannot create temporary file - mkstemp");
        exit(1);
    }
    unlink(template);
    return fd;
}

char *
lr_gettmpdir(void)
{
    char *template = g_build_filename(g_get_tmp_dir(), "librepo-tmpdir-XXXXXX", NULL);
    if (!mkdtemp(template)) {
        lr_free(template);
        return NULL;
    }
    return template;
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
    char *qmark_section;
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
        return g_strdup("");

    qmark_section = strchr(first, '?');

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
         if (is_first && qmark_section)
             end -= strlen(qmark_section);

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

    if (qmark_section) {
        strcpy(res + offset, qmark_section);
        offset += strlen(qmark_section);
    }

    assert(offset <= total_len);

    if (offset == 0) {
        lr_free(res);
        return g_strdup(first);
    }

    /* If last element was emtpy string, append separator to the end */
    if (previous_was_empty && is_first == 0) {
        strncpy(res + offset, separator, separator_len);
        offset += separator_len;
    }

    assert(offset <= total_len);

    res[offset] = '\0';

    return res;
}

int
lr_remove_dir_cb(const char *fpath,
                 G_GNUC_UNUSED const struct stat *sb,
                 G_GNUC_UNUSED int typeflag,
                 G_GNUC_UNUSED struct FTW *ftwbuf)
{
    int rv = remove(fpath);
    if (rv)
        g_debug("%s: Cannot remove: %s: %s", __func__, fpath, g_strerror(errno));
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
        if (write(dest, buf, size) == -1)
            return -1;

    return (size < 0) ? -1 : 0;
}

char *
lr_prepend_url_protocol(const char *path)
{
    if (!path)
        return NULL;

    if (strstr(path, "://"))  // Protocol was specified
        return g_strdup(path);

    if (g_str_has_prefix(path, "file:/"))
        return g_strdup(path);

    if (path[0] == '/')  // Path is absolute path
        return g_strconcat("file://", path, NULL);

    char *path_with_protocol, *resolved_path = realpath(path, NULL);
    if (!resolved_path) {
        g_debug("%s: %s - realpath: %s ", __func__, path, g_strerror(errno));
        return NULL;
    }
    path_with_protocol = g_strconcat("file://", resolved_path, NULL);
    free(resolved_path);
    return path_with_protocol;
}

gchar *
lr_string_chunk_insert(GStringChunk *chunk, const gchar *string)
{
    assert(chunk);

    if (!string)
        return NULL;

    return g_string_chunk_insert(chunk, string);
}

int
lr_xml_parser_warning_logger(LrXmlParserWarningType type G_GNUC_UNUSED,
                             char *msg,
                             void *cbdata,
                             GError **err G_GNUC_UNUSED)
{
    g_debug("WARNING: %s: %s", (char *) cbdata, msg);
    return LR_CB_RET_OK;
}

gboolean
lr_best_checksum(GSList *list, LrChecksumType *type, gchar **value)
{
    if (!list)
        return FALSE;

    assert(type);
    assert(value);

    LrChecksumType tmp_type = LR_CHECKSUM_UNKNOWN;
    gchar *tmp_value = NULL;

    for (GSList *elem = list; elem; elem = g_slist_next(elem)) {
        LrMetalinkHash *hash = elem->data;

        if (!hash->type || !hash->value)
            continue;

        LrChecksumType ltype = lr_checksum_type(hash->type);
        if (ltype != LR_CHECKSUM_UNKNOWN && ltype > tmp_type) {
            tmp_type = ltype;
            tmp_value = hash->value;
        }
    }

    if (tmp_type != LR_CHECKSUM_UNKNOWN) {
        *type = tmp_type;
        *value = tmp_value;
        return TRUE;
    }

    return FALSE;
}

gchar *
lr_url_without_path(const char *url)
{
    if (!url) return NULL;

    // Filesystem
    if (g_str_has_prefix(url, "file:///"))
        return g_strdup("file://");
    if (g_str_has_prefix(url, "file:/"))
        return g_strdup("file://");

    // Skip protocol prefix (ftp://, http://, file://, etc.)
    gchar *ptr = strstr(url, "://");
    if (ptr)
        ptr += 3;
    else
        ptr = (gchar *) url;

    // Find end of the host name
    while (*ptr != '\0' && *ptr != '/')
        ptr++;

    // Calculate length of hostname
    size_t len = ptr - url;

    gchar *host = g_strndup(url, len);
    //g_debug("%s: %s -> %s", __func__, url, host);

    return host;
}

gchar **
lr_strv_dup(gchar **array)
{
    guint length;
    gchar **copy = NULL;
    GPtrArray *ptrarray = NULL;

    if (!array)
        return array;

    length = g_strv_length(array);
    ptrarray = g_ptr_array_sized_new(length + 1);
    for (guint x=0; x < length; x++)
        g_ptr_array_add(ptrarray, g_strdup(array[x]));
    g_ptr_array_add(ptrarray, NULL);
    copy = (gchar **) ptrarray->pdata;
    g_ptr_array_free(ptrarray, FALSE);
    return copy;
}

gboolean
lr_is_local_path(const gchar *path)
{
    if (!path || !*path)
        return FALSE;

    if (strstr(path, "://") && !g_str_has_prefix(path, "file://"))
        return FALSE;

    return TRUE;
}

gboolean
lr_key_file_save_to_file(GKeyFile *keyfile,
                         const gchar *filename,
                         GError **err)
{
    _cleanup_free_ gchar *content = NULL;
    gsize length;

    content = g_key_file_to_data(keyfile, &length, err);
    if (!content)
        return FALSE;

    return g_file_set_contents(filename, content, length, err);
}

#ifdef WITH_ZCHUNK
LrChecksumType
lr_checksum_from_zck_hash(zck_hash zck_checksum_type)
{
    switch (zck_checksum_type) {
        case ZCK_HASH_SHA1:
            return LR_CHECKSUM_SHA1;
        case ZCK_HASH_SHA256:
            return LR_CHECKSUM_SHA256;
        default:
            return LR_CHECKSUM_UNKNOWN;
    }
}

zck_hash
lr_zck_hash_from_lr_checksum(LrChecksumType checksum_type)
{
    switch (checksum_type) {
        case LR_CHECKSUM_SHA1:
            return ZCK_HASH_SHA1;
        case LR_CHECKSUM_SHA256:
            return ZCK_HASH_SHA256;
        default:
            return ZCK_HASH_UNKNOWN;
    }
}

static zckCtx *
init_zck_read(const char *checksum, LrChecksumType checksum_type,
              gint64 zck_header_size, int fd, GError **err)
{
    assert(!err || *err == NULL);

    zckCtx *zck = zck_create();
    if(!zck_init_adv_read(zck, fd)) {
        g_set_error(err, LR_DOWNLOADER_ERROR, LRE_ZCK,
                    "Unable to initialize zchunk file for reading");
        return FALSE;
    }

    zck_hash ct = lr_zck_hash_from_lr_checksum(checksum_type);
    if(ct == ZCK_HASH_UNKNOWN) {
        g_set_error(err, LR_YUM_ERROR, LRE_ZCK,
                    "Zchunk doesn't support checksum type %i",
                    checksum_type);
        free(zck);
        return NULL;
    }
    if(!zck_set_ioption(zck, ZCK_VAL_HEADER_HASH_TYPE, ct)) {
        g_set_error(err, LR_YUM_ERROR, LRE_ZCK,
                    "Error setting validation checksum type");
        free(zck);
        return NULL;
    }
    if(!zck_set_ioption(zck, ZCK_VAL_HEADER_LENGTH, zck_header_size)) {
        g_set_error(err, LR_YUM_ERROR, LRE_ZCK,
                    "Error setting header size");
        free(zck);
        return NULL;
    }
    if(!zck_set_soption(zck, ZCK_VAL_HEADER_DIGEST, checksum,
                        strlen(checksum))) {
        g_set_error(err, LR_YUM_ERROR, LRE_ZCK,
                    "Unable to set validation checksum: %s",
                    checksum);
        free(zck);
        return NULL;
    }
    return zck;
}

zckCtx *
lr_zck_init_read_base(const char *checksum, LrChecksumType checksum_type,
                      gint64 zck_header_size, int fd, GError **err)
{
    assert(!err || *err == NULL);

    lseek(fd, 0, SEEK_SET);
    zckCtx *zck = init_zck_read(checksum, checksum_type, zck_header_size, fd, err);
    if(zck == NULL)
        return NULL;

    if(!zck_read_lead(zck)) {
        g_set_error(err, LR_YUM_ERROR, LRE_ZCK,
                    "Unable to read zchunk lead");
        zck_free(&zck);
        return NULL;
    }
    if(!zck_read_header(zck)) {
        g_set_error(err, LR_YUM_ERROR, LRE_ZCK,
                    "Unable to read zchunk header");
        zck_free(&zck);
        return NULL;
    }
    return zck;
}

gboolean
lr_zck_valid_header_base(const char *checksum, LrChecksumType checksum_type,
                         gint64 zck_header_size, int fd, GError **err)
{
    assert(!err || *err == NULL);

    lseek(fd, 0, SEEK_SET);
    zckCtx *zck = init_zck_read(checksum, checksum_type, zck_header_size, fd, err);
    if(zck == NULL)
        return FALSE;

    if(!zck_validate_lead(zck)) {
        g_set_error(err, LR_YUM_ERROR, LRE_ZCK,
                    "Unable to read zchunk lead");
        zck_free(&zck);
        return FALSE;
    }
    zck_free(&zck);
    return TRUE;
}

zckCtx *
lr_zck_init_read(LrDownloadTarget *target, char *filename, int fd, GError **err)
{
    zckCtx *zck = NULL;
    gboolean found = FALSE;
    for(GSList *cksum = target->checksums; cksum; cksum = g_slist_next(cksum)) {
        GError *tmp_err = NULL;
        LrDownloadTargetChecksum *ck =
            (LrDownloadTargetChecksum*)(cksum->data);
        g_debug("Checking checksum: %i: %s", ck->type, ck->value);
        zck = lr_zck_init_read_base(ck->value, ck->type, target->zck_header_size,
                                    fd, &tmp_err);
        if(zck == NULL) {
            g_debug("%s: Didn't find matching header in %s: %s", __func__,
                    filename, tmp_err->message);
            g_clear_error(&tmp_err);
            continue;
        }
        g_debug("%s: Found matching header in %s", __func__, filename);
        found = TRUE;
        break;
    }
    if(!found)
        g_set_error(err, LR_YUM_ERROR, LRE_ZCK,
                    "Zchunk header checksum didn't match expected checksum");
    return zck;
}

gboolean
lr_zck_valid_header(LrDownloadTarget *target, char *filename, int fd, GError **err)
{
    assert(!err || *err == NULL);

    for(GSList *cksum = target->checksums; cksum; cksum = g_slist_next(cksum)) {
        GError *tmp_err = NULL;
        LrDownloadTargetChecksum *ck =
            (LrDownloadTargetChecksum*)(cksum->data);
        if(lr_zck_valid_header_base(ck->value, ck->type, target->zck_header_size,
                                    fd, &tmp_err)) {
            return TRUE;
        }
        g_clear_error(&tmp_err);
    }
    g_set_error(err, LR_DOWNLOADER_ERROR, LRE_ZCK,
                "%s's zchunk header doesn't match", filename);
    return FALSE;
}
#endif /* WITH_ZCHUNK */

gboolean
lr_get_recursive_files_rec(char *path, char *extension, GSList **filelist,
                           GError **err)
{
    assert(!err || *err == NULL);
    assert(filelist);

    GDir *d = g_dir_open(path, 0, err);
    if(d == NULL)
        return FALSE;

    const char *file = NULL;
    while((file = g_dir_read_name(d))) {
        GError *tmp_err = NULL;

        char *fullpath = g_build_path("/", path, file, NULL);
        if(g_file_test(fullpath, G_FILE_TEST_IS_DIR)) {
            lr_get_recursive_files_rec(fullpath, extension, filelist, &tmp_err);
            if(tmp_err) {
                g_debug("%s: Unable to read directory %s: %s\n", __func__,
                        fullpath, tmp_err->message);
                g_clear_error(&tmp_err);
            }
            g_free(fullpath);
        } else if(g_file_test(fullpath, G_FILE_TEST_IS_REGULAR) &&
                  g_str_has_suffix(fullpath, extension)) {
            *filelist = g_slist_prepend(*filelist, fullpath);
        } else {
            g_free(fullpath);
        }
    }
    g_dir_close(d);
    return TRUE;
}

GSList *
lr_get_recursive_files(char *path, char *extension, GError **err)
{
    GSList *filelist = NULL;
    assert(!err || *err == NULL);

    if(!lr_get_recursive_files_rec(path, extension, &filelist, err)) {
        g_slist_free_full(filelist, free);
        return NULL;
    }
    return filelist;
}
