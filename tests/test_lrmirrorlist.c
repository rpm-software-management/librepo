#include "testsys.h"
#include "test_lrmirrorlist.h"
#include "librepo/lrmirrorlist.h"

START_TEST(test_lrmirrorlist_append_mirrorlist)
{
    lr_LrMirrorlist *iml = NULL;
    lr_LrMirror *mirror = NULL;
    char *url = NULL;
    lr_Mirrorlist ml = {
        .urls = NULL,
    };

    ml.urls = g_slist_prepend(ml.urls, "ftp://bar");
    ml.urls = g_slist_prepend(ml.urls, "http://foo");

    fail_if(g_slist_length(iml) != 0);
    iml = lr_lrmirrorlist_append_mirrorlist(iml, NULL, NULL);
    fail_if(g_slist_length(iml) != 0);

    iml = lr_lrmirrorlist_append_mirrorlist(iml, &ml, NULL);
    fail_if(g_slist_length(iml) != 2);
    mirror = lr_lrmirrorlist_nth(iml, 0);
    fail_if(!mirror);
    fail_if(strcmp(mirror->url, "http://foo"));
    fail_if(mirror->preference != 100);
    fail_if(mirror->fails != 0);
    mirror = lr_lrmirrorlist_nth(iml, 1);
    fail_if(!mirror);
    fail_if(strcmp(mirror->url, "ftp://bar"));
    fail_if(mirror->preference != 100);
    fail_if(mirror->fails != 0);

    fail_if(g_slist_length(iml) != 2);

    url = lr_lrmirrorlist_nth_url(iml, 0);
    fail_if(strcmp(url, "http://foo"));

    url = lr_lrmirrorlist_nth_url(iml, 1);
    fail_if(strcmp(url, "ftp://bar"));

    lr_lrmirrorlist_free(iml);
    g_slist_free(ml.urls);
}
END_TEST

