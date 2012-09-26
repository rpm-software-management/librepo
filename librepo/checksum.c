#include <string.h>
#include <ctype.h>
#include <unistd.h>

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

    for (int x = 0; x <= len; x++)
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

char *
lr_checksum_calculate(lr_ChecksumType type, int fd)
{
    int rc;
    unsigned int len;
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
    rc = EVP_DigestInit(ctx, ctx_type);
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

    EVP_DigestFinal(ctx, raw_checksum, &len);
    checksum = lr_malloc0(sizeof(char) * (len * 2 + 1));
    for (int x = 0; x < len; x++)
        sprintf(checksum+(x*2), "%02x", raw_checksum[x]);

    return checksum;
}

int
lr_checksum_check(lr_ChecksumType type, int fd, const char *expected)
{
    int ret;
    char *checksum;

    DEBUGASSERT(fd >= 0);

    if (!expected)
        return 1;

    checksum = lr_checksum_calculate(type, fd);
    if (!checksum) {
        return 1;
    }

    ret = strcmp(expected, checksum);
    lr_free(checksum);
    return ret;
}
