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
    fail_if(h == NULL);
    lr_handle_free(h);
    h = NULL;

    /* This test is meant to check memory leaks. (Use valgrind) */
    h = lr_handle_init();
    char *urls[] = {"foo", NULL};
    fail_if(!lr_handle_setopt(h, &tmp_err, LRO_URLS, urls));
    fail_if(tmp_err);
    fail_if(!lr_handle_setopt(h, &tmp_err, LRO_URLS, urls));
    fail_if(tmp_err);
    fail_if(!lr_handle_setopt(h, &tmp_err, LRO_MIRRORLIST, "foo"));
    fail_if(tmp_err);
    fail_if(!lr_handle_setopt(h, &tmp_err, LRO_MIRRORLIST, "bar"));
    fail_if(tmp_err);
    fail_if(!lr_handle_setopt(h, NULL, LRO_USERPWD, "user:pwd"));
    fail_if(!lr_handle_setopt(h, NULL, LRO_PROXY, "proxy"));
    fail_if(!lr_handle_setopt(h, NULL, LRO_PROXYUSERPWD, "proxyuser:pwd"));
    fail_if(!lr_handle_setopt(h, NULL, LRO_DESTDIR, "foodir"));
    fail_if(!lr_handle_setopt(h, NULL, LRO_USERAGENT, "librepo/0.0"));
    char *dlist[] = {"primary", "filelists", NULL};
    fail_if(!lr_handle_setopt(h, NULL, LRO_YUMDLIST, dlist));
    fail_if(!lr_handle_setopt(h, NULL, LRO_YUMBLIST, dlist));
    LrUrlVars *vars = NULL;
    vars = lr_urlvars_set(vars, "foo", "bar");
    fail_if(!lr_handle_setopt(h, NULL, LRO_VARSUB, vars));
    fail_if(!lr_handle_setopt(h, NULL, LRO_FASTESTMIRRORCACHE,
                              "/var/cache/fastestmirror.librepo"));
    fail_if(!lr_handle_setopt(h, NULL, LRO_SSLCLIENTCERT, "/etc/cert.pem"));
    fail_if(!lr_handle_setopt(h, NULL, LRO_SSLCLIENTKEY, "/etc/cert.key"));
    fail_if(!lr_handle_setopt(h, NULL, LRO_SSLCACERT, "/etc/ca.pem"));
    fail_if(!lr_handle_setopt(h, NULL, LRO_PROXY_SSLCLIENTCERT, "/etc/proxy_cert.pem"));
    fail_if(!lr_handle_setopt(h, NULL, LRO_PROXY_SSLCLIENTKEY, "/etc/proxy_cert.key"));
    fail_if(!lr_handle_setopt(h, NULL, LRO_PROXY_SSLCACERT, "/etc/proxy_ca.pem"));
    fail_if(!lr_handle_setopt(h, NULL, LRO_HTTPAUTHMETHODS, LR_AUTH_NTLM));
    fail_if(!lr_handle_setopt(h, NULL, LRO_PROXYAUTHMETHODS, LR_AUTH_DIGEST));
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
    fail_if(!lr_handle_getinfo(h, &tmp_err, LRI_UPDATE, &num));
    fail_if(num != 0);
    fail_if(tmp_err);

    strlist = NULL;
    fail_if(!lr_handle_getinfo(h, &tmp_err, LRI_URLS, &strlist));
    fail_if(strlist != NULL);
    fail_if(tmp_err);

    str = NULL;
    fail_if(!lr_handle_getinfo(h, NULL, LRI_MIRRORLIST, &str));
    fail_if(str != NULL);

    num = -1;
    fail_if(!lr_handle_getinfo(h, NULL, LRI_LOCAL, &num));
    fail_if(num != 0);

    str = NULL;
    fail_if(!lr_handle_getinfo(h, NULL, LRI_DESTDIR, &str));
    fail_if(str != NULL);

    num = -1;
    fail_if(!lr_handle_getinfo(h, NULL, LRI_REPOTYPE, &num));
    fail_if(num != 0);

    str = NULL;
    fail_if(!lr_handle_getinfo(h, NULL, LRI_USERAGENT, &str));
    fail_if(str != NULL);

    strlist = NULL;
    fail_if(!lr_handle_getinfo(h, NULL, LRI_YUMDLIST, &strlist));
    fail_if(strlist != NULL);

    strlist = NULL;
    fail_if(!lr_handle_getinfo(h, NULL, LRI_YUMBLIST, &strlist));
    fail_if(strlist != NULL);

    num = -1;
    fail_if(!lr_handle_getinfo(h, NULL, LRI_MAXMIRRORTRIES, &num));
    fail_if(num != 0);

    LrUrlVars *vars = NULL;
    fail_if(!lr_handle_getinfo(h, NULL, LRI_VARSUB, &vars));
    fail_if(strlist != NULL);

    str = NULL;
    fail_if(!lr_handle_getinfo(h, NULL, LRI_FASTESTMIRRORCACHE, &str));
    fail_if(str != NULL);

    num = -1;
    fail_if(!lr_handle_getinfo(h, NULL, LRI_FASTESTMIRRORMAXAGE, &num));
    fail_if(num != LRO_FASTESTMIRRORMAXAGE_DEFAULT);

    str = NULL;
    fail_if(!lr_handle_getinfo(h, NULL, LRI_SSLCLIENTCERT, &str));
    fail_if(str != NULL);

    str = NULL;
    fail_if(!lr_handle_getinfo(h, NULL, LRI_SSLCLIENTKEY, &str));
    fail_if(str != NULL);

    str = NULL;
    fail_if(!lr_handle_getinfo(h, NULL, LRI_SSLCACERT, &str));
    fail_if(str != NULL);

    str = NULL;
    fail_if(!lr_handle_getinfo(h, NULL, LRI_PROXY_SSLCLIENTCERT, &str));
    fail_if(str != NULL);

    str = NULL;
    fail_if(!lr_handle_getinfo(h, NULL, LRI_PROXY_SSLCLIENTKEY, &str));
    fail_if(str != NULL);

    str = NULL;
    fail_if(!lr_handle_getinfo(h, NULL, LRI_PROXY_SSLCACERT, &str));
    fail_if(str != NULL);

    LrAuth auth = LR_AUTH_NONE;
    fail_if(!lr_handle_getinfo(h, NULL, LRI_HTTPAUTHMETHODS, &auth));
    fail_if(auth != LR_AUTH_BASIC);

    auth = LR_AUTH_NONE;
    fail_if(!lr_handle_getinfo(h, NULL, LRI_PROXYAUTHMETHODS, &auth));
    fail_if(auth != LR_AUTH_BASIC);

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
