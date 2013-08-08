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
    fail_if(ml == NULL);
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
    lr_free(path);
    fail_if(fd < 0);
    ml = lr_metalink_init();
    fail_if(ml == NULL);
    ret = lr_metalink_parse_file(ml, fd, REPOMD, NULL, NULL, &tmp_err);
    fail_if(!ret);
    fail_if(tmp_err);
    close(fd);

    fail_if(ml->filename == NULL);
    fail_if(strcmp(ml->filename, "repomd.xml"));
    fail_if(ml->timestamp != 1337942396);
    fail_if(ml->size != 4309);
    fail_if(g_slist_length(ml->hashes) != 4);
    fail_if(g_slist_length(ml->urls) != 106);

    elem = g_slist_nth(ml->hashes, 0);
    fail_if(!elem);
    mlhash = elem->data;
    fail_if(!mlhash);
    fail_if(mlhash->type == NULL);
    fail_if(strcmp(mlhash->type, "md5"));
    fail_if(mlhash->value == NULL);
    fail_if(strcmp(mlhash->value, "20b6d77930574ae541108e8e7987ad3f"));

    elem = g_slist_nth(ml->hashes, 1);
    fail_if(!elem);
    mlhash = elem->data;
    fail_if(!mlhash);
    fail_if(mlhash->type == NULL);
    fail_if(strcmp(mlhash->type, "sha1"));
    fail_if(mlhash->value == NULL);
    fail_if(strcmp(mlhash->value, "4a5ae1831a567b58e2e0f0de1529ca199d1d8319"));

    elem = g_slist_nth(ml->hashes, 2);
    fail_if(!elem);
    mlhash = elem->data;
    fail_if(!mlhash);
    fail_if(mlhash->type == NULL);
    fail_if(strcmp(mlhash->type, "sha256"));
    fail_if(mlhash->value == NULL);
    fail_if(strcmp(mlhash->value, "0076c44aabd352da878d5c4d794901ac87f66afac869488f6a4ef166de018cdf"));

    elem = g_slist_nth(ml->hashes, 3);
    fail_if(!elem);
    mlhash = elem->data;
    fail_if(!mlhash);
    fail_if(mlhash->type == NULL);
    fail_if(strcmp(mlhash->type, "sha512"));
    fail_if(mlhash->value == NULL);
    fail_if(strcmp(mlhash->value, "884dc465da67fee8fe3f11dab321a99d9a13b22ce97f84ceff210e82b6b1a8c635ccd196add1dd738807686714c3a0a048897e2d0650bc05302b3ee26de521fd"));

    elem = g_slist_nth(ml->urls, 0);
    fail_if(!elem);
    mlurl = elem->data;
    fail_if(!mlurl);
    fail_if(mlurl->protocol == NULL);
    fail_if(strcmp(mlurl->protocol, "http"));
    fail_if(mlurl->type == NULL);
    fail_if(strcmp(mlurl->type, "http"));
    fail_if(mlurl->location == NULL);
    fail_if(strcmp(mlurl->location, "US"));
    fail_if(mlurl->preference != 99);
    fail_if(mlurl->url == NULL);
    fail_if(strcmp(mlurl->url,
                   "http://mirror.pnl.gov/fedora/linux/releases/17/Everything/x86_64/os/repodata/repomd.xml"));

    elem = g_slist_nth(ml->urls, 2);
    fail_if(!elem);
    mlurl = elem->data;
    fail_if(!mlurl);
    fail_if(mlurl->protocol == NULL);
    fail_if(strcmp(mlurl->protocol, "ftp"));
    fail_if(mlurl->type == NULL);
    fail_if(strcmp(mlurl->type, "ftp"));
    fail_if(mlurl->location == NULL);
    fail_if(strcmp(mlurl->location, "US"));
    fail_if(mlurl->preference != 98);
    fail_if(mlurl->url == NULL);
    fail_if(strcmp(mlurl->url,
                   "ftp://mirrors.syringanetworks.net/fedora/releases/17/Everything/x86_64/os/repodata/repomd.xml"));

    elem = g_slist_nth(ml->urls, 104);
    fail_if(!elem);
    mlurl = elem->data;
    fail_if(!mlurl);
    fail_if(mlurl->protocol == NULL);
    fail_if(strcmp(mlurl->protocol, "rsync"));
    fail_if(mlurl->type == NULL);
    fail_if(strcmp(mlurl->type, "rsync"));
    fail_if(mlurl->location == NULL);
    fail_if(strcmp(mlurl->location, "CA"));
    fail_if(mlurl->preference != 48);
    fail_if(mlurl->url == NULL);
    fail_if(strcmp(mlurl->url,
                   "rsync://mirror.csclub.uwaterloo.ca/fedora-enchilada/linux/releases/17/Everything/x86_64/os/repodata/repomd.xml"));

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
    lr_free(path);
    fail_if(fd < 0);
    ml = lr_metalink_init();
    fail_if(ml == NULL);
    ret = lr_metalink_parse_file(ml, fd, REPOMD, NULL, NULL, &tmp_err);
    fail_if(!ret);
    fail_if(tmp_err);
    close(fd);

    fail_if(ml->filename == NULL);
    fail_if(strcmp(ml->filename, "repomd.xml"));
    fail_if(ml->timestamp != 0);
    fail_if(ml->size != 0);
    fail_if(g_slist_length(ml->hashes) != 0);
    fail_if(g_slist_length(ml->urls) != 3);

    GSList *list = g_slist_nth(ml->urls, 0);
    fail_if(!list);
    LrMetalinkUrl *mlurl = list->data;
    fail_if(!mlurl);
    fail_if(mlurl->protocol == NULL);
    fail_if(strcmp(mlurl->protocol, "http"));
    fail_if(mlurl->type == NULL);
    fail_if(strcmp(mlurl->type, "http"));
    fail_if(mlurl->location == NULL);
    fail_if(strcmp(mlurl->location, "US"));
    fail_if(mlurl->preference != 97);
    fail_if(mlurl->url == NULL);
    fail_if(strcmp(mlurl->url,
                   "http://mirror.pnl.gov/fedora/linux/releases/17/Everything/x86_64/os/repodata/repomd.xml"));

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
    lr_free(path);
    fail_if(fd < 0);
    ml = lr_metalink_init();
    fail_if(ml == NULL);
    ret = lr_metalink_parse_file(ml, fd, REPOMD, NULL, NULL, &tmp_err);
    fail_if(!ret);
    fail_if(tmp_err);
    close(fd);

    fail_if(ml->filename == NULL);
    fail_if(strcmp(ml->filename, "repomd.xml"));
    fail_if(ml->timestamp != 0);
    fail_if(ml->size != 0);
    fail_if(g_slist_length(ml->hashes) != 0);
    fail_if(g_slist_length(ml->urls) != 0);

    lr_metalink_free(ml);
}
END_TEST

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
    lr_free(path);
    fail_if(fd < 0);
    ml = lr_metalink_init();
    fail_if(ml == NULL);
    ret = lr_metalink_parse_file(ml, fd, REPOMD, NULL, NULL, &tmp_err);
    fail_if(!ret);
    fail_if(tmp_err);
    close(fd);

    fail_if(ml->filename == NULL);
    fail_if(strcmp(ml->filename, "repomd.xml"));
    fail_if(ml->timestamp != 0);
    fail_if(ml->size != 0);
    fail_if(g_slist_length(ml->hashes) != 4);
    fail_if(g_slist_length(ml->urls) != 4);


    elem = g_slist_nth(ml->hashes, 0);
    fail_if(!elem);
    mlhash = elem->data;
    fail_if(!mlhash);
    fail_if(mlhash->type == NULL);
    fail_if(strcmp(mlhash->type, "md5"));
    fail_if(mlhash->value == NULL);
    fail_if(strcmp(mlhash->value,
                    "20b6d77930574ae541108e8e7987ad3f"));

    elem = g_slist_nth(ml->hashes, 1);
    fail_if(!elem);
    mlhash = elem->data;
    fail_if(!mlhash);
    fail_if(mlhash->type == NULL);
    fail_if(strcmp(mlhash->type, "foo"));
    fail_if(mlhash->value == NULL);
    fail_if(strcmp(mlhash->value, ""));

    elem = g_slist_nth(ml->hashes, 2);
    fail_if(!elem);
    mlhash = elem->data;
    fail_if(!mlhash);
    fail_if(mlhash->type == NULL);
    fail_if(strcmp(mlhash->type, "sha256"));
    fail_if(mlhash->value == NULL);
    fail_if(strcmp(mlhash->value,
                    "0076c44aabd352da878d5c4d794901ac87f66afac869488f6a4ef166de018cdf"));

    elem = g_slist_nth(ml->hashes, 3);
    fail_if(!elem);
    mlhash = elem->data;
    fail_if(!mlhash);
    fail_if(mlhash->type == NULL);
    fail_if(strcmp(mlhash->type, "sha512"));
    fail_if(mlhash->value == NULL);
    fail_if(strcmp(mlhash->value,
                    "884dc465da67fee8fe3f11dab321a99d9a13b22ce97f84ceff210e82b6b1a8c635ccd196add1dd738807686714c3a0a048897e2d0650bc05302b3ee26de521fd"));

    elem = g_slist_nth(ml->urls, 0);
    fail_if(!elem);
    mlurl = elem->data;
    fail_if(!mlurl);
    fail_if(mlurl->protocol == NULL);
    fail_if(strcmp(mlurl->protocol, "http"));
    fail_if(mlurl->type == NULL);
    fail_if(strcmp(mlurl->type, "http"));
    fail_if(mlurl->location == NULL);
    fail_if(strcmp(mlurl->location, "US"));
    fail_if(mlurl->preference != 0);
    fail_if(mlurl->url == NULL);
    fail_if(strcmp(mlurl->url,
                   "http://mirror.pnl.gov/fedora/linux/releases/17/Everything/x86_64/os/repodata/repomd.xml"));

    elem = g_slist_nth(ml->urls, 1);
    fail_if(!elem);
    mlurl = elem->data;
    fail_if(!mlurl);
    fail_if(mlurl->protocol != NULL);
    fail_if(mlurl->type != NULL);
    fail_if(mlurl->location != NULL);
    fail_if(mlurl->preference != -5);
    fail_if(mlurl->url == NULL);
    fail_if(strcmp(mlurl->url,
                   "ftp://mirrors.syringanetworks.net/fedora/releases/17/Everything/x86_64/os/repodata/repomd.xml"));

    elem = g_slist_nth(ml->urls, 2);
    fail_if(!elem);
    mlurl = elem->data;
    fail_if(!mlurl);
    fail_if(mlurl->protocol != NULL);
    fail_if(mlurl->type != NULL);
    fail_if(mlurl->location != NULL);
    fail_if(mlurl->preference != 0);
    fail_if(mlurl->url == NULL);
    fail_if(strcmp(mlurl->url,
                   "rsync://mirrors.syringanetworks.net/fedora/releases/17/Everything/x86_64/os/repodata/repomd.xml"));

    elem = g_slist_nth(ml->urls, 3);
    fail_if(!elem);
    mlurl = elem->data;
    fail_if(!mlurl);
    fail_if(mlurl->protocol != NULL);
    fail_if(mlurl->type != NULL);
    fail_if(mlurl->location != NULL);
    fail_if(mlurl->preference != 0);
    fail_if(mlurl->url == NULL);
    fail_if(strcmp(mlurl->url, ""));

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
    lr_free(path);
    fail_if(fd < 0);
    ml = lr_metalink_init();
    fail_if(ml == NULL);
    ret = lr_metalink_parse_file(ml, fd, REPOMD, NULL, NULL, &tmp_err);
    fail_if(!ret);
    fail_if(tmp_err);
    close(fd);
    fail_if(g_slist_length(ml->urls) != 0);
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
    lr_free(path);
    fail_if(fd < 0);
    ml = lr_metalink_init();
    fail_if(ml == NULL);
    ret = lr_metalink_parse_file(ml, fd, REPOMD, NULL, NULL, &tmp_err);
    fail_if(ret);
    fail_if(!tmp_err);
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
    lr_free(path);
    fail_if(fd < 0);
    ml = lr_metalink_init();
    fail_if(ml == NULL);
    ret = lr_metalink_parse_file(ml, fd, REPOMD, NULL, NULL, &tmp_err);
    fail_if(ret);
    fail_if(!tmp_err);
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
    lr_free(path);
    fail_if(fd < 0);
    ml = lr_metalink_init();
    fail_if(ml == NULL);
    ret = lr_metalink_parse_file(ml, fd, REPOMD, NULL, NULL, &tmp_err);
    fail_if(ret);
    fail_if(!tmp_err);
    g_error_free(tmp_err);
    close(fd);
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
    suite_add_tcase(s, tc);
    return s;
}
