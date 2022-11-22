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
    ck_assert(ret);
    ck_assert_ptr_null(tmp_err);

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
    ck_assert(ret);
    ck_assert_ptr_null(tmp_err);

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
    ck_assert(g_strcmp0(uids[0], "Tomas Mlcoch (test key) <tmlcoch@redhat.com>") == 0);
    ck_assert_ptr_null(uids[1]);

    // Get subkeys
    const LrGpgSubkey *subkeys = lr_gpg_key_get_subkeys(keys);
    ck_assert_ptr_nonnull(subkeys);

    // Test first subkey
    const char *id = lr_gpg_subkey_get_id(subkeys);
    ck_assert(g_strcmp0(id, "46AF958A22F2C4E9") == 0);
    const char *fingerprint = lr_gpg_subkey_get_fingerprint(subkeys);
    ck_assert(g_strcmp0(fingerprint, "55B80C4944D8938E94980B6D46AF958A22F2C4E9") == 0);
    long timestamp = lr_gpg_subkey_get_timestamp(subkeys);
    ck_assert(timestamp == 1347882156);
    gboolean can_sign = lr_gpg_subkey_get_can_sign(subkeys);
    ck_assert(can_sign);

    // Get second subkey
    subkeys = lr_gpg_subkey_get_next(subkeys);
    ck_assert_ptr_nonnull(subkeys);

    // Test second subkey
    id = lr_gpg_subkey_get_id(subkeys);
    ck_assert(g_strcmp0(id, "7AE6F6EF026AF38A") == 0);
    fingerprint = lr_gpg_subkey_get_fingerprint(subkeys);
    ck_assert(g_strcmp0(fingerprint, "D855DBAA43DB343EC4F214EB7AE6F6EF026AF38A") == 0);
    timestamp = lr_gpg_subkey_get_timestamp(subkeys);
    ck_assert(timestamp == 1347882156);
    can_sign = lr_gpg_subkey_get_can_sign(subkeys);
    ck_assert(!can_sign);

    // There are no other subkeys for the key
    subkeys = lr_gpg_subkey_get_next(subkeys);
    ck_assert_ptr_null(subkeys);

    // Test exported raw key
    const char *raw_key = lr_gpg_key_get_raw_key(keys);
    ck_assert_ptr_nonnull(raw_key);
    ck_assert(strstr(raw_key,
                     "mQENBFBXDKwBCADA5jpCwpb/JKOG8mcFyIanNojDwpHwKoyjGNpZNPNUDJguvkRa\n"
                     "IO3NdoyXYd5QVTOsnyKBaRaiLLJWI/VJxTOT3fwOPprrzUlkHwoWl+sYuSdXHASu\n"
                     "m4lkBiXHsa5oiXPdrY6hoh5vsF8ASwCHXOwpR9yyvGEaUUMBl2GpJAX/cGVcL4Dy\n"
                     "Z0pyJMLO4qrIPoX+wd1ZSFSc8JcAC4UtA82HCGTmesgialpwKdoQyt+em94oIM1f\n"
                     "D6v7zzcRX/zLKKEzpFnU458WBA+JACkde3ohX//0fDCeaLqMzs++FCgwm/HMCszw\n"
                     "RnINr+K8ENfMYBoeM7a7tnhiae+rkxWmvWz/ABEBAAG0LFRvbWFzIE1sY29jaCAo\n"
                     "dGVzdCBrZXkpIDx0bWxjb2NoQHJlZGhhdC5jb20+iQE4BBMBAgAiBQJQVwysAhsD\n"
                     "BgsJCAcDAgYVCAIJCgsEFgIDAQIeAQIXgAAKCRBGr5WKIvLE6fdyB/9OzDczaqGy\n"
                     "1rzk7Wp2C9S5QatFUFNWt6FIFPITbixT4jrDo/LyUJVWLw4ng7ldg79vmrzhpP8h\n"
                     "yFVvuvGvSEMn5sgnZ83SEd4vRJ2O8K5RuVs5Kcj7ayLlxPpqbYOYmrmTaLwYTwdv\n"
                     "0wDnNU9IkkMSK752RQes4J+4XGikd8CNm5lw8cRQ7bcQd8s2rnCoiyGt7PIdl13z\n"
                     "8hO9KA52iUP06AbbIusbQ1jzsViEny+xQH7SZ53Ga4eRr0mW2iA20Mkp4Ieb+dNo\n"
                     "47Q8aHUqI9O4HTF/3Fzt7KmNxXCpCOhxTWx0IkqPGoZ0W63Aut/CVh0LXsBF2TUD\n"
                     "Ym9P/IjRgJLhuQENBFBXDKwBCACvhlMcgjLJ4PtMTtauF1OXVTfODQSHo+qwKt4S\n"
                     "GyDlTGayQ76pOqYkkzIRqmNYl1ThmcfzpmJ3O+hFQQ7OdguYcfkbfgIMjEJEbKG3\n"
                     "wsR5pm9zjwStzYHedwkct1nyROgBz70o16FfdiWOguw58jQZOSO/I2S3JpLsLgI8\n"
                     "KqdIk/0WuoOfzt+KcvL52lX94O2hBpRI0v6lDgSm7KkPGQrVFnSIUR5r73ceageL\n"
                     "5LmGm1TlEjWHwA9iYIvBcjnE26/l8u58IYQ/sUmn0u4jBcBNc7iqdWvlSLZLlMmi\n"
                     "qnzDNhUup9neKGxgr4hGAblxiSxXlmOoFv0jEW81b4VximSJABEBAAGJAR8EGAEC\n"
                     "AAkFAlBXDKwCGwwACgkQRq+ViiLyxOnf6Qf+PIG//12qp3hXZsvB7JQuQ4nUNwp6\n"
                     "Ufm6W9pFm3DOqnI9H9ZNzGbkoS5WwRp0B1NLfNKipQVORnDs6qve298ReRrmLKnk\n"
                     "BPZqxFpPqLQ6X83Or2bqKiJS1axonIgqkImFLfxxqKoukvhn328Z2FVlrvkKSMU8\n"
                     "eHi/iDF/TCHoPE9WtnVSzsNU9i+9j8j//GO+bMC5AGNOxcBKlChFpLYpE/pfITL/\n"
                     "icS7wB9MrMLNvjlN1EKszQFxJrFVBGTt8hUqRH3CCUFRwbpE1QJ1WAzJ0Vzk5nWR\n"
                     "rVZQiiLe03B8hC7/qRiB4bya5nbWcwe9ltPFja4/tTe92ivScFfCLyALVQ==\n"
                     "=G300") != NULL);

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
