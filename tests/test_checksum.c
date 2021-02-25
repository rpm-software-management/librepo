#define _GNU_SOURCE
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <fcntl.h>

#include "librepo/util.h"
#include "librepo/checksum.h"
#include "librepo/xattr_internal.h"

#include "fixtures.h"
#include "testsys.h"
#include "test_checksum.h"

#define CHKS_CONTENT_00 ""
#define CHKS_VAL_00_MD5     "d41d8cd98f00b204e9800998ecf8427e"
#define CHKS_VAL_00_SHA1    "da39a3ee5e6b4b0d3255bfef95601890afd80709"
#define CHKS_VAL_00_SHA224  "d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f"
#define CHKS_VAL_00_SHA256  "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
#define CHKS_VAL_00_SHA384  "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b"
#define CHKS_VAL_00_SHA512  "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"

#define CHKS_CONTENT_01 "foo\nbar\n\n"
#define CHKS_VAL_01_MD5     "8b03476e0c92a803ed47e0817b2717ed"
#define CHKS_VAL_01_SHA1    "db957786773c3b2ae5fc2d2c839bab1bbe5dc09e"
#define CHKS_VAL_01_SHA224  "00f288c08f7f745d59252f875007b6734bb2f0aac295463cf242aac0"
#define CHKS_VAL_01_SHA256  "242218bd1466c1ab22e501d1e86bc906bacfd6943ded790ea8b2b040e618d81b"
#define CHKS_VAL_01_SHA384  "1f9d03f7a9fc1c22bfe114e2b58c334fcf58a07df78b4b0e942a28582ebf6489823e242492b2e0e5df1ce995e918c80d"
#define CHKS_VAL_01_SHA512  "704861d613afe433160d9b5aa6870e0dd96f5e56c4976f1fe39f4f648e37517ad7374209034290284949b4218ab0c8d8860f941c884cad47a61f208803128049"

static void
build_test_file(const char *filename, const char *content)
{
    FILE *fp = fopen(filename, "w");
    size_t len = strlen(content);
    fail_if(fp == NULL);
    fail_unless(fwrite(content, 1, len, fp) == len);
    fclose(fp);
}

static void
test_checksum(const char *filename, LrChecksumType ch_type, char *expected)
{
    int fd;
    char *checksum;
    GError *tmp_err = NULL;

    fd = open(filename, O_RDONLY);
    fail_if(fd < 0);
    checksum = lr_checksum_fd(ch_type, fd, &tmp_err);
    fail_if(checksum == NULL);
    fail_if(tmp_err);
    fail_if(strcmp(checksum, expected),
        "Checksum is %s instead of %s", checksum, expected);
    lr_free(checksum);
    close(fd);
}

START_TEST(test_checksum_fd)
{
    char *file;

    file = lr_pathconcat(test_globals.tmpdir, "/test_checksum", NULL);

    /* Empty file */
    build_test_file(file, CHKS_CONTENT_00);
    test_checksum(file, LR_CHECKSUM_MD5,    CHKS_VAL_00_MD5);
    test_checksum(file, LR_CHECKSUM_SHA1,   CHKS_VAL_00_SHA1);
    test_checksum(file, LR_CHECKSUM_SHA224, CHKS_VAL_00_SHA224);
    test_checksum(file, LR_CHECKSUM_SHA256, CHKS_VAL_00_SHA256);
    test_checksum(file, LR_CHECKSUM_SHA384, CHKS_VAL_00_SHA384);
    test_checksum(file, LR_CHECKSUM_SHA512, CHKS_VAL_00_SHA512);

    /* File with some content */
    build_test_file(file, CHKS_CONTENT_01);
    test_checksum(file, LR_CHECKSUM_MD5,    CHKS_VAL_01_MD5);
    test_checksum(file, LR_CHECKSUM_SHA1,   CHKS_VAL_01_SHA1);
    test_checksum(file, LR_CHECKSUM_SHA224, CHKS_VAL_01_SHA224);
    test_checksum(file, LR_CHECKSUM_SHA256, CHKS_VAL_01_SHA256);
    test_checksum(file, LR_CHECKSUM_SHA384, CHKS_VAL_01_SHA384);
    test_checksum(file, LR_CHECKSUM_SHA512, CHKS_VAL_01_SHA512);

    fail_if(remove(file) != 0, "Cannot delete temporary test file");
    lr_free(file);
}
END_TEST

