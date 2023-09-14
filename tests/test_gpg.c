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

    // Import the first key directly from the file
    ret = lr_gpg_import_key(key_path, tmp_home_path, &tmp_err);
    ck_assert(ret);
    ck_assert_ptr_null(tmp_err);

    // Valid key and data
    ret = lr_gpg_check_signature(signature_path,
                                 data_path,
                                 tmp_home_path,
                                 &tmp_err);
    ck_assert_msg(ret, "Checking valid key and data from file failed with \"%s\"",
            (tmp_err && tmp_err->message) ? tmp_err->message : "");
    ck_assert_msg(NULL == tmp_err, "Checking valid key and data from file passed but set error \"%s\"",
            (tmp_err && tmp_err->message) ? tmp_err->message : "");

    // Bad signature signed with unknown key
    ret = lr_gpg_check_signature(_signature_path,
                                 data_path,
                                 tmp_home_path,
                                 &tmp_err);
    ck_assert(!ret);
    ck_assert_ptr_nonnull(tmp_err);
    g_clear_error(&tmp_err);

    // Bad data
    ret = lr_gpg_check_signature(signature_path,
                                 _data_path,
                                 tmp_home_path,
                                 &tmp_err);
    ck_assert(!ret);
    ck_assert_ptr_nonnull(tmp_err);
    g_clear_error(&tmp_err);

    // Load the second key into memory and import it from memory
    gchar *contents;
    gsize length;
    ret = g_file_get_contents(_key_path, &contents, &length, &tmp_err);
    ck_assert(ret);
    ck_assert_ptr_null(tmp_err);

    ret = lr_gpg_import_key_from_memory(contents, length, tmp_home_path, &tmp_err);
    ck_assert(ret);
    ck_assert_ptr_null(tmp_err);
    g_free(contents);

    // Valid key and data
    ret = lr_gpg_check_signature(_signature_path,
                                 _data_path,
                                 tmp_home_path,
                                 &tmp_err);
    ck_assert_msg(ret, "Checking valid key and data from memory failed with \"%s\"",
            (tmp_err && tmp_err->message) ? tmp_err->message : "");
    ck_assert_msg(NULL == tmp_err, "Checking valid key and data from memory passed but set error \"%s\"",
            (tmp_err && tmp_err->message) ? tmp_err->message : "");

    // Bad signature signed with known key
    ret = lr_gpg_check_signature(_signature_path,
                                 data_path,
                                 tmp_home_path,
                                 &tmp_err);
    ck_assert(!ret);
    ck_assert_ptr_nonnull(tmp_err);
    g_clear_error(&tmp_err);
    tmp_err = NULL;

    // Bad data 2
    ret = lr_gpg_check_signature(_signature_path,
                                 data_path,
                                 tmp_home_path,
                                 &tmp_err);
    ck_assert(!ret);
    ck_assert_ptr_nonnull(tmp_err);
    g_clear_error(&tmp_err);
    tmp_err = NULL;

    lr_remove_dir(tmp_home_path);
    lr_free(key_path);
    lr_free(_key_path);
    lr_free(data_path);
    lr_free(_data_path);
    lr_free(signature_path);
    lr_free(_signature_path);
    g_free(tmp_home_path);
}
END_TEST

