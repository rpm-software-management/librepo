#define _GNU_SOURCE
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "librepo/librepo.h"
#include "librepo/rcodes.h"
#include "librepo/util.h"
#include "librepo/downloader.h"
#include "librepo/handle_internal.h"

#include "fixtures.h"
#include "testsys.h"
#include "test_url_substitution.h"

// TODO: disable this dest by default (enable it by special compile flag)

START_TEST(test_downloader_no_list)
{
    int ret;
    GError *err = NULL;

    ret = lr_download(NULL, NULL, &err);
    fail_if(ret != LRE_OK);
    fail_if(err);
}
END_TEST

START_TEST(test_downloader_single_file)
{
    int ret;
    LrHandle *handle;
    GSList *list = NULL;
    GError *err = NULL;
    int fd1;
    char *tmpfn1;
    LrDownloadTarget *t1;
    GError *tmp_err = NULL;

    // Prepare list of download targets

    tmpfn1 = lr_pathconcat(test_globals.tmpdir, "single_file_XXXXXX", NULL);

    mktemp(tmpfn1);
    fd1 = open(tmpfn1, O_RDWR|O_CREAT|O_TRUNC, 0666);
    lr_free(tmpfn1);
    fail_if(fd1 == -1);

    t1 = lr_downloadtarget_new("index.html", NULL, fd1, 0, NULL,
                               0, NULL, NULL);

    list = g_slist_append(list, t1);

    // Download

    handle = lr_handle_init();
    fail_if(handle == NULL);

    lr_handle_setopt(handle, LRO_URL, "http://www.google.com");
    lr_handle_prepare_internal_mirrorlist(handle, &tmp_err);
    fail_if(tmp_err);

    ret = lr_download(handle, list, &err);
    fail_if(ret != LRE_OK);
    fail_if(err);

    lr_handle_free(handle);

    // Check results

    for (GSList *elem = list; elem; elem = g_slist_next(elem)) {
            LrDownloadTarget *dtarget = elem->data;
            if (dtarget->err) {
                printf("Error msg: %s\n", dtarget->err);
                ck_abort();
            }
    }

    g_slist_free_full(list, (GDestroyNotify) lr_downloadtarget_free);
}
END_TEST

START_TEST(test_downloader_single_file_2)
{
    int ret;
    GSList *list = NULL;
    GError *err = NULL;
    int fd1;
    char *tmpfn1;
    LrDownloadTarget *t1;

    // Prepare list of download targets

    tmpfn1 = lr_pathconcat(test_globals.tmpdir, "single_file_2_XXXXXX", NULL);

    mktemp(tmpfn1);
    fd1 = open(tmpfn1, O_RDWR|O_CREAT|O_TRUNC, 0666);
    lr_free(tmpfn1);
    fail_if(fd1 == -1);

    t1 = lr_downloadtarget_new("http://seznam.cz/index.html", NULL, fd1,
                               0, NULL, 0, NULL, NULL);

    list = g_slist_append(list, t1);

    // Download

    ret = lr_download(NULL, list, &err);
    fail_if(ret != LRE_OK);
    fail_if(err);

    // Check results

    for (GSList *elem = list; elem; elem = g_slist_next(elem)) {
            LrDownloadTarget *dtarget = elem->data;
            if (dtarget->err) {
                printf("Error msg: %s\n", dtarget->err);
                ck_abort();
            }
    }

    g_slist_free_full(list, (GDestroyNotify) lr_downloadtarget_free);
}
END_TEST

