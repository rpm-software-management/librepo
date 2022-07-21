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

START_TEST(test_downloader_no_list)
{
    GError *err = NULL;
    ck_assert(lr_download(NULL, FALSE, &err));
    ck_assert_ptr_null(err);
}
END_TEST

START_TEST(test_downloader_single_file)
{
    LrHandle *handle;
    GSList *list = NULL;
    GError *err = NULL;
    int fd1;
    char *tmpfn1;
    LrDownloadTarget *t1;
    GError *tmp_err = NULL;

    // Prepare handle

    handle = lr_handle_init();
    ck_assert_ptr_nonnull(handle);

    char *urls[] = {"http://www.google.com", NULL};
    ck_assert(lr_handle_setopt(handle, NULL, LRO_URLS, urls));
    lr_handle_prepare_internal_mirrorlist(handle, FALSE, &tmp_err);
    ck_assert_ptr_null(tmp_err);


    // Prepare list of download targets

    tmpfn1 = lr_pathconcat(test_globals.tmpdir, "single_file_XXXXXX", NULL);

    fd1 = mkstemp(tmpfn1);
    g_free(tmpfn1);
    ck_assert_int_ge(fd1, 0);

    t1 = lr_downloadtarget_new(handle, "index.html", NULL, fd1, NULL, NULL,
                               0, 0, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL,
                               FALSE, FALSE);
    ck_assert_ptr_nonnull(t1);

    list = g_slist_append(list, t1);

    // Download

    ck_assert(lr_download(list, FALSE, &err));
    ck_assert_ptr_null(err);

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
    close(fd1);
}
END_TEST

START_TEST(test_downloader_single_file_2)
{
    GSList *list = NULL;
    GError *err = NULL;
    int fd1;
    char *tmpfn1;
    LrDownloadTarget *t1;

    // Prepare list of download targets

    tmpfn1 = lr_pathconcat(test_globals.tmpdir, "single_file_2_XXXXXX", NULL);

    fd1 = mkstemp(tmpfn1);
    g_free(tmpfn1);
    ck_assert_int_ge(fd1, 0);

    t1 = lr_downloadtarget_new(NULL, "http://seznam.cz/index.html", NULL,
                               fd1, NULL, NULL, 0, 0, NULL, NULL, NULL,
                               NULL, NULL, 0, 0, NULL, FALSE, FALSE);
    ck_assert_ptr_nonnull(t1);

    list = g_slist_append(list, t1);

    // Download

    ck_assert(lr_download(list, FALSE, &err));
    ck_assert_ptr_null(err);

    // Check results

    for (GSList *elem = list; elem; elem = g_slist_next(elem)) {
            LrDownloadTarget *dtarget = elem->data;
            if (dtarget->err) {
                printf("Error msg: %s\n", dtarget->err);
                ck_abort();
            }
    }

    g_slist_free_full(list, (GDestroyNotify) lr_downloadtarget_free);
    close(fd1);
}
END_TEST

START_TEST(test_downloader_two_files)
{
    LrHandle *handle;
    GSList *list = NULL;
    GError *err = NULL;
    int fd1, fd2;
    char *tmpfn1, *tmpfn2;
    LrDownloadTarget *t1, *t2;
    GError *tmp_err = NULL;

    // Prepare handle

    handle = lr_handle_init();
    ck_assert_ptr_nonnull(handle);

    char *urls[] = {"http://www.google.com", NULL};
    ck_assert(lr_handle_setopt(handle, NULL, LRO_URLS, urls));
    lr_handle_prepare_internal_mirrorlist(handle, FALSE, &tmp_err);
    ck_assert_ptr_null(tmp_err);

    // Prepare list of download targets

    tmpfn1 = lr_pathconcat(test_globals.tmpdir, "single_file_1_XXXXXX", NULL);
    tmpfn2 = lr_pathconcat(test_globals.tmpdir, "single_file_2_XXXXXX", NULL);

    fd1 = mkstemp(tmpfn1);
    fd2 = mkstemp(tmpfn2);
    g_free(tmpfn1);
    g_free(tmpfn2);
    ck_assert_int_ge(fd1, 0);
    ck_assert_int_ge(fd2, 0);

    t1 = lr_downloadtarget_new(handle, "index.html", NULL, fd1, NULL,
                               NULL, 0, 0, NULL, NULL, NULL,
                               NULL, NULL, 0, 0, NULL, FALSE, FALSE);
    ck_assert_ptr_nonnull(t1);
    t2 = lr_downloadtarget_new(handle, "index.html", "http://seznam.cz", fd2,
                               NULL, NULL, 0, 0, NULL, NULL, NULL,
                               NULL, NULL, 0, 0, NULL, FALSE, FALSE);
    ck_assert_ptr_nonnull(t2);

    list = g_slist_append(list, t1);
    list = g_slist_append(list, t2);

    // Download

    ck_assert(lr_download(list, FALSE, &err));
    ck_assert_ptr_null(err);

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
    close(fd1);
    close(fd2);
}
END_TEST

