#include "testsys.h"
#include "test_internal_mirrorlist.h"
#include "librepo/internal_mirrorlist.h"

START_TEST(test_internalmirrorlist_append_mirrorlist)
{
    lr_InternalMirrorlist iml = NULL;
    lr_InternalMirror mirror = NULL;
    char *url = NULL;
    struct _lr_Mirrorlist ml = {
        .urls = (char*[4]) {"http://foo", "", NULL, "ftp://bar"},
        .nou = 4,
        .lou = 4,
    };

    iml = lr_internalmirrorlist_new();
    fail_if(lr_internalmirrorlist_len(iml) != 0);
    lr_internalmirrorlist_append_mirrorlist(iml, NULL);
    fail_if(lr_internalmirrorlist_len(iml) != 0);

    lr_internalmirrorlist_append_mirrorlist(iml, &ml);
    fail_if(iml->nom != 2);
    fail_if(strcmp(iml->mirrors[0]->url, "http://foo"));
    fail_if(iml->mirrors[0]->preference != 100);
    fail_if(iml->mirrors[0]->fails != 0);
    fail_if(strcmp(iml->mirrors[1]->url, "ftp://bar"));
    fail_if(iml->mirrors[1]->preference != 100);
    fail_if(iml->mirrors[1]->fails != 0);

    fail_if(lr_internalmirrorlist_len(iml) != 2);

    mirror = lr_internalmirrorlist_get(iml, 0);
    fail_if(mirror == NULL);
    fail_if(strcmp(mirror->url, "http://foo"));
    url = lr_internalmirrorlist_get_url(iml, 0);
    fail_if(strcmp(url, "http://foo"));

    mirror = lr_internalmirrorlist_get(iml, 1);
    fail_if(mirror == NULL);
    fail_if(strcmp(mirror->url, "ftp://bar"));
    url = lr_internalmirrorlist_get_url(iml, 1);
    fail_if(strcmp(url, "ftp://bar"));

    lr_internalmirrorlist_free(iml);
}
END_TEST

START_TEST(test_internalmirrorlist_append_metalink)
{
    lr_InternalMirrorlist iml = NULL;
    lr_InternalMirror mirror = NULL;
    char *url = NULL;
    struct _lr_MetalinkUrl url1 = {
            .protocol = "http",
            .type = "http",
            .location = "CZ",
            .preference = 100,
            .url = "http://foo/repodata/repomd.xml",
        };
    struct _lr_MetalinkUrl url2 = {
            .protocol = "rsync",
            .type = "rsync",
            .location = "US",
            .preference = 50,
            .url = "",
        };
    struct _lr_MetalinkUrl url3 = {
            .protocol = "ftp",
            .type = "ftp",
            .location = "CZ",
            .preference = 1,
            .url = NULL,
        };
    struct _lr_MetalinkUrl url4 = {
            .protocol = "ftp",
            .type = "ftp",
            .location = "US",
            .preference = 95,
            .url = "ftp://bar/repodata/repomd.xml",
        };
    struct _lr_Metalink ml = {
        .filename = NULL,
        .timestamp = 1,
        .size = 1,
        .hashes = NULL,
        .urls = (lr_MetalinkUrl[4]) {
            (lr_MetalinkUrl) &url1,
            (lr_MetalinkUrl) &url2,
            (lr_MetalinkUrl) &url3,
            (lr_MetalinkUrl) &url4
        },
        .noh = 0,
        .nou = 4,
        .loh = 0,
        .lou = 4,
    };

    iml = lr_internalmirrorlist_new();
    fail_if(lr_internalmirrorlist_len(iml) != 0);
    lr_internalmirrorlist_append_metalink(iml, NULL, NULL);
    fail_if(lr_internalmirrorlist_len(iml) != 0);

    lr_internalmirrorlist_append_metalink(iml, &ml, "/repodata/repomd.xml");
    fail_if(iml->nom != 2);  // 2 because element with empty url shoud be skipped
    fail_if(strcmp(iml->mirrors[0]->url, "http://foo"));
    fail_if(iml->mirrors[0]->preference != 100);
    fail_if(iml->mirrors[0]->fails != 0);
    fail_if(strcmp(iml->mirrors[1]->url, "ftp://bar"));
    fail_if(iml->mirrors[1]->preference != 95);
    fail_if(iml->mirrors[1]->fails != 0);

    fail_if(lr_internalmirrorlist_len(iml) != 2);

    mirror = lr_internalmirrorlist_get(iml, 0);
    fail_if(mirror == NULL);
    fail_if(strcmp(mirror->url, "http://foo"));
    url = lr_internalmirrorlist_get_url(iml, 0);
    fail_if(strcmp(url, "http://foo"));

    mirror = lr_internalmirrorlist_get(iml, 1);
    fail_if(mirror == NULL);
    fail_if(strcmp(mirror->url, "ftp://bar"));
    url = lr_internalmirrorlist_get_url(iml, 1);
    fail_if(strcmp(url, "ftp://bar"));

    lr_internalmirrorlist_free(iml);

    // Try append on list with existing element
    iml = lr_internalmirrorlist_new();
    fail_if(lr_internalmirrorlist_len(iml) != 0);
    lr_internalmirrorlist_append_url(iml, "http://abc");
    fail_if(lr_internalmirrorlist_len(iml) != 1);
    lr_internalmirrorlist_append_metalink(iml, &ml, "/repodata/repomd.xml");
    fail_if(lr_internalmirrorlist_len(iml) != 3);
    url = lr_internalmirrorlist_get_url(iml, 0);
    fail_if(strcmp(url, "http://abc"));
    url = lr_internalmirrorlist_get_url(iml, 1);
    fail_if(strcmp(url, "http://foo"));
    url = lr_internalmirrorlist_get_url(iml, 2);
    fail_if(strcmp(url, "ftp://bar"));
    lr_internalmirrorlist_free(iml);
}
END_TEST

