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

#define BUFFER_SIZE             2048
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
    _cleanup_free_ gchar *checksum = NULL;

    assert(fd >= 0);
    assert(!err || *err == NULL);

    *matches = FALSE;

    if (!expected) {
        g_set_error(err, LR_CHECKSUM_ERROR, LRE_BADFUNCARG,
                    "No expected checksum passed");
        return FALSE;
    }

    if (caching) {
        // Load cached checksum if enabled and used
        struct stat st;
        if (fstat(fd, &st) == 0) {
            ssize_t attr_ret;
            _cleanup_free_ gchar *key = NULL;
            char buf[256];

            key = g_strdup_printf("user.Zif.MdChecksum[%llu]",
                                  (unsigned long long) st.st_mtime);
            attr_ret = fgetxattr(fd, key, &buf, 256);
            if (attr_ret != -1) {
                // Cached checksum found
                g_debug("%s: Using checksum cached in xattr: [%s] %s",
                        __func__, key, buf);
                *matches = strcmp(expected, buf) ? FALSE : TRUE;
                return TRUE;
            }
        }
    }

    checksum = lr_checksum_fd(type, fd, err);
    if (!checksum)
        return FALSE;

    *matches = (strcmp(expected, checksum)) ? FALSE : TRUE;

    if (caching && *matches) {
        // Store checksum as extended file attribute if caching is enabled
        struct stat st;
        if (fstat(fd, &st) == 0) {
            _cleanup_free_ gchar *key = NULL;
            key = g_strdup_printf("user.Zif.MdChecksum[%llu]",
                                  (unsigned long long) st.st_mtime);
            fsetxattr(fd, key, checksum, strlen(checksum)+1, 0);
        }
    }

    if (calculated)
        *calculated = g_strdup(checksum);

    return TRUE;
}
