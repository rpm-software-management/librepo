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
    ck_assert_ptr_nonnull(num);
    lr_free(num);
}
END_TEST

START_TEST(test_malloc0)
{
    long long *num = NULL;
    num = lr_malloc0(sizeof(long long));
    ck_assert_ptr_nonnull(num);
    ck_assert(*num == 0LL);
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
    ck_assert_int_ge(fd, 0);
    close(fd);
}
END_TEST

START_TEST(test_gettmpdir)
{
    char *tmp_dir = lr_gettmpdir();
    ck_assert_ptr_nonnull(tmp_dir);
    ck_assert_int_eq(rmdir(tmp_dir), 0);
    g_free(tmp_dir);
}
END_TEST

START_TEST(test_pathconcat)
{
    char *path = NULL;

    path = lr_pathconcat(NULL, NULL);
    ck_assert_ptr_null(path);

    path = lr_pathconcat("", NULL);
    ck_assert_ptr_nonnull(path);
    ck_assert_str_eq(path, "");
    lr_free(path);
    path = NULL;

    path = lr_pathconcat("/tmp", "foo///", "bar", NULL);
    ck_assert_ptr_nonnull(path);
    ck_assert_str_eq(path, "/tmp/foo/bar");
    lr_free(path);
    path = NULL;

    path = lr_pathconcat("foo", "bar/", NULL);
    ck_assert_ptr_nonnull(path);
    ck_assert_str_eq(path, "foo/bar");
    lr_free(path);
    path = NULL;

    path = lr_pathconcat("foo", "/bar/", NULL);
    ck_assert_ptr_nonnull(path);
    ck_assert_str_eq(path, "foo/bar");
    lr_free(path);
    path = NULL;

    path = lr_pathconcat("foo", "bar", "", NULL);
    ck_assert_ptr_nonnull(path);
    ck_assert_str_eq(path, "foo/bar/");
    lr_free(path);
    path = NULL;

    path = lr_pathconcat("http://host.net", "path/to/somewhere", NULL);
    ck_assert_ptr_nonnull(path);
    ck_assert_str_eq(path, "http://host.net/path/to/somewhere");
    lr_free(path);
    path = NULL;

    path = lr_pathconcat("http://host.net?hello=1", "path/to/", "somewhere", NULL);
    ck_assert_ptr_nonnull(path);
    ck_assert_str_eq(path, "http://host.net/path/to/somewhere?hello=1");
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
    ck_assert_ptr_nonnull(tmp_dir);
    tmp_file = lr_pathconcat(tmp_dir, "file_a", NULL);
    fd = open(tmp_file, O_CREAT|O_TRUNC|O_RDWR, 0660);
    ck_assert_int_ge(fd, 0);
    close(fd);

    rc = lr_remove_dir(tmp_dir);
    ck_assert_int_eq(rc, 0);
    ck_assert_int_ne(unlink(tmp_file), 0);
    ck_assert_int_ne(rmdir(tmp_dir), 0);
    g_free(tmp_dir);
    lr_free(tmp_file);
}
END_TEST

START_TEST(test_url_without_path)
{
    char *new_url = NULL;

    new_url = lr_url_without_path(NULL);
    ck_assert_ptr_null(new_url);

    new_url = lr_url_without_path("");
    ck_assert_ptr_nonnull(new_url);
    ck_assert_str_eq(new_url, "");
    g_free(new_url);
    new_url = NULL;

    new_url = lr_url_without_path("hostname");
    ck_assert_ptr_nonnull(new_url);
    ck_assert_str_eq(new_url, "hostname");
    g_free(new_url);
    new_url = NULL;

    new_url = lr_url_without_path("hostname/foo/bar/");
    ck_assert_ptr_nonnull(new_url);
    ck_assert_str_eq(new_url, "hostname");
    g_free(new_url);
    new_url = NULL;

    new_url = lr_url_without_path("hostname:80");
    ck_assert_ptr_nonnull(new_url);
    ck_assert_str_eq(new_url, "hostname:80");
    g_free(new_url);
    new_url = NULL;

    new_url = lr_url_without_path("hostname:80/foo/bar");
    ck_assert_ptr_nonnull(new_url);
    ck_assert_str_eq(new_url, "hostname:80");
    g_free(new_url);
    new_url = NULL;

    new_url = lr_url_without_path("http://hostname:80/");
    ck_assert_ptr_nonnull(new_url);
    ck_assert_str_eq(new_url, "http://hostname:80");
    g_free(new_url);
    new_url = NULL;

    new_url = lr_url_without_path("http://hostname:80/foo/bar");
    ck_assert_ptr_nonnull(new_url);
    ck_assert_str_eq(new_url, "http://hostname:80");
    g_free(new_url);
    new_url = NULL;

    new_url = lr_url_without_path("ftp://foo.hostname:80/foo/bar");
    ck_assert_ptr_nonnull(new_url);
    ck_assert_str_eq(new_url, "ftp://foo.hostname:80");
    g_free(new_url);
    new_url = NULL;

    new_url = lr_url_without_path("file:///home/foobar");
    ck_assert_ptr_nonnull(new_url);
    ck_assert_str_eq(new_url, "file://");
    g_free(new_url);
    new_url = NULL;

    new_url = lr_url_without_path("file:/home/foobar");
    ck_assert_ptr_nonnull(new_url);
    ck_assert_str_eq(new_url, "file://");
    g_free(new_url);
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
    ck_assert_ptr_null(copy);

    copy = lr_strv_dup(in1);
    ck_assert(copy);
    ck_assert_ptr_ne(copy, in1);
    ck_assert_ptr_null(copy[0]);
    g_strfreev(copy);

    copy = lr_strv_dup(in2);
    ck_assert(copy);
    ck_assert_ptr_ne(copy, in2);
    ck_assert_str_eq(copy[0], "foo");
    ck_assert_ptr_ne(copy[0], in2[0]);
    ck_assert_ptr_null(copy[1]);
    g_strfreev(copy);
}
END_TEST

START_TEST(test_is_local_path)
{
    ck_assert(lr_is_local_path("/tmp"));
    ck_assert(lr_is_local_path("foo/bar"));
    ck_assert(lr_is_local_path("bar"));
    ck_assert(lr_is_local_path("/"));
    ck_assert(lr_is_local_path("file:///tmp"));
    ck_assert(lr_is_local_path("file:/tmp"));

    ck_assert(!lr_is_local_path(NULL));
    ck_assert(!lr_is_local_path(""));
    ck_assert(!lr_is_local_path("http://foo.bar"));
    ck_assert(!lr_is_local_path("https://foo.bar/x"));
    ck_assert(!lr_is_local_path("ftp://foo.bar/foobar"));
    ck_assert(!lr_is_local_path("rsync://xyz"));
}
END_TEST

START_TEST(test_prepend_url_protocol)
{
    gchar *url = NULL;

    url = lr_prepend_url_protocol("/tmp");
    ck_assert_str_eq(url, "file:///tmp");
    g_free(url);

    url = lr_prepend_url_protocol("file:///tmp");
    ck_assert_str_eq(url, "file:///tmp");
    g_free(url);

    url = lr_prepend_url_protocol("http://tmp");
    ck_assert_str_eq(url, "http://tmp");
    g_free(url);

    url = lr_prepend_url_protocol("file:/tmp");
    ck_assert_str_eq(url, "file:/tmp");
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
