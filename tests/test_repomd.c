#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "testsys.h"
#include "fixtures.h"
#include "test_repomd.h"
#include "librepo/rcodes.h"
#include "librepo/types.h"
#include "librepo/repomd.h"
#include "librepo/util.h"

START_TEST(test_repomd_parsing)
{
    int fd;
    gboolean ret;
    LrYumRepoMd *repomd;
    char *repomd_path;
    GError *tmp_err = NULL;

    repomd_path = lr_pathconcat(test_globals.testdata_dir,
                                "repo_yum_02/repodata/repomd.xml",
                                NULL);
    repomd = lr_yum_repomd_init();
    ck_assert_ptr_nonnull(repomd);
    fd = open(repomd_path, O_RDONLY);
    ck_assert_int_ge(fd, 0);

    ret = lr_yum_repomd_parse_file(repomd, fd, NULL, NULL, &tmp_err);
    close(fd);

    ck_assert(ret);
    ck_assert_ptr_null(tmp_err);
    ck_assert(g_slist_length(repomd->records) == 12);
    ck_assert_ptr_nonnull(lr_yum_repomd_get_record(repomd, "primary"));
    ck_assert_ptr_nonnull(lr_yum_repomd_get_record(repomd, "filelists"));
    ck_assert_ptr_nonnull(lr_yum_repomd_get_record(repomd, "other"));
    ck_assert_ptr_nonnull(lr_yum_repomd_get_record(repomd, "primary_db"));
    ck_assert_ptr_nonnull(lr_yum_repomd_get_record(repomd, "filelists_db"));
    ck_assert_ptr_nonnull(lr_yum_repomd_get_record(repomd, "other_db"));
    ck_assert_ptr_nonnull(lr_yum_repomd_get_record(repomd, "group"));
    ck_assert_ptr_nonnull(lr_yum_repomd_get_record(repomd, "group_gz"));
    ck_assert_ptr_nonnull(lr_yum_repomd_get_record(repomd, "updateinfo"));
    ck_assert_ptr_nonnull(lr_yum_repomd_get_record(repomd, "origin"));
    ck_assert_ptr_nonnull(lr_yum_repomd_get_record(repomd, "prestodelta"));
    ck_assert_ptr_nonnull(lr_yum_repomd_get_record(repomd, "deltainfo"));

    ck_assert_ptr_null(lr_yum_repomd_get_record(repomd, "foo"));
    ck_assert_ptr_null(lr_yum_repomd_get_record(repomd, "bar"));

    lr_yum_repomd_free(repomd);
    lr_free(repomd_path);
}
END_TEST

Suite *
repomd_suite(void)
{
    Suite *s = suite_create("repomd");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_repomd_parsing);
    suite_add_tcase(s, tc);
    return s;
}