START_TEST(test_downloader_two_files)
{
    int ret;
    LrHandle *handle;
    GSList *list = NULL;
    GError *err = NULL;
    int fd1, fd2;
    char *tmpfn1, *tmpfn2;
    LrDownloadTarget *t1, *t2;
    GError *tmp_err = NULL;

    // Prepare list of download targets

    tmpfn1 = lr_pathconcat(test_globals.tmpdir, "single_file_1_XXXXXX", NULL);
    tmpfn2 = lr_pathconcat(test_globals.tmpdir, "single_file_2_XXXXXX", NULL);

    mktemp(tmpfn1);
    mktemp(tmpfn2);
    fd1 = open(tmpfn1, O_RDWR|O_CREAT|O_TRUNC, 0666);
    fd2 = open(tmpfn2, O_RDWR|O_CREAT|O_TRUNC, 0666);
    lr_free(tmpfn1);
    lr_free(tmpfn2);
    fail_if(fd1 == -1);
    fail_if(fd2 == -1);

    t1 = lr_downloadtarget_new("index.html", NULL, fd1, 0, NULL,
                               0, NULL, NULL);
    t2 = lr_downloadtarget_new("index.html", "http://seznam.cz", fd2, 0, NULL,
                               0, NULL, NULL);

    list = g_slist_append(list, t1);
    list = g_slist_append(list, t2);

    // Download

    handle = lr_handle_init();
    fail_if(handle == NULL);

    lr_handle_setopt(handle, LRO_URL, "http://www.google.com");
    lr_handle_prepare_internal_mirrorlist(handle, &tmp_err);
    fail_if(tmp_err);

    ret = lr_download(handle, list, &err);
    fail_if(ret != LRE_OK);
    fail_if(err);

    lr_handle_free(handle);

    // Check results

    for (GSList *elem = list; elem; elem = g_slist_next(elem)) {
            LrDownloadTarget *dtarget = elem->data;
            if (dtarget->err) {
                printf("Error msg: %s\n", dtarget->err);
                ck_abort();
            }
    }

    g_slist_free_full(list, (GDestroyNotify) lr_downloadtarget_free);
}
END_TEST

START_TEST(test_downloader_three_files_with_error)
{
    int ret;
    LrHandle *handle;
    GSList *list = NULL;
    GError *err = NULL;
    int fd1, fd2, fd3;
    char *tmpfn1, *tmpfn2, *tmpfn3;
    LrDownloadTarget *t1, *t2, *t3;
    GError *tmp_err = NULL;

    // Prepare list of download targets

    tmpfn1 = lr_pathconcat(test_globals.tmpdir, "single_file_1_XXXXXX", NULL);
    tmpfn2 = lr_pathconcat(test_globals.tmpdir, "single_file_2_XXXXXX", NULL);
    tmpfn3 = lr_pathconcat(test_globals.tmpdir, "single_file_3_XXXXXX", NULL);

    mktemp(tmpfn1);
    mktemp(tmpfn2);
    mktemp(tmpfn3);
    fd1 = open(tmpfn1, O_RDWR|O_CREAT|O_TRUNC, 0666);
    fd2 = open(tmpfn2, O_RDWR|O_CREAT|O_TRUNC, 0666);
    fd3 = open(tmpfn3, O_RDWR|O_CREAT|O_TRUNC, 0666);
    lr_free(tmpfn1);
    lr_free(tmpfn2);
    lr_free(tmpfn3);
    fail_if(fd1 == -1);
    fail_if(fd2 == -1);
    fail_if(fd3 == -1);

    t1 = lr_downloadtarget_new("index.html", NULL, fd1, 0, NULL,
                               0, NULL, NULL);
    t2 = lr_downloadtarget_new("index.html", "http://seznam.cz", fd2, 0, NULL,
                               0, NULL, NULL);
    t3 = lr_downloadtarget_new("i_hope_this_page_doesnt_exists.html",
                               "http://google.com", fd3, 0, NULL, 0, NULL, NULL);

    list = g_slist_append(list, t1);
    list = g_slist_append(list, t2);
    list = g_slist_append(list, t3);

    // Download

    handle = lr_handle_init();
    fail_if(handle == NULL);

    lr_handle_setopt(handle, LRO_URL, "http://www.google.com");
    lr_handle_prepare_internal_mirrorlist(handle, &tmp_err);
    fail_if(tmp_err);

    ret = lr_download(handle, list, &err);
    fail_if(ret != LRE_OK);
    fail_if(err);

    lr_handle_free(handle);

    // Check results

    int x = 0;
    for (GSList *elem = list; elem; elem = g_slist_next(elem)) {
            LrDownloadTarget *dtarget = elem->data;
            ++x;

            if (x != 3 && dtarget->err) {
                printf("Error msg: %s\n", dtarget->err);
                ck_abort();
            }

            if (x == 3 && !dtarget->err) {
                printf("No 404 error raised!");
                ck_abort();
            }
    }

    g_slist_free_full(list, (GDestroyNotify) lr_downloadtarget_free);
}
END_TEST

Suite *
downloader_suite(void)
{
    Suite *s = suite_create("downloader");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_downloader_no_list);
    tcase_add_test(tc, test_downloader_single_file);
    tcase_add_test(tc, test_downloader_single_file_2);
    tcase_add_test(tc, test_downloader_two_files);
    tcase_add_test(tc, test_downloader_three_files_with_error);
    suite_add_tcase(s, tc);
    return s;
}
