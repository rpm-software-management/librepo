#include <sys/types.h>
#include <sys/stat.h>
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
    int fd, rc;
    lr_YumRepoMd *repomd;
    char *repomd_path;
    GError *tmp_err = NULL;

    repomd_path = lr_pathconcat(test_globals.testdata_dir,
                                "repo_yum_02/repodata/repomd.xml",
                                NULL);
    repomd = lr_yum_repomd_init();
    fail_if(!repomd);
    fd = open(repomd_path, O_RDONLY);
    fail_if(fd < 0);

    rc = lr_yum_repomd_parse_file(repomd, fd, NULL, NULL, &tmp_err);
    fail_if(rc != LRE_OK);
    fail_if(g_slist_length(repomd->records) != 12);
    fail_if(!lr_yum_repomd_get_record(repomd, "primary"));
    fail_if(!lr_yum_repomd_get_record(repomd, "filelists"));
    fail_if(!lr_yum_repomd_get_record(repomd, "other"));
    fail_if(!lr_yum_repomd_get_record(repomd, "primary_db"));
    fail_if(!lr_yum_repomd_get_record(repomd, "filelists_db"));
    fail_if(!lr_yum_repomd_get_record(repomd, "other_db"));
    fail_if(!lr_yum_repomd_get_record(repomd, "group"));
    fail_if(!lr_yum_repomd_get_record(repomd, "group_gz"));
    fail_if(!lr_yum_repomd_get_record(repomd, "updateinfo"));
    fail_if(!lr_yum_repomd_get_record(repomd, "origin"));
    fail_if(!lr_yum_repomd_get_record(repomd, "prestodelta"));
    fail_if(!lr_yum_repomd_get_record(repomd, "deltainfo"));

    fail_if(lr_yum_repomd_get_record(repomd, "foo"));
    fail_if(lr_yum_repomd_get_record(repomd, "bar"));

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
