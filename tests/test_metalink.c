#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "testsys.h"
#include "fixtures.h"
#include "test_metalink.h"
#include "librepo/rcodes.h"
#include "librepo/types.h"
#include "librepo/metalink.h"
#include "librepo/util.h"

#define REPOMD              "repomd.xml"
#define METALINK_DIR        "metalinks"

/* metalink_good_*
 *      This metalink files shoud be always parsed without problems.
 * metalink_bad_*
 *      This metalink files are no valid, but parser could/should be
 *      able parse it without returning (raising) an error.
 *      But this behaviour could change in future releases.
 *  metalink_really_bad_*
 *      This files are invalid and shoudn't and parsing shoud raise an error.
 **/

START_TEST(test_metalink_init)
{
    LrMetalink *ml = NULL;

    ml = lr_metalink_init();
    ck_assert_ptr_nonnull(ml);
    lr_metalink_free(ml);
}
END_TEST

START_TEST(test_metalink_good_01)
{
    int fd;
    gboolean ret;
    char *path;
    LrMetalink *ml = NULL;
    LrMetalinkHash *mlhash = NULL;
    LrMetalinkUrl *mlurl = NULL;
    GSList *elem = NULL;
    GError *tmp_err = NULL;

    path = lr_pathconcat(test_globals.testdata_dir, METALINK_DIR,
                         "metalink_good_01", NULL);
    fd = open(path, O_RDONLY);
    g_free(path);
    ck_assert_int_ge(fd, 0);
    ml = lr_metalink_init();
    ck_assert_ptr_nonnull(ml);
    ret = lr_metalink_parse_file(ml, fd, REPOMD, NULL, NULL, &tmp_err);
    ck_assert(ret);
    ck_assert_ptr_null(tmp_err);
    close(fd);

    ck_assert_ptr_nonnull(ml->filename);
    ck_assert_str_eq(ml->filename, "repomd.xml");
    ck_assert(ml->timestamp == 1337942396);
    ck_assert(ml->size == 4309);
    ck_assert(g_slist_length(ml->hashes) == 4);
    ck_assert(g_slist_length(ml->urls) == 106);

    elem = g_slist_nth(ml->hashes, 0);
    ck_assert_ptr_nonnull(elem);
    mlhash = elem->data;
    ck_assert_ptr_nonnull(mlhash);
    ck_assert_ptr_nonnull(mlhash->type);
    ck_assert_str_eq(mlhash->type, "md5");
    ck_assert_ptr_nonnull(mlhash->value);
    ck_assert_str_eq(mlhash->value, "20b6d77930574ae541108e8e7987ad3f");

    elem = g_slist_nth(ml->hashes, 1);
    ck_assert_ptr_nonnull(elem);
    mlhash = elem->data;
    ck_assert_ptr_nonnull(mlhash);
    ck_assert_ptr_nonnull(mlhash->type);
    ck_assert_str_eq(mlhash->type, "sha1");
    ck_assert_ptr_nonnull(mlhash->value);
    ck_assert_str_eq(mlhash->value, "4a5ae1831a567b58e2e0f0de1529ca199d1d8319");

    elem = g_slist_nth(ml->hashes, 2);
    ck_assert_ptr_nonnull(elem);
    mlhash = elem->data;
    ck_assert_ptr_nonnull(mlhash);
    ck_assert_ptr_nonnull(mlhash->type);
    ck_assert_str_eq(mlhash->type, "sha256");
    ck_assert_ptr_nonnull(mlhash->value);
    ck_assert_str_eq(mlhash->value, "0076c44aabd352da878d5c4d794901ac87f66afac869488f6a4ef166de018cdf");

    elem = g_slist_nth(ml->hashes, 3);
    ck_assert_ptr_nonnull(elem);
    mlhash = elem->data;
    ck_assert_ptr_nonnull(mlhash);
    ck_assert_ptr_nonnull(mlhash->type);
    ck_assert_str_eq(mlhash->type, "sha512");
    ck_assert_ptr_nonnull(mlhash->value);
    ck_assert_str_eq(mlhash->value, "884dc465da67fee8fe3f11dab321a99d9a13b22ce97f84ceff210e82b6b1a8c635ccd196add1dd738807686714c3a0a048897e2d0650bc05302b3ee26de521fd");

    elem = g_slist_nth(ml->urls, 0);
    ck_assert_ptr_nonnull(elem);
    mlurl = elem->data;
    ck_assert_ptr_nonnull(mlurl);
    ck_assert_ptr_nonnull(mlurl->protocol);
    ck_assert_str_eq(mlurl->protocol, "http");
    ck_assert_ptr_nonnull(mlurl->type);
    ck_assert_str_eq(mlurl->type, "http");
    ck_assert_ptr_nonnull(mlurl->location);
    ck_assert_str_eq(mlurl->location, "US");
    ck_assert(mlurl->preference == 99);
    ck_assert_ptr_nonnull(mlurl->url);
    ck_assert_str_eq(mlurl->url,
                   "http://mirror.pnl.gov/fedora/linux/releases/17/Everything/x86_64/os/repodata/repomd.xml");

    elem = g_slist_nth(ml->urls, 2);
    ck_assert_ptr_nonnull(elem);
    mlurl = elem->data;
    ck_assert_ptr_nonnull(mlurl);
    ck_assert_ptr_nonnull(mlurl->protocol);
    ck_assert_str_eq(mlurl->protocol, "ftp");
    ck_assert_ptr_nonnull(mlurl->type);
    ck_assert_str_eq(mlurl->type, "ftp");
    ck_assert_ptr_nonnull(mlurl->location);
    ck_assert_str_eq(mlurl->location, "US");
    ck_assert(mlurl->preference == 98);
    ck_assert_ptr_nonnull(mlurl->url);
    ck_assert_str_eq(mlurl->url,
                   "ftp://mirrors.syringanetworks.net/fedora/releases/17/Everything/x86_64/os/repodata/repomd.xml");

    elem = g_slist_nth(ml->urls, 104);
    ck_assert_ptr_nonnull(elem);
    mlurl = elem->data;
    ck_assert_ptr_nonnull(mlurl);
    ck_assert_ptr_nonnull(mlurl->protocol);
    ck_assert_str_eq(mlurl->protocol, "rsync");
    ck_assert_ptr_nonnull(mlurl->type);
    ck_assert_str_eq(mlurl->type, "rsync");
    ck_assert_ptr_nonnull(mlurl->location);
    ck_assert_str_eq(mlurl->location, "CA");
    ck_assert(mlurl->preference == 48);
    ck_assert_ptr_nonnull(mlurl->url);
    ck_assert_str_eq(mlurl->url,
                   "rsync://mirror.csclub.uwaterloo.ca/fedora-enchilada/linux/releases/17/Everything/x86_64/os/repodata/repomd.xml");

    lr_metalink_free(ml);
}
END_TEST

