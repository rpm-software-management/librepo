#include "testsys.h"
#include "test_lrmirrorlist.h"
#include "librepo/lrmirrorlist.h"

START_TEST(test_lrmirrorlist_append_url)
{
    LrInternalMirrorlist *iml = NULL;
    LrInternalMirror *mirror = NULL;
    LrUrlVars *vars = NULL;

    gchar *url1 = g_strdup("ftp://bar");
    gchar *url2 = g_strdup("http://foo");
    gchar *url3 = g_strdup("http://xyz/$arch/");
    gchar *var = g_strdup("arch");
    gchar *val = g_strdup("i386");

    vars = lr_urlvars_set(vars, var, val);

    g_free(var);
    g_free(val);

    iml = lr_lrmirrorlist_append_url(iml, url1, NULL);
    iml = lr_lrmirrorlist_append_url(iml, url2, NULL);
    iml = lr_lrmirrorlist_append_url(iml, url3, vars);

    g_free(url1);
    g_free(url2);
    g_free(url3);
    lr_urlvars_free(vars);

    mirror = lr_lrmirrorlist_nth(iml, 0);
    ck_assert_ptr_nonnull(mirror);
    ck_assert_str_eq(mirror->url, "ftp://bar");

    mirror = lr_lrmirrorlist_nth(iml, 1);
    ck_assert_ptr_nonnull(mirror);
    ck_assert_str_eq(mirror->url, "http://foo");

    mirror = lr_lrmirrorlist_nth(iml, 2);
    ck_assert_ptr_nonnull(mirror);
    ck_assert_str_eq(mirror->url, "http://xyz/i386/");

    lr_lrmirrorlist_free(iml);
}
END_TEST

START_TEST(test_lrmirrorlist_append_mirrorlist)
{
    LrInternalMirrorlist *iml = NULL;
    LrInternalMirror *mirror = NULL;
    char *url = NULL;
    LrMirrorlist ml = {
        .urls = NULL,
    };

    ml.urls = g_slist_prepend(ml.urls, "ftp://bar");
    ml.urls = g_slist_prepend(ml.urls, "http://foo");

    ck_assert(g_slist_length(iml) == 0);
    iml = lr_lrmirrorlist_append_mirrorlist(iml, NULL, NULL);
    ck_assert(g_slist_length(iml) == 0);

    iml = lr_lrmirrorlist_append_mirrorlist(iml, &ml, NULL);
    ck_assert(g_slist_length(iml) == 2);
    mirror = lr_lrmirrorlist_nth(iml, 0);
    ck_assert_ptr_nonnull(mirror);
    ck_assert_str_eq(mirror->url, "http://foo");
    ck_assert(mirror->preference == 100);
    ck_assert(mirror->protocol == LR_PROTOCOL_HTTP);
    mirror = lr_lrmirrorlist_nth(iml, 1);
    ck_assert_ptr_nonnull(mirror);
    ck_assert_str_eq(mirror->url, "ftp://bar");
    ck_assert(mirror->preference == 100);
    ck_assert(mirror->protocol == LR_PROTOCOL_FTP);

    ck_assert(g_slist_length(iml) == 2);

    url = lr_lrmirrorlist_nth_url(iml, 0);
    ck_assert_str_eq(url, "http://foo");

    url = lr_lrmirrorlist_nth_url(iml, 1);
    ck_assert_str_eq(url, "ftp://bar");

    lr_lrmirrorlist_free(iml);
    g_slist_free(ml.urls);
}
END_TEST