START_TEST(test_downloader_three_files_with_error)
{
    LrHandle *handle;
    GSList *list = NULL;
    GError *err = NULL;
    int fd1, fd2, fd3;
    char *tmpfn1, *tmpfn2, *tmpfn3;
    LrDownloadTarget *t1, *t2, *t3;
    GError *tmp_err = NULL;

    // Prepare handle

    handle = lr_handle_init();
    ck_assert_ptr_nonnull(handle);

    char *urls[] = {"http://www.google.com", NULL};
    ck_assert(lr_handle_setopt(handle, NULL, LRO_URLS, urls));
    lr_handle_prepare_internal_mirrorlist(handle, FALSE, &tmp_err);
    ck_assert_ptr_null(tmp_err);

    // Prepare list of download targets

    tmpfn1 = lr_pathconcat(test_globals.tmpdir, "single_file_1_XXXXXX", NULL);
    tmpfn2 = lr_pathconcat(test_globals.tmpdir, "single_file_2_XXXXXX", NULL);
    tmpfn3 = lr_pathconcat(test_globals.tmpdir, "single_file_3_XXXXXX", NULL);

    fd1 = mkstemp(tmpfn1);
    fd2 = mkstemp(tmpfn2);
    fd3 = mkstemp(tmpfn3);
    g_free(tmpfn1);
    g_free(tmpfn2);
    g_free(tmpfn3);
    ck_assert_int_ge(fd1, 0);
    ck_assert_int_ge(fd2, 0);
    ck_assert_int_ge(fd3, 0);

    t1 = lr_downloadtarget_new(handle, "index.html", NULL, fd1, NULL, NULL,
                               0, 0, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL,
                               FALSE, FALSE);
    ck_assert_ptr_nonnull(t1);

    t2 = lr_downloadtarget_new(handle, "index.html", "http://seznam.cz", fd2,
                               NULL, NULL, 0, 0, NULL, NULL, NULL, NULL,
                               NULL, 0, 0, NULL, FALSE, FALSE);
    ck_assert_ptr_nonnull(t2);

    t3 = lr_downloadtarget_new(handle, "i_hope_this_page_doesnt_exists.html",
                               "http://google.com", fd3, NULL, NULL,
                               0, 0, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL,
                               FALSE, FALSE);
    ck_assert_ptr_nonnull(t3);

    list = g_slist_append(list, t1);
    list = g_slist_append(list, t2);
    list = g_slist_append(list, t3);

    // Download

    ck_assert(lr_download(list, FALSE, &err));
    ck_assert_ptr_null(err);

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
                printf("No 404 error raised!\n");
                ck_abort();
            }
    }

    g_slist_free_full(list, (GDestroyNotify) lr_downloadtarget_free);
    close(fd1);
    close(fd2);
    close(fd3);
}
END_TEST

START_TEST(test_downloader_checksum)
{
    const struct {
        const char *sha512;
        int expect_err;
    } tests[] = {
        {
            "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e",
            0,
        },
        {
            "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            1,
        },
        {
            NULL
        }
    };
    int i;

    for (i = 0; tests[i].sha512; i++) {
        LrHandle *handle;
        GSList *list = NULL;
        GError *err = NULL;
        int fd1;
        char *tmpfn1;
        LrDownloadTargetChecksum *checksum;
        GSList *checksums = NULL;
        LrDownloadTarget *t1;
        GError *tmp_err = NULL;

        // Prepare handle

        handle = lr_handle_init();
        ck_assert_ptr_nonnull(handle);

        char *urls[] = {"file:///", NULL};
        ck_assert(lr_handle_setopt(handle, NULL, LRO_URLS, urls));
        lr_handle_prepare_internal_mirrorlist(handle, FALSE, &tmp_err);
        ck_assert_ptr_null(tmp_err);


        // Prepare list of download targets

        tmpfn1 = lr_pathconcat(test_globals.tmpdir, "single_file_XXXXXX", NULL);

        fd1 = mkstemp(tmpfn1);
        g_free(tmpfn1);
        ck_assert_int_ge(fd1, 0);

        checksum = lr_downloadtargetchecksum_new(LR_CHECKSUM_SHA512,
                                                 tests[i].sha512);
        checksums = g_slist_append(checksums, checksum);

        t1 = lr_downloadtarget_new(handle, "dev/null", NULL, fd1, NULL, checksums,
                                   0, 0, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL,
                                   FALSE, FALSE);
        ck_assert_ptr_nonnull(t1);

        list = g_slist_append(list, t1);

        // Download

        ck_assert(lr_download(list, FALSE, &err));
        ck_assert_ptr_null(err);

        lr_handle_free(handle);

        // Check results

        for (GSList *elem = list; elem; elem = g_slist_next(elem)) {
                LrDownloadTarget *dtarget = elem->data;
                if (!tests[i].expect_err) {
                    if (dtarget->err) {
                        printf("Error msg: %s\n", dtarget->err);
                        ck_abort();
                    }
                } else {
                    if (!dtarget->err) {
                        printf("No checksum error raised!\n");
                        ck_abort();
                    }
                }
        }

        g_slist_free_full(list, (GDestroyNotify) lr_downloadtarget_free);
        close(fd1);
    }
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
    tcase_add_test(tc, test_downloader_checksum);
    suite_add_tcase(s, tc);
    return s;
}