START_TEST(test_metalink_good_02)
{
    int fd;
    gboolean ret;
    char *path;
    LrMetalink *ml = NULL;
    GError *tmp_err = NULL;

    path = lr_pathconcat(test_globals.testdata_dir, METALINK_DIR,
                         "metalink_good_02", NULL);
    fd = open(path, O_RDONLY);
    g_free(path);
    ck_assert_int_ge(fd, 0);
    ml = lr_metalink_init();
    ck_assert_ptr_nonnull(ml);
    ret = lr_metalink_parse_file(ml, fd, REPOMD, NULL, NULL, &tmp_err);
    ck_assert(ret);
    ck_assert_ptr_null(tmp_err);
    close(fd);

    ck_assert_ptr_nonnull(ml->filename);
    ck_assert_str_eq(ml->filename, "repomd.xml");
    ck_assert(ml->timestamp == 0);
    ck_assert(ml->size == 0);
    ck_assert(g_slist_length(ml->hashes) == 0);
    ck_assert(g_slist_length(ml->urls) == 3);

    GSList *list = g_slist_nth(ml->urls, 0);
    ck_assert_ptr_nonnull(list);
    LrMetalinkUrl *mlurl = list->data;
    ck_assert_ptr_nonnull(mlurl);
    ck_assert_ptr_nonnull(mlurl->protocol);
    ck_assert_str_eq(mlurl->protocol, "http");
    ck_assert_ptr_nonnull(mlurl->type);
    ck_assert_str_eq(mlurl->type, "http");
    ck_assert_ptr_nonnull(mlurl->location);
    ck_assert_str_eq(mlurl->location, "US");
    ck_assert(mlurl->preference == 97);
    ck_assert_ptr_nonnull(mlurl->url);
    ck_assert_str_eq(mlurl->url,
                   "http://mirror.pnl.gov/fedora/linux/releases/17/Everything/x86_64/os/repodata/repomd.xml");

    lr_metalink_free(ml);
}
END_TEST