START_TEST(test_lrmirrorlist_append_metalink)
{
    lr_LrMirrorlist *iml = NULL;
    lr_LrMirror *mirror = NULL;
    char *url = NULL;
    lr_MetalinkUrl url1 = {
            .protocol = "http",
            .type = "http",
            .location = "CZ",
            .preference = 100,
            .url = "http://foo/repodata/repomd.xml",
        };
    lr_MetalinkUrl url2 = {
            .protocol = "rsync",
            .type = "rsync",
            .location = "US",
            .preference = 50,
            .url = "",
        };
    lr_MetalinkUrl url3 = {
            .protocol = "ftp",
            .type = "ftp",
            .location = "CZ",
            .preference = 1,
            .url = NULL,
        };
    lr_MetalinkUrl url4 = {
            .protocol = "ftp",
            .type = "ftp",
            .location = "US",
            .preference = 95,
            .url = "ftp://bar/repodata/repomd.xml",
        };
    lr_Metalink ml = {
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

    fail_if(g_slist_length(iml) != 0);
    iml = lr_lrmirrorlist_append_metalink(iml, NULL, NULL, NULL);
    fail_if(g_slist_length(iml) != 0);

    iml = lr_lrmirrorlist_append_metalink(iml, &ml, "/repodata/repomd.xml", NULL);
    fail_if(g_slist_length(iml) != 2);  // 2 because element with empty url shoud be skipped

    mirror = lr_lrmirrorlist_nth(iml, 0);
    fail_if(strcmp(mirror->url, "http://foo"));
    fail_if(mirror->preference != 100);
    fail_if(mirror->fails != 0);

    mirror = lr_lrmirrorlist_nth(iml, 1);
    fail_if(strcmp(mirror->url, "ftp://bar"));
    fail_if(mirror->preference != 95);
    fail_if(mirror->fails != 0);

    fail_if(g_slist_length(iml) != 2);

    url = lr_lrmirrorlist_nth_url(iml, 0);
    fail_if(strcmp(url, "http://foo"));

    url = lr_lrmirrorlist_nth_url(iml, 1);
    fail_if(strcmp(url, "ftp://bar"));

    lr_lrmirrorlist_free(iml);

    // Try append on list with existing element
    iml = NULL;
    fail_if(g_slist_length(iml) != 0);
    iml = lr_lrmirrorlist_append_url(iml, "http://abc", NULL);
    fail_if(g_slist_length(iml) != 1);
    iml = lr_lrmirrorlist_append_metalink(iml, &ml, "/repodata/repomd.xml", NULL);
    fail_if(g_slist_length(iml) != 3);
    url = lr_lrmirrorlist_nth_url(iml, 0);
    fail_if(strcmp(url, "http://abc"));
    url = lr_lrmirrorlist_nth_url(iml, 1);
    fail_if(strcmp(url, "http://foo"));
    url = lr_lrmirrorlist_nth_url(iml, 2);
    fail_if(strcmp(url, "ftp://bar"));

    lr_lrmirrorlist_free(iml);
    g_slist_free(ml.urls);
}
END_TEST

START_TEST(test_lrmirrorlist_append_lrmirrorlist)
{
    lr_LrMirrorlist *iml = NULL, *iml_2 = NULL;
    lr_LrMirror *mirror = NULL;
    char *url = NULL;

    iml_2 = lr_lrmirrorlist_append_url(iml_2, "http://foo", NULL);
    iml_2 = lr_lrmirrorlist_append_url(iml_2, "", NULL);
    iml_2 = lr_lrmirrorlist_append_url(iml_2, NULL, NULL);
    iml_2 = lr_lrmirrorlist_append_url(iml_2, "ftp://bar", NULL);

    fail_if(g_slist_length(iml_2) != 2);

    fail_if(g_slist_length(iml) != 0);
    iml = lr_lrmirrorlist_append_lrmirrorlist(iml, NULL);
    fail_if(g_slist_length(iml) != 0);

    iml = lr_lrmirrorlist_append_lrmirrorlist(iml, iml_2);
    fail_if(g_slist_length(iml) != 2);  // 2 because element with empty url shoud be skipped

    mirror = lr_lrmirrorlist_nth(iml, 0);
    fail_if(strcmp(mirror->url, "http://foo"));
    fail_if(mirror->preference != 100);
    fail_if(mirror->fails != 0);

    mirror = lr_lrmirrorlist_nth(iml, 1);
    fail_if(strcmp(mirror->url, "ftp://bar"));
    fail_if(mirror->preference != 100);
    fail_if(mirror->fails != 0);

    fail_if(g_slist_length(iml) != 2);

    url = lr_lrmirrorlist_nth_url(iml, 0);
    fail_if(strcmp(url, "http://foo"));

    url = lr_lrmirrorlist_nth_url(iml, 1);
    fail_if(strcmp(url, "ftp://bar"));

    lr_lrmirrorlist_free(iml);

    // Try append on list with existing element
    iml = NULL;
    fail_if(g_slist_length(iml) != 0);
    iml = lr_lrmirrorlist_append_url(iml, "http://abc", NULL);
    fail_if(g_slist_length(iml) != 1);
    iml = lr_lrmirrorlist_append_lrmirrorlist(iml, iml_2);
    fail_if(g_slist_length(iml) != 3);
    url = lr_lrmirrorlist_nth_url(iml, 0);
    fail_if(strcmp(url, "http://abc"));
    url = lr_lrmirrorlist_nth_url(iml, 1);
    fail_if(strcmp(url, "http://foo"));
    url = lr_lrmirrorlist_nth_url(iml, 2);
    fail_if(strcmp(url, "ftp://bar"));
    lr_lrmirrorlist_free(iml);
    lr_lrmirrorlist_free(iml_2);
}
END_TEST

Suite *
lrmirrorlist_suite(void)
{
    Suite *s = suite_create("internal_mirrorlist");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_lrmirrorlist_append_mirrorlist);
    tcase_add_test(tc, test_lrmirrorlist_append_metalink);
    tcase_add_test(tc, test_lrmirrorlist_append_lrmirrorlist);
    suite_add_tcase(s, tc);
    return s;
}