START_TEST(test_lrmirrorlist_append_metalink)
{
    LrInternalMirrorlist *iml = NULL;
    LrInternalMirror *mirror = NULL;
    char *url = NULL;
    LrMetalinkUrl url1 = {
            .protocol = "http",
            .type = "http",
            .location = "CZ",
            .preference = 100,
            .url = "http://foo/repodata/repomd.xml",
        };
    LrMetalinkUrl url2 = {
            .protocol = "rsync",
            .type = "rsync",
            .location = "US",
            .preference = 50,
            .url = "",
        };
    LrMetalinkUrl url3 = {
            .protocol = "ftp",
            .type = "ftp",
            .location = "CZ",
            .preference = 1,
            .url = NULL,
        };
    LrMetalinkUrl url4 = {
            .protocol = "ftp",
            .type = "ftp",
            .location = "US",
            .preference = 95,
            .url = "ftp://bar/repodata/repomd.xml",
        };
    LrMetalink ml = {
        .filename = NULL,
        .timestamp = 1,
        .size = 1,
        .hashes = NULL,
        .urls = NULL,
    };

    ml.urls = g_slist_prepend(ml.urls, &url4);
    ml.urls = g_slist_prepend(ml.urls, &url3);
    ml.urls = g_slist_prepend(ml.urls, &url2);
    ml.urls = g_slist_prepend(ml.urls, &url1);

    ck_assert(g_slist_length(iml) == 0);
    iml = lr_lrmirrorlist_append_metalink(iml, NULL, NULL, NULL);
    ck_assert(g_slist_length(iml) == 0);

    iml = lr_lrmirrorlist_append_metalink(iml, &ml, "/repodata/repomd.xml", NULL);
    ck_assert(g_slist_length(iml) == 2);  // 2 because element with empty url shoud be skipped

    mirror = lr_lrmirrorlist_nth(iml, 0);
    ck_assert_str_eq(mirror->url, "http://foo");
    ck_assert(mirror->preference == 100);
    ck_assert(mirror->protocol == LR_PROTOCOL_HTTP);

    mirror = lr_lrmirrorlist_nth(iml, 1);
    ck_assert_str_eq(mirror->url, "ftp://bar");
    ck_assert(mirror->preference == 95);
    ck_assert(mirror->protocol == LR_PROTOCOL_FTP);

    ck_assert(g_slist_length(iml) == 2);

    url = lr_lrmirrorlist_nth_url(iml, 0);
    ck_assert_str_eq(url, "http://foo");

    url = lr_lrmirrorlist_nth_url(iml, 1);
    ck_assert_str_eq(url, "ftp://bar");

    lr_lrmirrorlist_free(iml);

    // Try append on list with existing element
    iml = NULL;
    ck_assert(g_slist_length(iml) == 0);
    iml = lr_lrmirrorlist_append_url(iml, "http://abc", NULL);
    ck_assert(g_slist_length(iml) == 1);
    iml = lr_lrmirrorlist_append_metalink(iml, &ml, "/repodata/repomd.xml", NULL);
    ck_assert(g_slist_length(iml) == 3);
    url = lr_lrmirrorlist_nth_url(iml, 0);
    ck_assert_str_eq(url, "http://abc");
    url = lr_lrmirrorlist_nth_url(iml, 1);
    ck_assert_str_eq(url, "http://foo");
    url = lr_lrmirrorlist_nth_url(iml, 2);
    ck_assert_str_eq(url, "ftp://bar");

    lr_lrmirrorlist_free(iml);
    g_slist_free(ml.urls);
}
END_TEST

START_TEST(test_lrmirrorlist_append_lrmirrorlist)
{
    LrInternalMirrorlist *iml = NULL, *iml_2 = NULL;
    LrInternalMirror *mirror = NULL;
    char *url = NULL;

    iml_2 = lr_lrmirrorlist_append_url(iml_2, "http://foo", NULL);
    iml_2 = lr_lrmirrorlist_append_url(iml_2, "", NULL);
    iml_2 = lr_lrmirrorlist_append_url(iml_2, NULL, NULL);
    iml_2 = lr_lrmirrorlist_append_url(iml_2, "ftp://bar", NULL);

    ck_assert(g_slist_length(iml_2) == 2);

    ck_assert(g_slist_length(iml) == 0);
    iml = lr_lrmirrorlist_append_lrmirrorlist(iml, NULL);
    ck_assert(g_slist_length(iml) == 0);

    iml = lr_lrmirrorlist_append_lrmirrorlist(iml, iml_2);
    ck_assert(g_slist_length(iml) == 2);  // 2 because element with empty url shoud be skipped

    mirror = lr_lrmirrorlist_nth(iml, 0);
    ck_assert_str_eq(mirror->url, "http://foo");
    ck_assert(mirror->preference == 100);
    ck_assert(mirror->protocol == LR_PROTOCOL_HTTP);

    mirror = lr_lrmirrorlist_nth(iml, 1);
    ck_assert_str_eq(mirror->url, "ftp://bar");
    ck_assert(mirror->preference == 100);
    ck_assert(mirror->protocol == LR_PROTOCOL_FTP);

    ck_assert(g_slist_length(iml) == 2);

    url = lr_lrmirrorlist_nth_url(iml, 0);
    ck_assert_str_eq(url, "http://foo");

    url = lr_lrmirrorlist_nth_url(iml, 1);
    ck_assert_str_eq(url, "ftp://bar");

    lr_lrmirrorlist_free(iml);

    // Try append on list with existing element
    iml = NULL;
    ck_assert(g_slist_length(iml) == 0);
    iml = lr_lrmirrorlist_append_url(iml, "http://abc", NULL);
    ck_assert(g_slist_length(iml) == 1);
    iml = lr_lrmirrorlist_append_lrmirrorlist(iml, iml_2);
    ck_assert(g_slist_length(iml) == 3);
    url = lr_lrmirrorlist_nth_url(iml, 0);
    ck_assert_str_eq(url, "http://abc");
    url = lr_lrmirrorlist_nth_url(iml, 1);
    ck_assert_str_eq(url, "http://foo");
    url = lr_lrmirrorlist_nth_url(iml, 2);
    ck_assert_str_eq(url, "ftp://bar");
    lr_lrmirrorlist_free(iml);
    lr_lrmirrorlist_free(iml_2);
}
END_TEST

Suite *
lrmirrorlist_suite(void)
{
    Suite *s = suite_create("internal_mirrorlist");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_lrmirrorlist_append_url);
    tcase_add_test(tc, test_lrmirrorlist_append_mirrorlist);
    tcase_add_test(tc, test_lrmirrorlist_append_metalink);
    tcase_add_test(tc, test_lrmirrorlist_append_lrmirrorlist);
    suite_add_tcase(s, tc);
    return s;
}
