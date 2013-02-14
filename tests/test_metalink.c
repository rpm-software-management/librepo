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
    lr_Metalink ml = NULL;

    ml = lr_metalink_init();
    fail_if(ml == NULL);
    lr_metalink_free(ml);
}
END_TEST

START_TEST(test_metalink_good_01)
{
    int fd;
    int ret;
    char *path;
    lr_Metalink ml = NULL;

    path = lr_pathconcat(test_globals.testdata_dir, METALINK_DIR,
                         "metalink_good_01", NULL);
    fd = open(path, O_RDONLY);
    lr_free(path);
    fail_if(fd < 0);
    ml = lr_metalink_init();
    fail_if(ml == NULL);
    ret = lr_metalink_parse_file(ml, fd, REPOMD);
    fail_if(ret != LRE_OK);
    close(fd);

    fail_if(ml->filename == NULL);
    fail_if(strcmp(ml->filename, "repomd.xml"));
    fail_if(ml->timestamp != 1337942396);
    fail_if(ml->size != 4309);
    fail_if(ml->noh != 4);
    fail_if(ml->nou != 106);

    fail_if(ml->hashes[0]->type == NULL);
    fail_if(strcmp(ml->hashes[0]->type, "md5"));
    fail_if(ml->hashes[0]->value == NULL);
    fail_if(strcmp(ml->hashes[0]->value,
                    "20b6d77930574ae541108e8e7987ad3f"));

    fail_if(ml->hashes[1]->type == NULL);
    fail_if(strcmp(ml->hashes[1]->type, "sha1"));
    fail_if(ml->hashes[1]->value == NULL);
    fail_if(strcmp(ml->hashes[1]->value,
                    "4a5ae1831a567b58e2e0f0de1529ca199d1d8319"));

    fail_if(ml->hashes[2]->type == NULL);
    fail_if(strcmp(ml->hashes[2]->type, "sha256"));
    fail_if(ml->hashes[2]->value == NULL);
    fail_if(strcmp(ml->hashes[2]->value,
                    "0076c44aabd352da878d5c4d794901ac87f66afac869488f6a4ef166de018cdf"));

    fail_if(ml->hashes[3]->type == NULL);
    fail_if(strcmp(ml->hashes[3]->type, "sha512"));
    fail_if(ml->hashes[3]->value == NULL);
    fail_if(strcmp(ml->hashes[3]->value,
                    "884dc465da67fee8fe3f11dab321a99d9a13b22ce97f84ceff210e82b6b1a8c635ccd196add1dd738807686714c3a0a048897e2d0650bc05302b3ee26de521fd"));

    fail_if(ml->urls[0]->protocol == NULL);
    fail_if(strcmp(ml->urls[0]->protocol, "http"));
    fail_if(ml->urls[0]->type == NULL);
    fail_if(strcmp(ml->urls[0]->type, "http"));
    fail_if(ml->urls[0]->location == NULL);
    fail_if(strcmp(ml->urls[0]->location, "US"));
    fail_if(ml->urls[0]->preference != 99);
    fail_if(ml->urls[0]->url == NULL);
    fail_if(strcmp(ml->urls[0]->url,
                   "http://mirror.pnl.gov/fedora/linux/releases/17/Everything/x86_64/os/repodata/repomd.xml"));

    fail_if(ml->urls[2]->protocol == NULL);
    fail_if(strcmp(ml->urls[2]->protocol, "ftp"));
    fail_if(ml->urls[2]->type == NULL);
    fail_if(strcmp(ml->urls[2]->type, "ftp"));
    fail_if(ml->urls[2]->location == NULL);
    fail_if(strcmp(ml->urls[2]->location, "US"));
    fail_if(ml->urls[2]->preference != 98);
    fail_if(ml->urls[2]->url == NULL);
    fail_if(strcmp(ml->urls[2]->url,
                   "ftp://mirrors.syringanetworks.net/fedora/releases/17/Everything/x86_64/os/repodata/repomd.xml"));

    fail_if(ml->urls[104]->protocol == NULL);
    fail_if(strcmp(ml->urls[104]->protocol, "rsync"));
    fail_if(ml->urls[104]->type == NULL);
    fail_if(strcmp(ml->urls[104]->type, "rsync"));
    fail_if(ml->urls[104]->location == NULL);
    fail_if(strcmp(ml->urls[104]->location, "CA"));
    fail_if(ml->urls[104]->preference != 48);
    fail_if(ml->urls[104]->url == NULL);
    fail_if(strcmp(ml->urls[104]->url,
                   "rsync://mirror.csclub.uwaterloo.ca/fedora-enchilada/linux/releases/17/Everything/x86_64/os/repodata/repomd.xml"));

    lr_metalink_free(ml);
}
END_TEST