START_TEST(test_internalmirrorlist_append_internalmirrorlist)
{
    lr_InternalMirrorlist iml = NULL;
    lr_InternalMirror mirror = NULL;
    char *url = NULL;
    struct _lr_InternalMirror url1 = {
            .url = "http://foo",
            .preference = 100,
            .fails = 0,
        };
    struct _lr_InternalMirror url2 = {
            .url = "",
            .preference = 50,
            .fails = 0,
        };
    struct _lr_InternalMirror url3 = {
            .url = NULL,
            .preference = 1,
            .fails = 0,
        };
    struct _lr_InternalMirror url4 = {
            .url = "ftp://bar",
            .preference = 95,
            .fails = 0,
        };
    struct _lr_InternalMirrorlist ml = {
        .mirrors = (lr_InternalMirror[4]) {
            (lr_InternalMirror) &url1,
            (lr_InternalMirror) &url2,
            (lr_InternalMirror) &url3,
            (lr_InternalMirror) &url4
        },
        .nom = 4,
    };

    iml = lr_internalmirrorlist_new();
    fail_if(lr_internalmirrorlist_len(iml) != 0);
    lr_internalmirrorlist_append_internalmirrorlist(iml, NULL);
    fail_if(lr_internalmirrorlist_len(iml) != 0);

    lr_internalmirrorlist_append_internalmirrorlist(iml, &ml);
    fail_if(iml->nom != 2);  // 2 because element with empty url shoud be skipped
    fail_if(strcmp(iml->mirrors[0]->url, "http://foo"));
    fail_if(iml->mirrors[0]->preference != 100);
    fail_if(iml->mirrors[0]->fails != 0);
    fail_if(strcmp(iml->mirrors[1]->url, "ftp://bar"));
    fail_if(iml->mirrors[1]->preference != 95);
    fail_if(iml->mirrors[1]->fails != 0);

    fail_if(lr_internalmirrorlist_len(iml) != 2);

    mirror = lr_internalmirrorlist_get(iml, 0);
    fail_if(mirror == NULL);
    fail_if(strcmp(mirror->url, "http://foo"));
    url = lr_internalmirrorlist_get_url(iml, 0);
    fail_if(strcmp(url, "http://foo"));

    mirror = lr_internalmirrorlist_get(iml, 1);
    fail_if(mirror == NULL);
    fail_if(strcmp(mirror->url, "ftp://bar"));
    url = lr_internalmirrorlist_get_url(iml, 1);
    fail_if(strcmp(url, "ftp://bar"));

    lr_internalmirrorlist_free(iml);

    // Try append on list with existing element
    iml = lr_internalmirrorlist_new();
    fail_if(lr_internalmirrorlist_len(iml) != 0);
    lr_internalmirrorlist_append_url(iml, "http://abc");
    fail_if(lr_internalmirrorlist_len(iml) != 1);
    lr_internalmirrorlist_append_internalmirrorlist(iml, &ml);
    fail_if(lr_internalmirrorlist_len(iml) != 3);
    url = lr_internalmirrorlist_get_url(iml, 0);
    fail_if(strcmp(url, "http://abc"));
    url = lr_internalmirrorlist_get_url(iml, 1);
    fail_if(strcmp(url, "http://foo"));
    url = lr_internalmirrorlist_get_url(iml, 2);
    fail_if(strcmp(url, "ftp://bar"));
    lr_internalmirrorlist_free(iml);

}
END_TEST

Suite *
internal_mirrorlist_suite(void)
{
    Suite *s = suite_create("internal_mirrorlist");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_internalmirrorlist_append_mirrorlist);
    tcase_add_test(tc, test_internalmirrorlist_append_metalink);
    tcase_add_test(tc, test_internalmirrorlist_append_internalmirrorlist);
    suite_add_tcase(s, tc);
    return s;
}
