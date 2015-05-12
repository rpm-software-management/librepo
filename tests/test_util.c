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

START_TEST(test_pathconcat)
{
    char *path = NULL;

    path = lr_pathconcat(NULL, NULL);
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
    fail_if(fd < 0);
    close(fd);

    rc = lr_remove_dir(tmp_dir);
    fail_if(rc != 0);
    fail_if(unlink(tmp_file) == 0);
    fail_if(rmdir(tmp_dir) == 0);
    lr_free(tmp_dir);
    lr_free(tmp_file);
}
END_TEST

START_TEST(test_url_without_path)
{
    char *new_url = NULL;

    new_url = lr_url_without_path(NULL);
    fail_if(new_url != NULL);

    new_url = lr_url_without_path("");
    fail_if(new_url == NULL);
    fail_if(strcmp(new_url, ""));
    lr_free(new_url);
    new_url = NULL;

    new_url = lr_url_without_path("hostname");
    fail_if(new_url == NULL);
    fail_if(strcmp(new_url, "hostname"));
    lr_free(new_url);
    new_url = NULL;

    new_url = lr_url_without_path("hostname/foo/bar/");
    fail_if(new_url == NULL);
    fail_if(strcmp(new_url, "hostname"));
    lr_free(new_url);
    new_url = NULL;

    new_url = lr_url_without_path("hostname:80");
    fail_if(new_url == NULL);
    fail_if(strcmp(new_url, "hostname:80"));
    lr_free(new_url);
    new_url = NULL;

    new_url = lr_url_without_path("hostname:80/foo/bar");
    fail_if(new_url == NULL);
    fail_if(strcmp(new_url, "hostname:80"));
    lr_free(new_url);
    new_url = NULL;

    new_url = lr_url_without_path("http://hostname:80/");
    fail_if(new_url == NULL);
    fail_if(strcmp(new_url, "http://hostname:80"));
    lr_free(new_url);
    new_url = NULL;

    new_url = lr_url_without_path("http://hostname:80/foo/bar");
    fail_if(new_url == NULL);
    fail_if(strcmp(new_url, "http://hostname:80"));
    lr_free(new_url);
    new_url = NULL;

    new_url = lr_url_without_path("ftp://foo.hostname:80/foo/bar");
    fail_if(new_url == NULL);
    fail_if(strcmp(new_url, "ftp://foo.hostname:80"));
    lr_free(new_url);
    new_url = NULL;

    new_url = lr_url_without_path("file:///home/foobar");
    fail_if(new_url == NULL);
    fail_if(strcmp(new_url, "file://"));
    lr_free(new_url);
    new_url = NULL;
}
END_TEST


START_TEST(test_strv_dup)
{
    gchar **in0 = NULL;
    gchar *in1[] = {NULL};
    gchar *in2[] = {"foo", NULL};
    gchar **copy = NULL;

    copy = lr_strv_dup(in0);
    fail_if(copy != NULL);

    copy = lr_strv_dup(in1);
    fail_if(!copy);
    fail_if(copy == in1);
    fail_if(copy[0] != NULL);
    g_strfreev(copy);

    copy = lr_strv_dup(in2);
    fail_if(!copy);
    fail_if(copy == in2);
    fail_if(g_strcmp0(copy[0], "foo"));
    fail_if(copy[0] == in2[0]);
    fail_if(copy[1] != NULL);
    g_strfreev(copy);
}
END_TEST

START_TEST(test_is_local_path)
{
    fail_if(!lr_is_local_path("/tmp"));
    fail_if(!lr_is_local_path("foo/bar"));
    fail_if(!lr_is_local_path("bar"));
    fail_if(!lr_is_local_path("/"));

    fail_if(lr_is_local_path(NULL));
    fail_if(lr_is_local_path(""));
    fail_if(lr_is_local_path("http://foo.bar"));
    fail_if(lr_is_local_path("https://foo.bar/x"));
    fail_if(lr_is_local_path("ftp://foo.bar/foobar"));
    fail_if(lr_is_local_path("rsync://xyz"));
}
END_TEST

START_TEST(test_prepend_url_protocol)
{
    gchar *url = NULL;

    url = lr_prepend_url_protocol("/tmp");
    fail_if(g_strcmp0(url, "file:///tmp"));
    g_free(url);

    url = lr_prepend_url_protocol("file:///tmp");
    fail_if(g_strcmp0(url, "file:///tmp"));
    g_free(url);

    url = lr_prepend_url_protocol("http://tmp");
    fail_if(g_strcmp0(url, "http://tmp"));
    g_free(url);

    url = lr_prepend_url_protocol("file:/tmp");
    fail_if(g_strcmp0(url, "file:/tmp"));
    g_free(url);
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
    tcase_add_test(tc, test_gettmpfile);
    tcase_add_test(tc, test_gettmpdir);
    tcase_add_test(tc, test_pathconcat);
    tcase_add_test(tc, test_remove_dir);
    tcase_add_test(tc, test_url_without_path);
    tcase_add_test(tc, test_strv_dup);
    tcase_add_test(tc, test_is_local_path);
    tcase_add_test(tc, test_prepend_url_protocol);
    suite_add_tcase(s, tc);
    return s;
}
