#define _GNU_SOURCE
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

/* Answer every connection with the response until killed */
static void
http_server(int sock, const char *response)
{
    for (;;) {
        char buf[4096];
        int conn = accept(sock, NULL, NULL);
        if (conn < 0)
            continue;
        ssize_t r = read(conn, buf, sizeof(buf));  // The request is ignored
        r = write(conn, response, strlen(response));
        (void) r;
        close(conn);
    }
}

/* Header names are case-insensitive (HTTP/2 servers send them in lower
 * case) and there doesn't have to be a space after the colon. Such
 * Content-Length must be compared with the expected size too. */
START_TEST(test_downloader_content_length_header)
{
    const char *response = "HTTP/1.1 200 OK\r\n"
                           "content-length:11\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "hello world";
    const gint64 expected_sizes[] = {1000, 11};  // A wrong and the right size
    struct sockaddr_in addr = {0};
    socklen_t addr_len = sizeof(addr);

    // Listen on a free port of the loopback interface
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ck_assert_int_eq(bind(sock, (struct sockaddr *) &addr, sizeof(addr)), 0);
    ck_assert_int_eq(listen(sock, 8), 0);
    ck_assert_int_eq(getsockname(sock, (struct sockaddr *) &addr, &addr_len), 0);

    pid_t server = fork();
    ck_assert_int_ge(server, 0);
    if (server == 0)
        http_server(sock, response);
    close(sock);

    // Don't let a proxy from the environment handle the transfer
    unsetenv("http_proxy");
    unsetenv("all_proxy");

    gchar *url = g_strdup_printf("http://127.0.0.1:%d/file", ntohs(addr.sin_port));
    char *fn = lr_pathconcat(test_globals.tmpdir, "content_length_header", NULL);
    for (int i = 0; i < 2; i++) {
        GError *err = NULL;
        LrDownloadTarget *t = lr_downloadtarget_new(NULL, url, NULL, -1, fn, NULL,
                                                    expected_sizes[i], FALSE,
                                                    NULL, NULL, NULL, NULL, NULL,
                                                    0, 0, NULL, FALSE, FALSE);
        GSList *list = g_slist_append(NULL, t);
        ck_assert(lr_download(list, FALSE, &err));
        ck_assert_ptr_null(err);
        if (expected_sizes[i] != 11)
            ck_assert_msg(t->err && strstr(t->err, "Inconsistent server data"),
                          "Size mismatch not detected: %s",
                          t->err ? t->err : "(no error)");
        else
            ck_assert_msg(t->err == NULL, "Unexpected error: %s", t->err);
        g_slist_free_full(list, (GDestroyNotify) lr_downloadtarget_free);
    }

    // SIGKILL: the server inherited check's SIGTERM handler, which would
    // forward the signal to the whole process group
    kill(server, SIGKILL);
    waitpid(server, NULL, 0);
    lr_free(fn);
    g_free(url);
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

/* Tests that don't need an internet connection */
Suite *
downloader_local_suite(void)
{
    Suite *s = suite_create("downloader_local");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_downloader_content_length_header);
    suite_add_tcase(s, tc);
    return s;
}