START_TEST(test_metalink_good_03)
{
    int fd;
    gboolean ret;
    char *path;
    LrMetalink *ml = NULL;
    GError *tmp_err = NULL;

    path = lr_pathconcat(test_globals.testdata_dir, METALINK_DIR,
                         "metalink_good_03", NULL);
    fd = open(path, O_RDONLY);
    g_free(path);
    ck_assert_int_ge(fd, 0);
    ml = lr_metalink_init();
    ck_assert_ptr_nonnull(ml);
    ret = lr_metalink_parse_file(ml, fd, REPOMD, NULL, NULL, &tmp_err);
    ck_assert(ret);
    ck_assert_ptr_null(tmp_err);
    close(fd);

    ck_assert_ptr_nonnull(ml->filename);
    ck_assert_str_eq(ml->filename, "repomd.xml");
    ck_assert(ml->timestamp == 0);
    ck_assert(ml->size == 0);
    ck_assert(g_slist_length(ml->hashes) == 0);
    ck_assert(g_slist_length(ml->urls) == 0);

    lr_metalink_free(ml);
}
END_TEST

static int
warning_cb(LrXmlParserWarningType type G_GNUC_UNUSED,
           char *msg G_GNUC_UNUSED,
           void *cbdata,
           GError **err G_GNUC_UNUSED)
{
    *((int *) cbdata) += 1;
    return LR_CB_RET_OK;
}

START_TEST(test_metalink_bad_01)
{
    int fd;
    gboolean ret;
    char *path;
    LrMetalink *ml = NULL;
    LrMetalinkHash *mlhash = NULL;
    LrMetalinkUrl *mlurl = NULL;
    GSList *elem = NULL;
    GError *tmp_err = NULL;

    path = lr_pathconcat(test_globals.testdata_dir, METALINK_DIR,
                         "metalink_bad_01", NULL);
    fd = open(path, O_RDONLY);
    g_free(path);
    ck_assert_int_ge(fd, 0);
    ml = lr_metalink_init();
    ck_assert_ptr_nonnull(ml);
    int call_counter = 0;
    ret = lr_metalink_parse_file(ml, fd, REPOMD, warning_cb, &call_counter, &tmp_err);
    ck_assert(ret);
    ck_assert_ptr_null(tmp_err);
    ck_assert_int_gt(call_counter, 0);
    close(fd);

    ck_assert_ptr_nonnull(ml->filename);
    ck_assert_str_eq(ml->filename, "repomd.xml");
    ck_assert(ml->timestamp == 0);
    ck_assert(ml->size == 0);
    ck_assert(g_slist_length(ml->hashes) == 4);
    ck_assert(g_slist_length(ml->urls) == 4);
    ck_assert(g_slist_length(ml->alternates) == 0);

    elem = g_slist_nth(ml->hashes, 0);
    ck_assert_ptr_nonnull(elem);
    mlhash = elem->data;
    ck_assert_ptr_nonnull(mlhash);
    ck_assert_ptr_nonnull(mlhash->type);
    ck_assert_str_eq(mlhash->type, "md5");
    ck_assert_ptr_nonnull(mlhash->value);
    ck_assert_str_eq(mlhash->value, "20b6d77930574ae541108e8e7987ad3f");

    elem = g_slist_nth(ml->hashes, 1);
    ck_assert_ptr_nonnull(elem);
    mlhash = elem->data;
    ck_assert_ptr_nonnull(mlhash);
    ck_assert_ptr_nonnull(mlhash->type);
    ck_assert_str_eq(mlhash->type, "foo");
    ck_assert_ptr_nonnull(mlhash->value);
    ck_assert_str_eq(mlhash->value, "");

    elem = g_slist_nth(ml->hashes, 2);
    ck_assert_ptr_nonnull(elem);
    mlhash = elem->data;
    ck_assert_ptr_nonnull(mlhash);
    ck_assert_ptr_nonnull(mlhash->type);
    ck_assert_str_eq(mlhash->type, "sha256");
    ck_assert_ptr_nonnull(mlhash->value);
    ck_assert_str_eq(mlhash->value, "0076c44aabd352da878d5c4d794901ac87f66afac869488f6a4ef166de018cdf");

    elem = g_slist_nth(ml->hashes, 3);
    ck_assert_ptr_nonnull(elem);
    mlhash = elem->data;
    ck_assert_ptr_nonnull(mlhash);
    ck_assert_ptr_nonnull(mlhash->type);
    ck_assert_str_eq(mlhash->type, "sha512");
    ck_assert_ptr_nonnull(mlhash->value);
    ck_assert_str_eq(mlhash->value,
                    "884dc465da67fee8fe3f11dab321a99d9a13b22ce97f84ceff210e82b6b1a8c635ccd196add1dd738807686714c3a0a048897e2d0650bc05302b3ee26de521fd");

    elem = g_slist_nth(ml->urls, 0);
    ck_assert_ptr_nonnull(elem);
    mlurl = elem->data;
    ck_assert_ptr_nonnull(mlurl);
    ck_assert_ptr_nonnull(mlurl->protocol);
    ck_assert_str_eq(mlurl->protocol, "http");
    ck_assert_ptr_nonnull(mlurl->type);
    ck_assert_str_eq(mlurl->type, "http");
    ck_assert_ptr_nonnull(mlurl->location);
    ck_assert_str_eq(mlurl->location, "US");
    ck_assert(mlurl->preference == 0);
    ck_assert_ptr_nonnull(mlurl->url);
    ck_assert_str_eq(mlurl->url,
                   "http://mirror.pnl.gov/fedora/linux/releases/17/Everything/x86_64/os/repodata/repomd.xml");

    elem = g_slist_nth(ml->urls, 1);
    ck_assert_ptr_nonnull(elem);
    mlurl = elem->data;
    ck_assert_ptr_nonnull(mlurl);
    ck_assert_ptr_null(mlurl->protocol);
    ck_assert_ptr_null(mlurl->type);
    ck_assert_ptr_null(mlurl->location);
    ck_assert(mlurl->preference >= 0 && mlurl->preference <= 100);
    ck_assert_ptr_nonnull(mlurl->url);
    ck_assert_str_eq(mlurl->url,
                   "ftp://mirrors.syringanetworks.net/fedora/releases/17/Everything/x86_64/os/repodata/repomd.xml");

    elem = g_slist_nth(ml->urls, 2);
    ck_assert_ptr_nonnull(elem);
    mlurl = elem->data;
    ck_assert_ptr_nonnull(mlurl);
    ck_assert_ptr_null(mlurl->protocol);
    ck_assert_ptr_null(mlurl->type);
    ck_assert_ptr_null(mlurl->location);
    ck_assert(mlurl->preference == 0);
    ck_assert_ptr_nonnull(mlurl->url);
    ck_assert_str_eq(mlurl->url,
                   "rsync://mirrors.syringanetworks.net/fedora/releases/17/Everything/x86_64/os/repodata/repomd.xml");

    elem = g_slist_nth(ml->urls, 3);
    ck_assert_ptr_nonnull(elem);
    mlurl = elem->data;
    ck_assert_ptr_nonnull(mlurl);
    ck_assert_ptr_null(mlurl->protocol);
    ck_assert_ptr_null(mlurl->type);
    ck_assert_ptr_null(mlurl->location);
    ck_assert(mlurl->preference == 0);
    ck_assert_ptr_nonnull(mlurl->url);
    ck_assert_str_eq(mlurl->url, "");

    lr_metalink_free(ml);
}
END_TEST

