#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "librepo/rcodes.h"
#include "librepo/util.h"

#include "fixtures.h"
#include "testsys.h"
#include "test_util.h"

START_TEST(test_malloc)
{
    long long *num = NULL;
    num = lr_malloc0(sizeof(long long));
    fail_if(num == NULL);
    lr_free(num);
}
END_TEST

START_TEST(test_malloc0)
{
    long long *num = NULL;
    num = lr_malloc0(sizeof(long long));
    fail_if(num == NULL);
    fail_if(*num != 0LL);
    lr_free(num);
}
END_TEST

START_TEST(test_free)
{
    // No SIGSEGV should be raised
    lr_free(NULL);
}
END_TEST

START_TEST(test_strdup)
{
    char msg[] = "Luke! I am your father!";
    char *dup = NULL;

    dup = lr_strdup(NULL);
    fail_if(dup != NULL);

    dup = lr_strdup(msg);
    fail_if(dup == NULL);
    fail_if(msg == dup);
    fail_if(strcmp(msg, dup) != 0);
    lr_free(dup);
}
END_TEST

START_TEST(test_strndup)
{
    char msg[] = "Luke! I am your father!";
    char *dup = NULL;

    dup = lr_strndup(NULL, 5);
    fail_if(dup != NULL);

    dup = lr_strndup(NULL, 0);
    fail_if(dup != NULL);

    dup = lr_strndup(msg, 5);
    fail_if(dup == NULL);
    fail_if(msg == dup);
    fail_if(strcmp(dup, "Luke!") != 0);
    lr_free(dup);
}
END_TEST

START_TEST(test_strconcat)
{
    char *msg = NULL;

    msg = lr_strconcat(NULL);
    fail_if(msg != NULL);

    msg = lr_strconcat("I see dead people.", NULL);
    fail_if(msg == NULL);
    fail_if(strcmp(msg, "I see dead people.") != 0);
    lr_free(msg);

    msg = NULL;
    msg = lr_strconcat("Houston, " "we have " "a problem", NULL);
    fail_if(msg == NULL);
    fail_if(strcmp(msg, "Houston, we have a problem") != 0);
    lr_free(msg);
}
END_TEST

START_TEST(test_gettmpfile)
{
    int fd = 0;
    fd = lr_gettmpfile();
    fail_if(fd < 0);
    close(fd);
}
END_TEST

START_TEST(test_gettmpdir)
{
    char *tmp_dir = lr_gettmpdir();
    fail_if(tmp_dir == NULL);
    fail_if(rmdir(tmp_dir) != 0);
    lr_free(tmp_dir);
}
END_TEST

START_TEST(test_ends_with)
{
    fail_if(lr_ends_with(NULL, NULL) != 0);
    fail_if(lr_ends_with(NULL, "ET phone home") != 0);
    fail_if(lr_ends_with("Bazinga!", NULL) != 0);
    fail_if(lr_ends_with("If I’m not back in five minutes", ", just wait longer") != 0);
    fail_if(lr_ends_with("Legen... Wait for it... dary", "dary") == 0);
    fail_if(lr_ends_with("Reskept", "") == 0);
    fail_if(lr_ends_with("", "") == 0);
    fail_if(lr_ends_with("", "Suit up!") != 0);
}
END_TEST

START_TEST(test_pathconcat)
{
    char *path = NULL;

    path = lr_pathconcat(NULL);
    fail_if(path != NULL);

    path = lr_pathconcat("", NULL);
    fail_if(path == NULL);
    fail_if(strcmp(path, ""));
    lr_free(path);
    path = NULL;

    path = lr_pathconcat("/tmp", "foo///", "bar", NULL);
    fail_if(path == NULL);
    fail_if(strcmp(path, "/tmp/foo/bar"));
    lr_free(path);
    path = NULL;

    path = lr_pathconcat("foo", "bar/", NULL);
    fail_if(path == NULL);
    fail_if(strcmp(path, "foo/bar"));
    lr_free(path);
    path = NULL;

    path = lr_pathconcat("foo", "/bar/", NULL);
    fail_if(path == NULL);
    fail_if(strcmp(path, "foo/bar"));
    lr_free(path);
    path = NULL;

    path = lr_pathconcat("foo", "bar", "", NULL);
    fail_if(path == NULL);
    fail_if(strcmp(path, "foo/bar/"));
    lr_free(path);
    path = NULL;
}
END_TEST

START_TEST(test_remove_dir)
{
    char *tmp_dir;
    char *tmp_file;
    int fd, rc;

    tmp_dir = lr_gettmpdir();
    fail_if(tmp_dir == NULL);
    tmp_file = lr_pathconcat(tmp_dir, "file_a", NULL);
    fd = open(tmp_file, O_CREAT|O_TRUNC|O_RDWR, 0660);
    fail_if(fd == -1);
    close(fd);

    rc = lr_remove_dir(tmp_dir);
    fail_if(rc != 0);
    fail_if(unlink(tmp_file) == 0);
    fail_if(rmdir(tmp_dir) == 0);
    lr_free(tmp_dir);
    lr_free(tmp_file);
}
END_TEST

START_TEST(test_asprintf)
{
    int len;
    char *str;

    len = lr_asprintf(&str, "Test %s", "foo");
    fail_if(len != 8);
    fail_if(str == NULL);
    fail_if(strcmp(str, "Test foo"));
    lr_free(str);
}
END_TEST

Suite *
util_suite(void)
{
    Suite *s = suite_create("util");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_malloc);
    tcase_add_test(tc, test_malloc0);
    tcase_add_test(tc, test_free);
    tcase_add_test(tc, test_strdup);
    tcase_add_test(tc, test_strndup);
    tcase_add_test(tc, test_strconcat);
    tcase_add_test(tc, test_gettmpfile);
    tcase_add_test(tc, test_gettmpdir);
    tcase_add_test(tc, test_ends_with);
    tcase_add_test(tc, test_pathconcat);
    tcase_add_test(tc, test_remove_dir);
    tcase_add_test(tc, test_asprintf);
    suite_add_tcase(s, tc);
    return s;
}