START_TEST(test_metalink_good_02)
{
    int fd;
    int ret;
    char *path;
    lr_Metalink ml = NULL;

    path = lr_pathconcat(test_globals.testdata_dir, METALINK_DIR,
                         "metalink_good_02", NULL);
    fd = open(path, O_RDONLY);
    lr_free(path);
    fail_if(fd < 0);
    ml = lr_metalink_init();
    fail_if(ml == NULL);
    ret = lr_metalink_parse_file(ml, fd, REPOMD);
    fail_if(ret != LRE_OK);
    close(fd);

    fail_if(ml->filename == NULL);
    fail_if(strcmp(ml->filename, "repomd.xml"));
    fail_if(ml->timestamp != 0);
    fail_if(ml->size != 0);
    fail_if(ml->noh != 0);
    fail_if(ml->nou != 3);

    fail_if(ml->urls[0]->protocol == NULL);
    fail_if(strcmp(ml->urls[0]->protocol, "http"));
    fail_if(ml->urls[0]->type == NULL);
    fail_if(strcmp(ml->urls[0]->type, "http"));
    fail_if(ml->urls[0]->location == NULL);
    fail_if(strcmp(ml->urls[0]->location, "US"));
    fail_if(ml->urls[0]->preference != 97);
    fail_if(ml->urls[0]->url == NULL);
    fail_if(strcmp(ml->urls[0]->url,
                   "http://mirror.pnl.gov/fedora/linux/releases/17/Everything/x86_64/os/repodata/repomd.xml"));

    lr_metalink_free(ml);
}
END_TEST

START_TEST(test_metalink_good_03)
{
    int fd;
    int ret;
    char *path;
    lr_Metalink ml = NULL;

    path = lr_pathconcat(test_globals.testdata_dir, METALINK_DIR,
                         "metalink_good_03", NULL);
    fd = open(path, O_RDONLY);
    lr_free(path);
    fail_if(fd < 0);
    ml = lr_metalink_init();
    fail_if(ml == NULL);
    ret = lr_metalink_parse_file(ml, fd, REPOMD);
    fail_if(ret != LRE_OK);
    close(fd);

    fail_if(ml->filename == NULL);
    fail_if(strcmp(ml->filename, "repomd.xml"));
    fail_if(ml->timestamp != 0);
    fail_if(ml->size != 0);
    fail_if(ml->noh != 0);
    fail_if(ml->nou != 0);

    lr_metalink_free(ml);
}
END_TEST