START_TEST(test_metalink_bad_02)
{
    int fd;
    gboolean ret;
    char *path;
    LrMetalink *ml = NULL;
    GError *tmp_err = NULL;

    path = lr_pathconcat(test_globals.testdata_dir, METALINK_DIR,
                         "metalink_bad_02", NULL);
    fd = open(path, O_RDONLY);
    g_free(path);
    ck_assert_int_ge(fd, 0);
    ml = lr_metalink_init();
    ck_assert_ptr_nonnull(ml);
    ret = lr_metalink_parse_file(ml, fd, REPOMD, NULL, NULL, &tmp_err);
    ck_assert(ret);
    ck_assert_ptr_null(tmp_err);
    close(fd);
    ck_assert(g_slist_length(ml->urls) == 0);
    lr_metalink_free(ml);
}
END_TEST

START_TEST(test_metalink_really_bad_01)
{
    int fd;
    gboolean ret;
    char *path;
    LrMetalink *ml = NULL;
    GError *tmp_err = NULL;

    path = lr_pathconcat(test_globals.testdata_dir, METALINK_DIR,
                         "metalink_really_bad_01", NULL);
    fd = open(path, O_RDONLY);
    g_free(path);
    ck_assert_int_ge(fd, 0);
    ml = lr_metalink_init();
    ck_assert_ptr_nonnull(ml);
    ret = lr_metalink_parse_file(ml, fd, REPOMD, NULL, NULL, &tmp_err);
    ck_assert(!ret);
    ck_assert_ptr_nonnull(tmp_err);
    g_error_free(tmp_err);
    close(fd);
    lr_metalink_free(ml);
}
END_TEST

