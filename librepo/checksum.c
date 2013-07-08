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

#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <attr/xattr.h>

#include <openssl/evp.h>

#include "setup.h"
#include "checksum.h"
#include "util.h"

#define BUFFER_SIZE             2048
#define MAX_CHECKSUM_NAME_LEN   7

lr_ChecksumType
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
        if (type_lower[2] == '2')
            return LR_CHECKSUM_MD2;
        else if (type_lower[2] == '5')
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
lr_checksum_type_to_str(lr_ChecksumType type)
{
    switch (type) {
    case LR_CHECKSUM_UNKNOWN:
        return "Unknown checksum";
    case LR_CHECKSUM_MD2:
        return "md2";
    case LR_CHECKSUM_MD5:
        return "md5";
    case LR_CHECKSUM_SHA:
        return "sha";
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
lr_checksum_fd(lr_ChecksumType type, int fd)
{
    int rc;
    unsigned int len, checksum_str_len;
    ssize_t readed;
    char buf[BUFFER_SIZE];
    unsigned char raw_checksum[EVP_MAX_MD_SIZE];
    char *checksum;
    EVP_MD_CTX *ctx;
    const EVP_MD *ctx_type;

    switch (type) {
        case LR_CHECKSUM_MD2:       ctx_type = EVP_md2();    break;
        case LR_CHECKSUM_MD5:       ctx_type = EVP_md5();    break;
        case LR_CHECKSUM_SHA:       ctx_type = EVP_sha();    break;
        case LR_CHECKSUM_SHA1:      ctx_type = EVP_sha1();   break;
        case LR_CHECKSUM_SHA224:    ctx_type = EVP_sha224(); break;
        case LR_CHECKSUM_SHA256:    ctx_type = EVP_sha256(); break;
        case LR_CHECKSUM_SHA384:    ctx_type = EVP_sha384(); break;
        case LR_CHECKSUM_SHA512:    ctx_type = EVP_sha512(); break;
        case LR_CHECKSUM_UNKNOWN:
        default:
            DEBUGASSERT(0);
            return NULL;
    }

    ctx = EVP_MD_CTX_create();
    rc = EVP_DigestInit_ex(ctx, ctx_type, NULL);
    if (!rc) {
        EVP_MD_CTX_destroy(ctx);
        return NULL;
    }

    while ((readed = read(fd, buf, BUFFER_SIZE)) > 0)
        EVP_DigestUpdate(ctx, buf, readed);

    if (readed == -1) {
        EVP_MD_CTX_destroy(ctx);
        return NULL;
    }

    EVP_DigestFinal_ex(ctx, raw_checksum, &len);
    EVP_MD_CTX_destroy(ctx);
    checksum_str_len = len * 2 + 1;
    checksum = lr_malloc0(sizeof(char) * checksum_str_len);
    for (size_t x = 0; x < len; x++)
        sprintf(checksum+(x*2), "%02x", raw_checksum[x]);

    return checksum;
}

int
lr_checksum_fd_cmp(lr_ChecksumType type,
                   int fd,
                   const char *expected,
                   int caching)
{
    int ret;
    char *checksum;

    DEBUGASSERT(fd >= 0);

    if (!expected)
        return 1;

    if (caching) {
        // Load cached checksum if enabled and used
        struct stat st;
        if (fstat(fd, &st) == 0) {
            ssize_t attr_ret;
            char *key;
            char buf[256];

            lr_asprintf(&key, "user.Zif.MdChecksum[%llu]",
                        (unsigned long long) st.st_mtime);
            attr_ret = fgetxattr(fd, key, &buf, 256);
            lr_free(key);
            if (attr_ret != -1) {
                // Cached checksum found
                DPRINTF("Using checksum cached in xattr: %s\n", buf);
                return strcmp(expected, buf);
            }
        }
    }

    checksum = lr_checksum_fd(type, fd);
    if (!checksum)
        return 1;

    ret = strcmp(expected, checksum);

    if (caching && ret == 0) {
        // Store checksum as extended file attribute if caching is enabled
        struct stat st;
        if (fstat(fd, &st) == 0) {
            char *key;
            lr_asprintf(&key, "user.Zif.MdChecksum[%llu]",
                        (unsigned long long) st.st_mtime);
            fsetxattr(fd, key, checksum, strlen(checksum)+1, 0);
            lr_free(key);
        }
    }

    lr_free(checksum);
    return ret;
}