START_TEST(test_metalink_bad_01)
{
    int fd;
    int ret;
    char *path;
    lr_Metalink ml = NULL;

    path = lr_pathconcat(test_globals.testdata_dir, METALINK_DIR,
                         "metalink_bad_01", NULL);
    fd = open(path, O_RDONLY);
    lr_free(path);
    fail_if(fd < 0);
    ml = lr_metalink_init();
    fail_if(ml == NULL);
    ret = lr_metalink_parse_file(ml, fd, REPOMD);
    fail_if(ret != LRE_OK);
    close(fd);

    fail_if(ml->filename == NULL);
    fail_if(strcmp(ml->filename, "repomd.xml"));
    fail_if(ml->timestamp != 0);
    fail_if(ml->size != 0);
    fail_if(ml->noh != 4);
    fail_if(ml->nou != 4);

    fail_if(ml->hashes[0]->type == NULL);
    fail_if(strcmp(ml->hashes[0]->type, "md5"));
    fail_if(ml->hashes[0]->value == NULL);
    fail_if(strcmp(ml->hashes[0]->value,
                    "20b6d77930574ae541108e8e7987ad3f"));

    fail_if(ml->hashes[1]->type == NULL);
    fail_if(strcmp(ml->hashes[1]->type, "foo"));
    fail_if(ml->hashes[1]->value == NULL);
    fail_if(strcmp(ml->hashes[1]->value, ""));

    fail_if(ml->hashes[2]->type == NULL);
    fail_if(strcmp(ml->hashes[2]->type, "sha256"));
    fail_if(ml->hashes[2]->value == NULL);
    fail_if(strcmp(ml->hashes[2]->value,
                    "0076c44aabd352da878d5c4d794901ac87f66afac869488f6a4ef166de018cdf"));

    fail_if(ml->hashes[3]->type == NULL);
    fail_if(strcmp(ml->hashes[3]->type, "sha512"));
    fail_if(ml->hashes[3]->value == NULL);
    fail_if(strcmp(ml->hashes[3]->value,
                    "884dc465da67fee8fe3f11dab321a99d9a13b22ce97f84ceff210e82b6b1a8c635ccd196add1dd738807686714c3a0a048897e2d0650bc05302b3ee26de521fd"));

    fail_if(ml->urls[0]->protocol == NULL);
    fail_if(strcmp(ml->urls[0]->protocol, "http"));
    fail_if(ml->urls[0]->type == NULL);
    fail_if(strcmp(ml->urls[0]->type, "http"));
    fail_if(ml->urls[0]->location == NULL);
    fail_if(strcmp(ml->urls[0]->location, "US"));
    fail_if(ml->urls[0]->preference != 0);
    fail_if(ml->urls[0]->url == NULL);
    fail_if(strcmp(ml->urls[0]->url,
                   "http://mirror.pnl.gov/fedora/linux/releases/17/Everything/x86_64/os/repodata/repomd.xml"));

    fail_if(ml->urls[1]->protocol != NULL);
    fail_if(ml->urls[1]->type != NULL);
    fail_if(ml->urls[1]->location != NULL);
    fail_if(ml->urls[1]->preference != -5);
    fail_if(ml->urls[1]->url == NULL);
    fail_if(strcmp(ml->urls[1]->url,
                   "ftp://mirrors.syringanetworks.net/fedora/releases/17/Everything/x86_64/os/repodata/repomd.xml"));

    fail_if(ml->urls[2]->protocol != NULL);
    fail_if(ml->urls[2]->type != NULL);
    fail_if(ml->urls[2]->location != NULL);
    fail_if(ml->urls[2]->preference != 0);
    fail_if(ml->urls[2]->url == NULL);
    fail_if(strcmp(ml->urls[2]->url,
                   "rsync://mirrors.syringanetworks.net/fedora/releases/17/Everything/x86_64/os/repodata/repomd.xml"));

    fail_if(ml->urls[3]->protocol != NULL);
    fail_if(ml->urls[3]->type != NULL);
    fail_if(ml->urls[3]->location != NULL);
    fail_if(ml->urls[3]->preference != 0);
    fail_if(ml->urls[3]->url == NULL);
    fail_if(strcmp(ml->urls[3]->url, ""));

    lr_metalink_free(ml);
}
END_TEST

START_TEST(test_metalink_really_bad_01)
{
    int fd;
    int ret;
    char *path;
    lr_Metalink ml = NULL;

    path = lr_pathconcat(test_globals.testdata_dir, METALINK_DIR,
                         "metalink_really_bad_01", NULL);
    fd = open(path, O_RDONLY);
    lr_free(path);
    fail_if(fd < 0);
    ml = lr_metalink_init();
    fail_if(ml == NULL);
    ret = lr_metalink_parse_file(ml, fd, REPOMD);
    fail_if(ret == LRE_OK);
    close(fd);
    lr_metalink_free(ml);
}
END_TEST

START_TEST(test_metalink_really_bad_02)
{
    int fd;
    int ret;
    char *path;
    lr_Metalink ml = NULL;

    path = lr_pathconcat(test_globals.testdata_dir, METALINK_DIR,
                         "metalink_really_bad_02", NULL);
    fd = open(path, O_RDONLY);
    lr_free(path);
    fail_if(fd < 0);
    ml = lr_metalink_init();
    fail_if(ml == NULL);
    ret = lr_metalink_parse_file(ml, fd, REPOMD);
    fail_if(ret == LRE_OK);
    close(fd);
    lr_metalink_free(ml);
}
END_TEST

START_TEST(test_metalink_really_bad_03)
{
    int fd;
    int ret;
    char *path;
    lr_Metalink ml = NULL;

    path = lr_pathconcat(test_globals.testdata_dir, METALINK_DIR,
                         "metalink_really_bad_03", NULL);
    fd = open(path, O_RDONLY);
    lr_free(path);
    fail_if(fd < 0);
    ml = lr_metalink_init();
    fail_if(ml == NULL);
    ret = lr_metalink_parse_file(ml, fd, REPOMD);
    fail_if(ret == LRE_OK);
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
    tcase_add_test(tc, test_metalink_really_bad_01);
    tcase_add_test(tc, test_metalink_really_bad_02);
    tcase_add_test(tc, test_metalink_really_bad_03);
    suite_add_tcase(s, tc);
    return s;
}