START_TEST(test_metalink_really_bad_02)
{
    int fd;
    gboolean ret;
    char *path;
    LrMetalink *ml = NULL;
    GError *tmp_err = NULL;

    path = lr_pathconcat(test_globals.testdata_dir, METALINK_DIR,
                         "metalink_really_bad_02", NULL);
    fd = open(path, O_RDONLY);
    g_free(path);
    ck_assert_int_ge(fd, 0);
    ml = lr_metalink_init();
    ck_assert_ptr_nonnull(ml);
    ret = lr_metalink_parse_file(ml, fd, REPOMD, NULL, NULL, &tmp_err);
    ck_assert(!ret);
    ck_assert_ptr_nonnull(tmp_err);
    g_error_free(tmp_err);
    close(fd);
    lr_metalink_free(ml);
}
END_TEST

START_TEST(test_metalink_really_bad_03)
{
    int fd;
    gboolean ret;
    char *path;
    LrMetalink *ml = NULL;
    GError *tmp_err = NULL;

    path = lr_pathconcat(test_globals.testdata_dir, METALINK_DIR,
                         "metalink_really_bad_03", NULL);
    fd = open(path, O_RDONLY);
    g_free(path);
    ck_assert_int_ge(fd, 0);
    ml = lr_metalink_init();
    ck_assert_ptr_nonnull(ml);
    ret = lr_metalink_parse_file(ml, fd, REPOMD, NULL, NULL, &tmp_err);
    ck_assert(!ret);
    ck_assert_ptr_nonnull(tmp_err);
    g_error_free(tmp_err);
    close(fd);
    lr_metalink_free(ml);
}
END_TEST

START_TEST(test_metalink_with_alternates)
{
    int fd;
    gboolean ret;
    char *path;
    LrMetalink *ml = NULL;
    GSList *elem = NULL;
    LrMetalinkHash *mlhash = NULL;
    LrMetalinkAlternate *malternate = NULL;
    GError *tmp_err = NULL;

    path = lr_pathconcat(test_globals.testdata_dir, METALINK_DIR,
                         "metalink_with_alternates", NULL);
    fd = open(path, O_RDONLY);
    g_free(path);
    ck_assert_int_ge(fd, 0);
    ml = lr_metalink_init();
    ck_assert_ptr_nonnull(ml);
    ret = lr_metalink_parse_file(ml, fd, REPOMD, NULL, NULL, &tmp_err);
    ck_assert(ret);
    ck_assert_ptr_null(tmp_err);
    close(fd);

    ck_assert_ptr_nonnull(ml->filename);
    ck_assert_str_eq(ml->filename, "repomd.xml");
    ck_assert(g_slist_length(ml->hashes) == 4);
    ck_assert(g_slist_length(ml->alternates) == 1);

    elem = g_slist_nth(ml->hashes, 0);
    ck_assert_ptr_nonnull(elem);
    mlhash = elem->data;
    ck_assert_ptr_nonnull(mlhash);
    ck_assert_ptr_nonnull(mlhash->type);
    ck_assert_str_eq(mlhash->type, "md5");
    ck_assert_ptr_nonnull(mlhash->value);
    ck_assert_str_eq(mlhash->value, "0ffcd7798421c9a6760f3e4202cc4675");

    elem = g_slist_nth(ml->alternates, 0);
    ck_assert_ptr_nonnull(elem);
    malternate = elem->data;
    ck_assert(malternate->timestamp == 1381706941);
    ck_assert(malternate->size == 4761);
    ck_assert(g_slist_length(malternate->hashes) == 4);
    elem = g_slist_nth(malternate->hashes, 0);
    mlhash = elem->data;
    ck_assert_ptr_nonnull(mlhash);
    ck_assert_ptr_nonnull(mlhash->type);
    ck_assert_str_eq(mlhash->type, "md5");
    ck_assert_ptr_nonnull(mlhash->value);
    ck_assert_str_eq(mlhash->value, "0c5b64d395d5364633df7c8e97a07fd6");

    lr_metalink_free(ml);
}
END_TEST

Suite *
metalink_suite(void)
{
    Suite *s = suite_create("metalink");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_metalink_init);
    tcase_add_test(tc, test_metalink_good_01);
    tcase_add_test(tc, test_metalink_good_02);
    tcase_add_test(tc, test_metalink_good_03);
    tcase_add_test(tc, test_metalink_bad_01);
    tcase_add_test(tc, test_metalink_bad_02);
    tcase_add_test(tc, test_metalink_really_bad_01);
    tcase_add_test(tc, test_metalink_really_bad_02);
    tcase_add_test(tc, test_metalink_really_bad_03);
    tcase_add_test(tc, test_metalink_with_alternates);
    suite_add_tcase(s, tc);
    return s;
}
