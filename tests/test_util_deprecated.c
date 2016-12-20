#define _GNU_SOURCE
#include "librepo/util.h"

#include "fixtures.h"
#include "testsys.h"
#include "test_util.h"


#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

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

    url = lr_prepend_url_protocol("..");
    fail_if(!g_str_has_prefix(url, "file:/"));

    url = lr_prepend_url_protocol("relative/path/does/not/exist");
    fail_if(url != NULL);
}
END_TEST

Suite *
util_deprecated_suite(void)
{
    Suite *s = suite_create("util_deprecated");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_prepend_url_protocol);
    suite_add_tcase(s, tc);
    return s;
}
