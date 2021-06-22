#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "librepo/librepo.h"
#include "librepo/rcodes.h"
#include "librepo/handle.h"
#include "librepo/url_substitution.h"

#include "fixtures.h"
#include "testsys.h"
#include "test_handle.h"

START_TEST(test_handle)
{
    LrHandle *h = NULL;
    GError *tmp_err = NULL;

    h = lr_handle_init();
    ck_assert_ptr_nonnull(h);
    lr_handle_free(h);
    h = NULL;

    /* This test is meant to check memory leaks. (Use valgrind) */
    h = lr_handle_init();
    char *urls[] = {"foo", NULL};
    ck_assert(lr_handle_setopt(h, &tmp_err, LRO_URLS, urls));
    ck_assert_ptr_null(tmp_err);
    ck_assert(lr_handle_setopt(h, &tmp_err, LRO_URLS, urls));
    ck_assert_ptr_null(tmp_err);
    ck_assert(lr_handle_setopt(h, &tmp_err, LRO_MIRRORLIST, "foo"));
    ck_assert_ptr_null(tmp_err);
    ck_assert(lr_handle_setopt(h, &tmp_err, LRO_MIRRORLIST, "bar"));
    ck_assert_ptr_null(tmp_err);
    ck_assert(lr_handle_setopt(h, NULL, LRO_USERPWD, "user:pwd"));
    ck_assert(lr_handle_setopt(h, NULL, LRO_PROXY, "proxy"));
    ck_assert(lr_handle_setopt(h, NULL, LRO_PROXYUSERPWD, "proxyuser:pwd"));
    ck_assert(lr_handle_setopt(h, NULL, LRO_DESTDIR, "foodir"));
    ck_assert(lr_handle_setopt(h, NULL, LRO_USERAGENT, "librepo/0.0"));
    char *dlist[] = {"primary", "filelists", NULL};
    ck_assert(lr_handle_setopt(h, NULL, LRO_YUMDLIST, dlist));
    ck_assert(lr_handle_setopt(h, NULL, LRO_YUMBLIST, dlist));
    LrUrlVars *vars = NULL;
    vars = lr_urlvars_set(vars, "foo", "bar");
    ck_assert(lr_handle_setopt(h, NULL, LRO_VARSUB, vars));
    ck_assert(lr_handle_setopt(h, NULL, LRO_FASTESTMIRRORCACHE,
                              "/var/cache/fastestmirror.librepo"));
    ck_assert(lr_handle_setopt(h, NULL, LRO_SSLCLIENTCERT, "/etc/cert.pem"));
    ck_assert(lr_handle_setopt(h, NULL, LRO_SSLCLIENTKEY, "/etc/cert.key"));
    ck_assert(lr_handle_setopt(h, NULL, LRO_SSLCACERT, "/etc/ca.pem"));
    ck_assert(lr_handle_setopt(h, NULL, LRO_PROXY_SSLCLIENTCERT, "/etc/proxy_cert.pem"));
    ck_assert(lr_handle_setopt(h, NULL, LRO_PROXY_SSLCLIENTKEY, "/etc/proxy_cert.key"));
    ck_assert(lr_handle_setopt(h, NULL, LRO_PROXY_SSLCACERT, "/etc/proxy_ca.pem"));
    ck_assert(lr_handle_setopt(h, NULL, LRO_HTTPAUTHMETHODS, LR_AUTH_NTLM));
    ck_assert(lr_handle_setopt(h, NULL, LRO_PROXYAUTHMETHODS, LR_AUTH_DIGEST));
    lr_handle_free(h);
}
END_TEST

