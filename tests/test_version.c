#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "librepo/version.h"

#include "fixtures.h"
#include "testsys.h"
#include "test_version.h"

START_TEST(test_version_check_macro)
{
    fail_if(!(LR_VERSION_CHECK(LR_VERSION_MAJOR,
                               LR_VERSION_MINOR,
                               LR_VERSION_PATCH)));

    fail_if(!(LR_VERSION_CHECK(0, 0, 0)));

    fail_if(LR_VERSION_CHECK(LR_VERSION_MAJOR,
                             LR_VERSION_MINOR,
                             LR_VERSION_PATCH+1));

    fail_if(LR_VERSION_CHECK(LR_VERSION_MAJOR,
                             LR_VERSION_MINOR+1,
                             LR_VERSION_PATCH));

    fail_if(LR_VERSION_CHECK(LR_VERSION_MAJOR+1,
                             LR_VERSION_MINOR,
                             LR_VERSION_PATCH));
}
END_TEST

Suite *
version_suite(void)
{
    Suite *s = suite_create("version");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_version_check_macro);
    suite_add_tcase(s, tc);
    return s;
}
