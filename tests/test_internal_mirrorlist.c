#include "testsys.h"
#include "test_internal_mirrorlist.h"
#include "librepo/internal_mirrorlist.h"

START_TEST(test_internalimirrorlist_append_mirrorlist)
{
    lr_InternalMirrorlist iml = NULL;
    lr_InternalMirror mirror = NULL;
    char *url = NULL;
    struct _lr_Mirrorlist ml = {
        .urls = (char*[2]) {"http://foo", "ftp://bar"},
        .nou = 2,
        .lou = 2,
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

START_TEST(test_internalimirrorlist_append_metalink)
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
        .urls = (lr_MetalinkUrl[2]) {
            (lr_MetalinkUrl) &url1,
            (lr_MetalinkUrl) &url2
        },
        .noh = 0,
        .nou = 2,
        .loh = 0,
        .lou = 2,
    };

    iml = lr_internalmirrorlist_new();
    fail_if(lr_internalmirrorlist_len(iml) != 0);
    lr_internalmirrorlist_append_metalink(iml, NULL, NULL);
    fail_if(lr_internalmirrorlist_len(iml) != 0);

    lr_internalmirrorlist_append_metalink(iml, &ml, "/repodata/repomd.xml");
    fail_if(iml->nom != 2);
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

Suite *
internal_mirrorlist_suite(void)
{
    Suite *s = suite_create("internal_mirrorlist");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_internalimirrorlist_append_mirrorlist);
    tcase_add_test(tc, test_internalimirrorlist_append_metalink);
    suite_add_tcase(s, tc);
    return s;
}
