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
    fail_if(!target);
    fail_if(err);

    fail_if(strcmp(target->relative_url, "url"));
    fail_if(target->dest);
    fail_if(target->base_url);
    fail_if(target->checksum_type != 0);
    fail_if(target->checksum);
    fail_if(target->resume != FALSE);
    fail_if(target->progresscb);
    fail_if(target->cbdata);
    fail_if(target->local_path);
    fail_if(target->err);

    lr_packagetarget_free(target);
    target = NULL;

    // Init with all options

    target = lr_packagetarget_new(NULL, "url", "dest", LR_CHECKSUM_SHA384,
                                  "xxx", 0, "baseurl", TRUE, (LrProgressCb) 22,
                                  (void *) 33, &err);
    fail_if(!target);
    fail_if(err);

    fail_if(strcmp(target->relative_url, "url"));
    fail_if(strcmp(target->dest, "dest"));
    fail_if(strcmp(target->base_url, "baseurl"));
    fail_if(target->checksum_type != LR_CHECKSUM_SHA384);
    fail_if(strcmp(target->checksum, "xxx"));
    fail_if(target->resume != TRUE);
    fail_if(target->progresscb != (LrProgressCb) 22);
    fail_if(target->cbdata != (void *) 33);
    fail_if(target->local_path);
    fail_if(target->err);

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