START_TEST(test_cached_checksum)
{
    FILE *f;
    int fd, ret;
    gboolean checksum_ret, matches;
    ssize_t attr_ret;
    char *filename;
    static char *expected = "d78931fcf2660108eec0d6674ecb4e02401b5256a6b5ee82527766ef6d198c67";
    struct stat st;
    char buf[256];
    GError *tmp_err = NULL;
    gchar *timestamp_key = g_strconcat(XATTR_CHKSUM_PREFIX, "mtime", NULL);
    gchar *checksum_key = g_strconcat(XATTR_CHKSUM_PREFIX, "sha256", NULL);
    gchar *mtime_str = NULL;

    filename = lr_pathconcat(test_globals.tmpdir, "/test_checksum", NULL);
    f = fopen(filename, "w");
    fail_if(f == NULL);
    fwrite("foo\nbar\n", 1, 8, f);
    fclose(f);

    // Assert no cached checksum exists
    attr_ret = GETXATTR(filename, timestamp_key, &buf, sizeof(buf)-1);
    fail_if(attr_ret != -1);  // Cached timestamp should not exists
    attr_ret = GETXATTR(filename, checksum_key, &buf, sizeof(buf)-1);
    fail_if(attr_ret != -1);  // Cached checksum should not exists

    // Calculate checksum
    fd = open(filename, O_RDONLY);
    fail_if(fd < 0);
    checksum_ret = lr_checksum_fd_cmp(LR_CHECKSUM_SHA256,
                                      fd,
                                      expected,
                                      1,
                                      &matches,
                                      &tmp_err);
    fail_if(tmp_err);
    fail_if(!checksum_ret);
    fail_if(!matches);
    close(fd);

    // Assert cached checksum exists
    attr_ret = GETXATTR(filename, checksum_key, &buf, sizeof(buf)-1);

    if (attr_ret == -1) {
        // Error encountered
        if (errno == ENOTSUP) {
            // Extended attributes are not supported
            goto exit_label;
        }
        // Any other errno means fail
        fail_if(attr_ret == -1);
    } else {
        buf[attr_ret] = 0;
        fail_if(strcmp(buf, expected));
    }

    // stored timestamp matches the file mtime
    ret = stat(filename, &st);
    fail_if(ret != 0);
    mtime_str = g_strdup_printf("%lli", (long long) st.st_mtime);
    attr_ret = GETXATTR(filename, timestamp_key, &buf, sizeof(buf)-1);
    fail_if(attr_ret == -1);
    buf[attr_ret] = 0;
    fail_if(strcmp(buf, mtime_str));

    // Calculate checksum again (cached shoud be used this time)
    fd = open(filename, O_RDONLY);
    fail_if(fd < 0);
    checksum_ret = lr_checksum_fd_cmp(LR_CHECKSUM_SHA256,
                                      fd,
                                      expected,
                                      1,
                                      &matches,
                                      &tmp_err);
    fail_if(tmp_err);
    fail_if(!checksum_ret);
    fail_if(!matches);
    close(fd);

exit_label:
    lr_free(filename);
    lr_free(timestamp_key);
    lr_free(checksum_key);
    lr_free(mtime_str);
}
END_TEST

START_TEST(test_cached_checksum_clear)
{
    FILE *f;
    int fd;
    ssize_t attr_ret;
    char *filename;
    char buf[256];
    gchar *timestamp_key = g_strconcat(XATTR_CHKSUM_PREFIX, "mtime", NULL);
    gchar *checksum_key = g_strconcat(XATTR_CHKSUM_PREFIX, "sha256", NULL);
    const char *other_key = "user.Other.Attribute";
    const char *value = "some value";

    filename = lr_pathconcat(test_globals.tmpdir, "/test_checksum_clear", NULL);
    f = fopen(filename, "w");
    fail_if(f == NULL);
    fclose(f);

    // set extended attributes
    fd = open(filename, O_RDONLY);
    fail_if(fd < 0);
    attr_ret = FSETXATTR(fd, timestamp_key, value, strlen(value), 0);
    if (attr_ret == -1) {
        if (errno == ENOTSUP) {
            goto cleanup;
        }
        fail_if(attr_ret == -1);
    }
    attr_ret = FSETXATTR(fd, checksum_key, value, strlen(value), 0);
    fail_if(attr_ret == -1);
    attr_ret = FSETXATTR(fd, other_key, value, strlen(value), 0);
    fail_if(attr_ret == -1);

    // verify that xattrs are set
    attr_ret = GETXATTR(filename, timestamp_key, &buf, sizeof(buf));
    fail_if(attr_ret == -1);
    attr_ret = GETXATTR(filename, checksum_key, &buf, sizeof(buf));
    fail_if(attr_ret == -1);
    attr_ret = GETXATTR(filename, other_key, &buf, sizeof(buf));
    fail_if(attr_ret == -1);

    lr_checksum_clear_cache(fd);

    // verify that checksum xattrs are removed
    attr_ret = GETXATTR(filename, timestamp_key, &buf, sizeof(buf));
    fail_if(attr_ret != -1);
    attr_ret = GETXATTR(filename, checksum_key, &buf, sizeof(buf));
    fail_if(attr_ret != -1);
    // other then checksum related attributes are not removed
    attr_ret = GETXATTR(filename, other_key, &buf, sizeof(buf));
    fail_if(attr_ret == -1);
cleanup:
    close(fd);
    lr_free(filename);
    lr_free(timestamp_key);
    lr_free(checksum_key);
}
END_TEST

Suite *
checksum_suite(void)
{
    Suite *s = suite_create("checksum");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_checksum_fd);
    tcase_add_test(tc, test_cached_checksum);
    tcase_add_test(tc, test_cached_checksum_clear);
    suite_add_tcase(s, tc);
    return s;
}
