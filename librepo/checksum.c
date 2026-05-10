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
#include <glib.h>
#include <glib/gprintf.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>
#include <openssl/evp.h>

#include "cleanup.h"
#include "checksum.h"
#include "rcodes.h"
#include "util.h"
#include "xattr_internal.h"

#define BUFFER_SIZE             128*1024
#define MAX_CHECKSUM_NAME_LEN   7

LrChecksumType
lr_checksum_type(const char *type)
{
    size_t len;
    char type_lower[MAX_CHECKSUM_NAME_LEN+1];

    if (!type)
        return LR_CHECKSUM_UNKNOWN;

    len = strlen(type);
    if (len > MAX_CHECKSUM_NAME_LEN)
        return LR_CHECKSUM_UNKNOWN;

    for (size_t x = 0; x <= len; x++)
        type_lower[x] = tolower(type[x]);

    if (!strncmp(type_lower, "md", 2)) {
        // MD* family
        char *md_type = type_lower + 2;
        if (!strcmp(md_type, "5"))
            return LR_CHECKSUM_MD5;
    } else if (!strncmp(type_lower, "sha", 3)) {
        // SHA* family
        char *sha_type = type_lower + 3;
        if (!strcmp(sha_type, ""))
            return LR_CHECKSUM_SHA1;
        else if (!strcmp(sha_type, "1"))
            return LR_CHECKSUM_SHA1;
        else if (!strcmp(sha_type, "224"))
            return LR_CHECKSUM_SHA224;
        else if (!strcmp(sha_type, "256"))
            return LR_CHECKSUM_SHA256;
        else if (!strcmp(sha_type, "384"))
            return LR_CHECKSUM_SHA384;
        else if (!strcmp(sha_type, "512"))
            return LR_CHECKSUM_SHA512;
    }

    return LR_CHECKSUM_UNKNOWN;
}

const char *
lr_checksum_type_to_str(LrChecksumType type)
{
    switch (type) {
    case LR_CHECKSUM_UNKNOWN:
        return "Unknown checksum";
    case LR_CHECKSUM_MD5:
        return "md5";
    case LR_CHECKSUM_SHA1:
        return "sha1";
    case LR_CHECKSUM_SHA224:
        return "sha224";
    case LR_CHECKSUM_SHA256:
        return "sha256";
    case LR_CHECKSUM_SHA384:
        return "sha384";
    case LR_CHECKSUM_SHA512:
        return "sha512";
    }
    return NULL;
}

char *
lr_checksum_fd(LrChecksumType type, int fd, GError **err)
{
    unsigned int len;
    ssize_t readed;
    char buf[BUFFER_SIZE];
    unsigned char raw_checksum[EVP_MAX_MD_SIZE];
    char *checksum;
    EVP_MD_CTX *ctx;
    const EVP_MD *ctx_type;

    assert(fd > -1);
    assert(!err || *err == NULL);

    switch (type) {
        case LR_CHECKSUM_MD5:       ctx_type = EVP_md5();    break;
        case LR_CHECKSUM_SHA1:      ctx_type = EVP_sha1();   break;
        case LR_CHECKSUM_SHA224:    ctx_type = EVP_sha224(); break;
        case LR_CHECKSUM_SHA256:    ctx_type = EVP_sha256(); break;
        case LR_CHECKSUM_SHA384:    ctx_type = EVP_sha384(); break;
        case LR_CHECKSUM_SHA512:    ctx_type = EVP_sha512(); break;
        case LR_CHECKSUM_UNKNOWN:
        default:
            g_debug("%s: Unknown checksum type", __func__);
            assert(0);
            g_set_error(err, LR_CHECKSUM_ERROR, LRE_BADFUNCARG,
                        "Unknown checksum type: %d", type);
            return NULL;
    }

    ctx = EVP_MD_CTX_create();
    if (!ctx) {
        g_set_error(err, LR_CHECKSUM_ERROR, LRE_OPENSSL,
                    "EVP_MD_CTX_create() failed");
        return NULL;
    }

    if (!EVP_DigestInit_ex(ctx, ctx_type, NULL)) {
        g_set_error(err, LR_CHECKSUM_ERROR, LRE_OPENSSL,
                    "EVP_DigestInit_ex() failed");
        EVP_MD_CTX_destroy(ctx);
        return NULL;
    }

    if (lseek(fd, 0, SEEK_SET) == -1) {
        g_set_error(err, LR_CHECKSUM_ERROR, LRE_IO,
                    "Cannot seek to the begin of the file. "
                    "lseek(%d, 0, SEEK_SET) error: %s", fd, g_strerror(errno));
        return NULL;
    }

    while ((readed = read(fd, buf, BUFFER_SIZE)) > 0)
        if (!EVP_DigestUpdate(ctx, buf, readed)) {
            g_set_error(err, LR_CHECKSUM_ERROR, LRE_OPENSSL,
                        "EVP_DigestUpdate() failed");
            return NULL;
        }

    if (readed == -1) {
        EVP_MD_CTX_destroy(ctx);
        g_set_error(err, LR_CHECKSUM_ERROR, LRE_IO,
                    "read(%d) failed: %s", fd, g_strerror(errno));
        return NULL;
    }

    if (!EVP_DigestFinal_ex(ctx, raw_checksum, &len)) {
        g_set_error(err, LR_CHECKSUM_ERROR, LRE_OPENSSL,
                    "EVP_DigestFinal_ex() failed");
        return NULL;
    }

    EVP_MD_CTX_destroy(ctx);

    checksum = lr_malloc0(sizeof(char) * (len * 2 + 1));
    for (size_t x = 0; x < len; x++)
        sprintf(checksum+(x*2), "%02x", raw_checksum[x]);

    return checksum;
}