START_TEST(test_handle_getinfo)
{
    long num;
    char *str;
    char **strlist;
    LrHandle *h = NULL;
    GError *tmp_err = NULL;

    h = lr_handle_init();

    num = -1;
    ck_assert(lr_handle_getinfo(h, &tmp_err, LRI_UPDATE, &num));
    ck_assert(num == 0);
    ck_assert_ptr_null(tmp_err);

    strlist = NULL;
    ck_assert(lr_handle_getinfo(h, &tmp_err, LRI_URLS, &strlist));
    ck_assert_ptr_null(strlist);
    ck_assert_ptr_null(tmp_err);

    str = NULL;
    ck_assert(lr_handle_getinfo(h, NULL, LRI_MIRRORLIST, &str));
    ck_assert_ptr_null(str);

    num = -1;
    ck_assert(lr_handle_getinfo(h, NULL, LRI_LOCAL, &num));
    ck_assert(num == 0);

    str = NULL;
    ck_assert(lr_handle_getinfo(h, NULL, LRI_DESTDIR, &str));
    ck_assert_ptr_null(str);

    num = -1;
    ck_assert(lr_handle_getinfo(h, NULL, LRI_REPOTYPE, &num));
    ck_assert(num == 0);

    str = NULL;
    ck_assert(lr_handle_getinfo(h, NULL, LRI_USERAGENT, &str));
    ck_assert_ptr_null(str);

    strlist = NULL;
    ck_assert(lr_handle_getinfo(h, NULL, LRI_YUMDLIST, &strlist));
    ck_assert_ptr_null(strlist);

    strlist = NULL;
    ck_assert(lr_handle_getinfo(h, NULL, LRI_YUMBLIST, &strlist));
    ck_assert_ptr_null(strlist);

    num = -1;
    ck_assert(lr_handle_getinfo(h, NULL, LRI_MAXMIRRORTRIES, &num));
    ck_assert(num == 0);

    LrUrlVars *vars = NULL;
    ck_assert(lr_handle_getinfo(h, NULL, LRI_VARSUB, &vars));
    ck_assert_ptr_null(strlist);

    str = NULL;
    ck_assert(lr_handle_getinfo(h, NULL, LRI_FASTESTMIRRORCACHE, &str));
    ck_assert_ptr_null(str);

    num = -1;
    ck_assert(lr_handle_getinfo(h, NULL, LRI_FASTESTMIRRORMAXAGE, &num));
    ck_assert(num == LRO_FASTESTMIRRORMAXAGE_DEFAULT);

    str = NULL;
    ck_assert(lr_handle_getinfo(h, NULL, LRI_SSLCLIENTCERT, &str));
    ck_assert_ptr_null(str);

    str = NULL;
    ck_assert(lr_handle_getinfo(h, NULL, LRI_SSLCLIENTKEY, &str));
    ck_assert_ptr_null(str);

    str = NULL;
    ck_assert(lr_handle_getinfo(h, NULL, LRI_SSLCACERT, &str));
    ck_assert_ptr_null(str);

    str = NULL;
    ck_assert(lr_handle_getinfo(h, NULL, LRI_PROXY_SSLCLIENTCERT, &str));
    ck_assert_ptr_null(str);

    str = NULL;
    ck_assert(lr_handle_getinfo(h, NULL, LRI_PROXY_SSLCLIENTKEY, &str));
    ck_assert_ptr_null(str);

    str = NULL;
    ck_assert(lr_handle_getinfo(h, NULL, LRI_PROXY_SSLCACERT, &str));
    ck_assert_ptr_null(str);

    LrAuth auth = LR_AUTH_NONE;
    ck_assert(lr_handle_getinfo(h, NULL, LRI_HTTPAUTHMETHODS, &auth));
    ck_assert(auth == LR_AUTH_BASIC);

    auth = LR_AUTH_NONE;
    ck_assert(lr_handle_getinfo(h, NULL, LRI_PROXYAUTHMETHODS, &auth));
    ck_assert(auth == LR_AUTH_BASIC);

    lr_handle_free(h);
}
END_TEST

Suite *
handle_suite(void)
{
    Suite *s = suite_create("handle");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_handle);
    tcase_add_test(tc, test_handle_getinfo);
    suite_add_tcase(s, tc);
    return s;
}
