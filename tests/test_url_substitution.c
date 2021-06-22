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
#include "librepo/url_substitution.h"

#include "fixtures.h"
#include "testsys.h"
#include "test_url_substitution.h"

START_TEST(test_urlvars_set)
{
    LrUrlVars *urlvars = NULL;

    urlvars = lr_urlvars_set(urlvars, "foo", "bar");
    ck_assert_ptr_nonnull(urlvars);
    ck_assert_str_eq(((LrVar *)urlvars->data)->var, "foo");

    urlvars = lr_urlvars_set(urlvars, "foo1", "bar1");
    ck_assert_ptr_nonnull(urlvars);

    urlvars = lr_urlvars_set(urlvars, "foo", NULL);
    ck_assert_ptr_nonnull(urlvars);
    ck_assert_str_eq(((LrVar *)urlvars->data)->var, "foo1");

    urlvars = lr_urlvars_set(urlvars, "foo1", NULL);
    ck_assert_ptr_null(urlvars);

    urlvars = lr_urlvars_set(urlvars, "bar", "foo");
    ck_assert_ptr_nonnull(urlvars);
    ck_assert_str_eq(((LrVar *)urlvars->data)->var, "bar");

    lr_urlvars_free(urlvars);
}
END_TEST

START_TEST(test_url_substitute_without_urlvars)
{
    char *url;
    LrUrlVars *urlvars = NULL;

    urlvars = lr_urlvars_set(urlvars, "foo", "bar");

    url = lr_url_substitute("", urlvars);
    ck_assert_str_eq(url, "");
    lr_free(url);

    url = lr_url_substitute("http://foo", urlvars);
    ck_assert_str_eq(url, "http://foo");
    lr_free(url);

    url = lr_url_substitute("http://foo?id=$bar", urlvars);
    ck_assert_str_eq(url, "http://foo?id=$bar");
    lr_free(url);

    url = lr_url_substitute("http://foo?id=$foox", urlvars);
    ck_assert_str_eq(url, "http://foo?id=$foox");
    lr_free(url);

    lr_urlvars_free(urlvars);
}
END_TEST

START_TEST(test_url_substitute)
{
    char *url;
    LrUrlVars *urlvars = NULL;

    urlvars = lr_urlvars_set(urlvars, "foo", "version");
    urlvars = lr_urlvars_set(urlvars, "fo", "ver");
    urlvars = lr_urlvars_set(urlvars, "bar", "repo");

    url = lr_url_substitute("", urlvars);
    ck_assert_str_eq(url, "");
    lr_free(url);

    url = lr_url_substitute("http://foo", urlvars);
    ck_assert_str_eq(url, "http://foo");
    lr_free(url);

    url = lr_url_substitute("http://foo?id=$bar", urlvars);
    ck_assert_str_eq(url, "http://foo?id=repo");
    lr_free(url);

    url = lr_url_substitute("http://$foo?id=$bar", urlvars);
    ck_assert_str_eq(url, "http://version?id=repo");
    lr_free(url);

    url = lr_url_substitute("http://$fo?id=$bar", urlvars);
    ck_assert_str_eq(url, "http://ver?id=repo");
    lr_free(url);

    url = lr_url_substitute("http://$foo$bar", urlvars);
    ck_assert_str_eq(url, "http://versionrepo");
    lr_free(url);

    url = lr_url_substitute("http://$foo$bar/", urlvars);
    ck_assert_str_eq(url, "http://versionrepo/");
    lr_free(url);

    lr_urlvars_free(urlvars);
}
END_TEST

START_TEST(test_url_substitute_braces)
{
    char *url;
    LrUrlVars *urlvars = NULL;

    urlvars = lr_urlvars_set(urlvars, "foo", "version");
    urlvars = lr_urlvars_set(urlvars, "fo", "ver");
    urlvars = lr_urlvars_set(urlvars, "bar", "repo");

    url = lr_url_substitute("http://foo?id=${bar}", urlvars);
    ck_assert_str_eq(url, "http://foo?id=repo");
    lr_free(url);

    url = lr_url_substitute("http://${foo}?id=${bar}", urlvars);
    ck_assert_str_eq(url, "http://version?id=repo");
    lr_free(url);

    url = lr_url_substitute("http://${fo}?id=$bar", urlvars);
    ck_assert_str_eq(url, "http://ver?id=repo");
    lr_free(url);

    url = lr_url_substitute("http://${fo?id=$bar", urlvars);
    ck_assert_str_eq(url, "http://${fo?id=repo");
    lr_free(url);

    url = lr_url_substitute("http://${foo${bar}", urlvars);
    ck_assert_str_eq(url, "http://${foorepo");
    lr_free(url);

    url = lr_url_substitute("http://${foo}${bar}/", urlvars);
    ck_assert_str_eq(url, "http://versionrepo/");
    lr_free(url);

    lr_urlvars_free(urlvars);
}
END_TEST

Suite *
url_substitution_suite(void)
{
    Suite *s = suite_create("url_substitution");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_urlvars_set);
    tcase_add_test(tc, test_url_substitute_without_urlvars);
    tcase_add_test(tc, test_url_substitute);
    tcase_add_test(tc, test_url_substitute_braces);
    suite_add_tcase(s, tc);
    return s;
}
