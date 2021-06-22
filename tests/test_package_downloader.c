#define _GNU_SOURCE
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fixtures.h"
#include "testsys.h"
#include "test_package_downloader.h"

#include "librepo/librepo.h"
#include "librepo/rcodes.h"
#include "librepo/package_downloader.h"

START_TEST(test_package_downloader_new_and_free)
{
    LrPackageTarget *target;
    GError *err = NULL;

    // Init with only basic options

    target = lr_packagetarget_new(NULL, "url", NULL, 0, NULL, 0, NULL, FALSE,
                                  NULL, NULL, &err);
    ck_assert_ptr_nonnull(target);
    ck_assert_ptr_null(err);

    ck_assert_str_eq(target->relative_url, "url");
    ck_assert_ptr_null(target->dest);
    ck_assert_ptr_null(target->base_url);
    ck_assert(target->checksum_type == 0);
    ck_assert_ptr_null(target->checksum);
    ck_assert(target->resume == FALSE);
    ck_assert(!target->progresscb);
    ck_assert_ptr_null(target->cbdata);
    ck_assert_ptr_null(target->local_path);
    ck_assert_ptr_null(target->err);

    lr_packagetarget_free(target);
    target = NULL;

    // Init with all options

    target = lr_packagetarget_new(NULL, "url", "dest", LR_CHECKSUM_SHA384,
                                  "xxx", 0, "baseurl", TRUE, (LrProgressCb) 22,
                                  (void *) 33, &err);
    ck_assert_ptr_nonnull(target);
    ck_assert_ptr_null(err);

    ck_assert_str_eq(target->relative_url, "url");
    ck_assert_str_eq(target->dest, "dest");
    ck_assert_str_eq(target->base_url, "baseurl");
    ck_assert(target->checksum_type == LR_CHECKSUM_SHA384);
    ck_assert_str_eq(target->checksum, "xxx");
    ck_assert(target->resume == TRUE);
    ck_assert(target->progresscb == (LrProgressCb) 22);
    ck_assert_ptr_eq(target->cbdata, (void *) 33);
    ck_assert_ptr_null(target->local_path);
    ck_assert_ptr_null(target->err);

    lr_packagetarget_free(target);
    target = NULL;

}
END_TEST

Suite *
package_downloader_suite(void)
{
    Suite *s = suite_create("package_downloader");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_package_downloader_new_and_free);
    suite_add_tcase(s, tc);
    return s;
}