gboolean
lr_checksum_fd_cmp(LrChecksumType type,
                   int fd,
                   const char *expected,
                   gboolean caching,
                   gboolean *matches,
                   GError **err)
{
    return lr_checksum_fd_compare(type, fd, expected, caching,
                                  matches, NULL, err);
}


gboolean
lr_checksum_fd_compare(LrChecksumType type,
                       int fd,
                       const char *expected,
                       gboolean caching,
                       gboolean *matches,
                       gchar **calculated,
                       GError **err)
{
    assert(fd >= 0);
    assert(!err || *err == NULL);

    *matches = FALSE;

    if (!expected) {
        g_set_error(err, LR_CHECKSUM_ERROR, LRE_BADFUNCARG,
                    "No expected checksum passed");
        return FALSE;
    }

    long long timestamp = -1;

    if (caching) {
        struct stat st;
        if (fstat(fd, &st) == 0) {
            timestamp = st.st_mtime;
            timestamp *= 1000000000; //convert sec timestamp to nanosec timestamp
            timestamp += st.st_mtim.tv_nsec;
        }
    }

    _cleanup_free_ gchar *timestamp_str = g_strdup_printf("%lli", timestamp);
    const char *type_str = lr_checksum_type_to_str(type);
    _cleanup_free_ gchar *checksum_key = g_strconcat(XATTR_CHKSUM_PREFIX, type_str, NULL);

    if (caching && timestamp != -1) {
        // Load cached checksum if enabled and used
        char buf[256];
        ssize_t attr_size;
        attr_size = FGETXATTR(fd, XATTR_CHKSUM_MTIME, &buf, sizeof(buf)-1);
        if (attr_size != -1) {
            buf[attr_size] = 0;
            // check that mtime stored in xattr is the same as timestamp
            if (strcmp(timestamp_str, buf) == 0) {
                g_debug("%s: Using mtime cached in xattr: [%s] %s", __func__, XATTR_CHKSUM_MTIME, buf);
                attr_size = FGETXATTR(fd, checksum_key, &buf, sizeof(buf)-1);
                if (attr_size != -1) {
                    buf[attr_size] = 0;
                    // Cached checksum found
                    g_debug("%s: Using checksum cached in xattr: [%s] %s",
                            __func__, checksum_key, buf);
                    *matches = (strcmp(expected, buf) == 0);
                    if (calculated)
                      *calculated = g_strdup(buf);
                    return TRUE;
                }
            } else {
                // timestamp stored in xattr is different => checksums are no longer valid
                lr_checksum_clear_cache(fd);
            }
        } else if (errno == ENOTSUP) {
            // Extended attributes are not supported by the filesystem, or are disabled.
            // Disable the use of extended attributes for cache.
            caching = FALSE;
        }
    }

    char *checksum = lr_checksum_fd(type, fd, err);
    if (!checksum)
        return FALSE;

    *matches = (strcmp(expected, checksum)) ? FALSE : TRUE;

    if (fsync(fd) != 0) {
        if (errno == EROFS || errno == EINVAL) {
            g_debug("fsync failed: %s", strerror(errno));
        } else {
            g_set_error(err, LR_CHECKSUM_ERROR, LRE_FILE,
                        "fsync failed: %s", strerror(errno));
            lr_free(checksum);
            return FALSE;
        }
    }

    if (caching && *matches && timestamp != -1) {
        // Store timestamp and checksum as extended file attribute if caching is enabled
        FSETXATTR(fd, XATTR_CHKSUM_MTIME, timestamp_str, strlen(timestamp_str), 0);
        FSETXATTR(fd, checksum_key, checksum, strlen(checksum), 0);
    }

    if (calculated)
        *calculated = g_strdup(checksum);

    lr_free(checksum);
    return TRUE;
}


void
lr_checksum_clear_cache(int fd)
{
    // Remove the extended attribute containing mtime.
    if (FREMOVEXATTR(fd, XATTR_CHKSUM_MTIME) == -1) {
        if (errno == ENOTSUP) {
            // Extended attributes are not supported by the filesystem, or are disabled.
            return;
        }
    }

    // Remove the other extended attributes used by librepo to store the file checksum.
    // Librepo does not need this to work properly. Deleting the mtime attribute is sufficient.
    // We're just cleaning up.
    ssize_t xattrs_len = FLISTXATTR(fd, NULL, 0);
    if (xattrs_len <= 0) {
        return;
    }
    char *xattrs = lr_malloc(xattrs_len);
    ssize_t bytes_read = FLISTXATTR(fd, xattrs, xattrs_len);
    if (bytes_read < 0) {
        goto cleanup;
    }
    ssize_t prefix_len = strlen(XATTR_CHKSUM_PREFIX);
    const char *attr = xattrs;
    while (attr < xattrs + xattrs_len) {
        if (strncmp(XATTR_CHKSUM_PREFIX, attr, prefix_len) == 0) {
            FREMOVEXATTR(fd, attr);
        }
        attr += strlen(attr) + 1;
    }
cleanup:
    lr_free(xattrs);
}
