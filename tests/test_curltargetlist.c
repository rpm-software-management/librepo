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
#include "librepo/curltargetlist.h"

#include "fixtures.h"
#include "testsys.h"
#include "test_curltargetlist.h"

START_TEST(test_curltarget)
{
    lr_CurlTarget t = NULL;

    t = lr_curltarget_new();
    fail_if(t == NULL);
    fail_if(t->path != NULL);
    fail_if(t->checksum != NULL);
    lr_curltarget_free(t);
}
END_TEST

START_TEST(test_curltargetlist)
{
    lr_CurlTarget t = NULL;
    lr_CurlTargetList tl = NULL;

    tl = lr_curltargetlist_new();
    fail_if(tl == NULL);
    fail_if(lr_curltargetlist_len(tl) != 0);
    fail_if(lr_curltargetlist_get(tl, 0) != NULL);
    fail_if(lr_curltargetlist_get(tl, -1) != NULL);
    fail_if(lr_curltargetlist_get(tl, 1) != NULL);

    t = lr_curltarget_new();
    t->path = lr_strdup("http://foobar");

    lr_curltargetlist_append(tl, t);
    fail_if(lr_curltargetlist_len(tl) != 1);
    fail_if(lr_curltargetlist_get(tl, -1) != NULL);
    fail_if(lr_curltargetlist_get(tl, 1) != NULL);
    t = lr_curltargetlist_get(tl, 0);
    fail_if(t == NULL);
    fail_if(strcmp(t->path, "http://foobar"));

    lr_curltargetlist_free(tl);
}
END_TEST

Suite *
curltargetlist_suite(void)
{
    Suite *s = suite_create("util");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_curltarget);
    tcase_add_test(tc, test_curltargetlist);
    suite_add_tcase(s, tc);
    return s;
}
