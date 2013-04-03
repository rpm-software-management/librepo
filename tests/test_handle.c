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

#include "fixtures.h"
#include "testsys.h"
#include "test_handle.h"

START_TEST(test_handle)
{
    lr_Handle h = NULL;

    lr_global_init();
    h = lr_handle_init();
    fail_if(h == NULL);
    lr_handle_free(h);
    h = NULL;

    /* This test is meant to check memory leaks. (Use valgrind) */
    h = lr_handle_init();
    lr_handle_setopt(h, LRO_URL, "foo");
    lr_handle_setopt(h, LRO_MIRRORLIST, "bar");
    lr_handle_setopt(h, LRO_USERPWD, "user:pwd");
    lr_handle_setopt(h, LRO_PROXY, "proxy");
    lr_handle_setopt(h, LRO_PROXYUSERPWD, "proxyuser:pwd");
    lr_handle_setopt(h, LRO_DESTDIR, "foodir");
    lr_handle_setopt(h, LRO_USERAGENT, "librepo/0.0");
    char *dlist[] = {"primary", "filelists", NULL};
    lr_handle_setopt(h, LRO_YUMDLIST, dlist);
    lr_handle_setopt(h, LRO_YUMBLIST, dlist);
    lr_handle_free(h);
    lr_global_cleanup();
}
END_TEST

START_TEST(test_handle_getinfo)
{
    long num;
    char *str;
    char **strlist;
    lr_Handle h = NULL;

    lr_global_init();
    h = lr_handle_init();

    num = -1;
    lr_handle_getinfo(h, LRI_UPDATE, &num);
    fail_if(num != 0);

    str = NULL;
    lr_handle_getinfo(h, LRI_URL, &str);
    fail_if(str != NULL);

    str = NULL;
    lr_handle_getinfo(h, LRI_MIRRORLIST, &str);
    fail_if(str != NULL);

    num = -1;
    lr_handle_getinfo(h, LRI_LOCAL, &num);
    fail_if(num != 0);

    str = NULL;
    lr_handle_getinfo(h, LRI_DESTDIR, &str);
    fail_if(str != NULL);

    num = -1;
    lr_handle_getinfo(h, LRI_REPOTYPE, &num);
    fail_if(num != 0);

    str = NULL;
    lr_handle_getinfo(h, LRI_USERAGENT, &str);
    fail_if(num != 0);

    strlist = NULL;
    lr_handle_getinfo(h, LRI_YUMDLIST, &strlist);
    fail_if(strlist != NULL);

    strlist = NULL;
    lr_handle_getinfo(h, LRI_YUMBLIST, &strlist);
    fail_if(strlist != NULL);

    num = -1;
    lr_handle_getinfo(h, LRI_LASTCURLERR, &num);
    fail_if(num != 0);

    num = -1;
    lr_handle_getinfo(h, LRI_LASTCURLMERR, &num);
    fail_if(num != 0);

    str = NULL;
    lr_handle_getinfo(h, LRI_LASTCURLSTRERR, &str);

    str = NULL;
    lr_handle_getinfo(h, LRI_LASTCURLMSTRERR, &str);

    num = -1;
    lr_handle_getinfo(h, LRI_LASTBADSTATUSCODE, &num);
    fail_if(num != 0);

    lr_handle_free(h);
    lr_global_cleanup();
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