START_TEST(test_gpg_check_key_export)
{
    gboolean ret;
    char *key_path;
    char *tmp_home_path;
    GError *tmp_err = NULL;

    tmp_home_path = lr_gettmpdir();
    key_path = lr_pathconcat(test_globals.testdata_dir,
                             "repo_yum_01/repodata/repomd.xml.key", NULL);

    // Import the key from file descriptor
    int key_fd = open(key_path, O_RDONLY);
    ck_assert(key_fd != -1);
    ret = lr_gpg_import_key_from_fd(key_fd, tmp_home_path, &tmp_err);
    ck_assert(ret);
    ck_assert_ptr_null(tmp_err);
    ck_assert(close(key_fd) != -1);

    // Export the keys
    LrGpgKey *keys = lr_gpg_list_keys(TRUE, tmp_home_path, &tmp_err);
    ck_assert_ptr_nonnull(keys);
    ck_assert_ptr_null(tmp_err);

    // Test key user ids
    char * const *uids = lr_gpg_key_get_userids(keys);
    ck_assert_ptr_nonnull(uids);
    ck_assert(g_strcmp0(uids[0], "repository signing key (test key) <noone@example.org>") == 0);
    ck_assert_ptr_null(uids[1]);

    // Get subkeys
    const LrGpgSubkey *subkeys = lr_gpg_key_get_subkeys(keys);
    ck_assert_ptr_nonnull(subkeys);

    // Test first subkey
    const char *id = lr_gpg_subkey_get_id(subkeys);
    ck_assert(g_strcmp0(id, "97BDB7906441EDF5") == 0);
    const char *fingerprint = lr_gpg_subkey_get_fingerprint(subkeys);
    ck_assert(g_strcmp0(fingerprint, "F4FC5B6AC58696023B9DBFB197BDB7906441EDF5") == 0);
    long timestamp = lr_gpg_subkey_get_timestamp(subkeys);
    ck_assert(timestamp == 1677863811);
    gboolean can_sign = lr_gpg_subkey_get_can_sign(subkeys);
    ck_assert(can_sign);

    // Get second subkey
    subkeys = lr_gpg_subkey_get_next(subkeys);
    ck_assert_ptr_nonnull(subkeys);

    // Test second subkey
    id = lr_gpg_subkey_get_id(subkeys);
    ck_assert(g_strcmp0(id, "C4101E247506302F") == 0);
//    fingerprint = lr_gpg_subkey_get_fingerprint(subkeys);
//    ck_assert(g_strcmp0(fingerprint, "FBF903F3B01FFA462C6DBF96C4101E247506302F") == 0);
    timestamp = lr_gpg_subkey_get_timestamp(subkeys);
    ck_assert(timestamp == 1677863811);
//    can_sign = lr_gpg_subkey_get_can_sign(subkeys);
//    ck_assert(!can_sign);

    // There are no other subkeys for the key
    subkeys = lr_gpg_subkey_get_next(subkeys);
    ck_assert_ptr_null(subkeys);

    // Test exported raw key
    const char *raw_key = lr_gpg_key_get_raw_key(keys);
    ck_assert_ptr_nonnull(raw_key);
    ck_assert(strstr(raw_key,
                     "-----BEGIN PGP PUBLIC KEY BLOCK-----\n"
                     "\n"
                     "mQENBGQCK4MBCACmfps+HeGv3BnrFmOO5PGPFDgQ0KtmSkfkrw8Sf3dcsfiMS9II\n"
                     "2AOJNLd8NOgOkGGlyymrrf/FKPWnQHQDKxLgOuZ0FvAIfb+VNTjnuj8TfO94y7n1\n"
                     "VpCu7oRKOQLfmRd3OQDe0CofNjYeYKn2l/an/x80o3ikoFMEdHcOkdhPkm/Kf5oc\n"
                     "bJYVvxb81vvtsEOV5/+ybCpJIxXhk1y3DBBhaZGfEYOy9oF5/w2ZmL+mbLQ6KVlK\n"
                     "euTDlADLv3KAtGALy1E/ZHANChPc372JI7osKT6MhRr1fItY6NCFSgXapDtea0x0\n"
                     "sZj6LvOuWISZ5n+YoTM+loMgxjFHXEd7nV93ABEBAAG0NXJlcG9zaXRvcnkgc2ln\n"
                     "bmluZyBrZXkgKHRlc3Qga2V5KSA8bm9vbmVAZXhhbXBsZS5vcmc+iQFOBBMBCgA4\n"
                     "FiEE9PxbasWGlgI7nb+xl723kGRB7fUFAmQCK4MCGwMFCwkIBwIGFQoJCAsCBBYC\n"
                     "AwECHgECF4AACgkQl723kGRB7fUC7Af/YSwbvM2xsjK947FOmVfX63qf5v2fRI7Q\n"
                     "CEil6X8uZw5UXEvqsGDZCqmhYKSKlNnuky392D0/Kp0h3KCJ7KHA9/SCHrnl2PN4\n"
                     "nenX5xunwJd3mhGOtLR2WSYXLBlG5Dh6bkDsWEaR8OPC8rSNNM9OfuuXMWf0OF91\n"
                     "WYwleixUrfMbX9gVKozFzGv/yAospsJ0F48Z3RW+XROYLwbk72G6jXR0hkB1zEpN\n"
                     "gwbpldxQFEagBDNwpvCF8jlwCOLjOQWM3oTx4YUdG4gAEGDKLsJcv4cMTaEUCr0r\n"
                     "FEIHRcFCox7P4f9UFlIUNLj6zWNP9Nr3Yuqfh4bdYJfWE9kK271NErkBDQRkAiuD\n"
                     "AQgAxjdpqh4z8wcURkon/bdxLKeI1weKYEdprNrEEHLlk5hV+qQ5dHtUCHBcf2sH\n"
                     "kAwG4S3s4WAVoNyFP2l1gC5eM6/J/t1K5UgKEsDdCcVQMO1hA+T6breCLGTyS3Ut\n"
                     "Qes/EZloe+zUjH8covZQwQJFuZp5xVaTVK/7IjYzL/zUK6ySjbXGU4W/iYGV07Je\n"
                     "1W401uCwc3nosjptXgmc6Nhawn6U+I5tgPgX4L8hI3tla+fsiUpDD+Xlz3m90f8w\n"
                     "4baHRSajB4b6Llu963JSTUE5tKfmxVVfHbKTe2Y5Nf6coJXfFxy3UuFDTUfK5S/C\n"
                     "rIwB+ZoahvjfhC6eg7gh4LNnawARAQABiQE2BBgBCgAgFiEE9PxbasWGlgI7nb+x\n"
                     "l723kGRB7fUFAmQCK4MCGwwACgkQl723kGRB7fV9Dgf9GvTNb36IxIOhyqWFFXZp\n"
                     "C3NqyyIbRZ6KEXUzLaC88fx2hK1BQkeDASUS0HxccCCdjR1zesk0QfZtGJxVQFw+\n"
                     "AyLWzMGRLDfMvXYRLzjYowYJ++beVadt/EfPzRHckunW75PLHrDd89Hur92r4ikK\n"
                     "42GA3ZDPr9yggpnYvsAXt25lDx37HN3SeVds2Gb6wA01Vo6jXVKQGhRgk9K5T8dB\n"
                     "szn12XdPGxhzo1HXPUFWwSi8SODpG+MkC+IGOrFzPHW5R5kOO7+nbALjzLWSrcfw\n"
                     "zLiMgTqA0Z2O8QcTGSQ7jYQhSaWguWzhT1LsZ0/9xNGSHyWfYkYtXPAogB50qlvM\n"
                     "LQ==\n"
                     "=RI7R\n"
                     "-----END PGP PUBLIC KEY BLOCK-----\n") != NULL);

    // Only one key is in keyring
    ck_assert_ptr_null(lr_gpg_key_get_next(keys));

    lr_gpg_keys_free(keys);
    lr_remove_dir(tmp_home_path);
    lr_free(key_path);
    g_free(tmp_home_path);
}
END_TEST


Suite *
gpg_suite(void)
{
    Suite *s = suite_create("gpg");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_gpg_check_signature);
    tcase_add_test(tc, test_gpg_check_key_export);
    suite_add_tcase(s, tc);
    return s;
}
