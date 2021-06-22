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
#include "librepo/gpg.h"

#include "fixtures.h"
#include "testsys.h"
#include "test_gpg.h"

START_TEST(test_gpg_check_signature)
{
    gboolean ret;
    char *key_path, *_key_path;
    char *data_path, *_data_path;
    char *signature_path, *_signature_path;
    char *tmp_home_path;
    GError *tmp_err = NULL;

    tmp_home_path = lr_gettmpdir();
    key_path = lr_pathconcat(test_globals.testdata_dir,
                             "repo_yum_01/repodata/repomd.xml.key", NULL);
    _key_path = lr_pathconcat(test_globals.testdata_dir,
                             "repo_yum_01/repodata/repomd.xml_bad.key", NULL);
    data_path = lr_pathconcat(test_globals.testdata_dir,
                             "repo_yum_01/repodata/repomd.xml", NULL);
    _data_path = lr_pathconcat(test_globals.testdata_dir,
                             "repo_yum_01/repodata/repomd.xml_bad", NULL);
    signature_path = lr_pathconcat(test_globals.testdata_dir,
                             "repo_yum_01/repodata/repomd.xml.asc", NULL);
    _signature_path = lr_pathconcat(test_globals.testdata_dir,
                             "repo_yum_01/repodata/repomd.xml_bad.asc", NULL);

    ret = lr_gpg_import_key(key_path, tmp_home_path, &tmp_err);
    ck_assert(ret);
    ck_assert_ptr_null(tmp_err);

    // Valid key and data
    ret = lr_gpg_check_signature(signature_path,
                                 data_path,
                                 tmp_home_path,
                                 &tmp_err);
    ck_assert(ret);
    ck_assert_ptr_null(tmp_err);

    // Bad signature signed with unknown key
    ret = lr_gpg_check_signature(_signature_path,
                                 data_path,
                                 tmp_home_path,
                                 &tmp_err);
    ck_assert(!ret);
    ck_assert_ptr_nonnull(tmp_err);
    g_error_free(tmp_err);
    tmp_err = NULL;

    // Bad data
    ret = lr_gpg_check_signature(signature_path,
                                 _data_path,
                                 tmp_home_path,
                                 &tmp_err);
    ck_assert(!ret);
    ck_assert_ptr_nonnull(tmp_err);
    g_error_free(tmp_err);
    tmp_err = NULL;

    // Import the 2nd key
    ret = lr_gpg_import_key(_key_path, tmp_home_path, &tmp_err);
    ck_assert(ret);
    ck_assert_ptr_null(tmp_err);

    // Valid key and data
    ret = lr_gpg_check_signature(_signature_path,
                                 _data_path,
                                 tmp_home_path,
                                 &tmp_err);
    ck_assert(ret);
    ck_assert_ptr_null(tmp_err);

    // Bad signature signed with known key
    ret = lr_gpg_check_signature(_signature_path,
                                 data_path,
                                 tmp_home_path,
                                 &tmp_err);
    ck_assert(!ret);
    ck_assert_ptr_nonnull(tmp_err);
    g_error_free(tmp_err);
    tmp_err = NULL;

    // Bad data 2
    ret = lr_gpg_check_signature(_signature_path,
                                 data_path,
                                 tmp_home_path,
                                 &tmp_err);
    ck_assert(!ret);
    ck_assert_ptr_nonnull(tmp_err);
    g_error_free(tmp_err);
    tmp_err = NULL;

    lr_remove_dir(tmp_home_path);
    lr_free(key_path);
    lr_free(_key_path);
    lr_free(data_path);
    lr_free(_data_path);
    lr_free(signature_path);
    lr_free(_signature_path);
    lr_free(tmp_home_path);
}
END_TEST

Suite *
gpg_suite(void)
{
    Suite *s = suite_create("gpg");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_gpg_check_signature);
    suite_add_tcase(s, tc);
    return s;
}
