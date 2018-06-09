#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "testsys.h"
#include "fixtures.h"
#include "test_repo_zck.h"
#include "librepo/rcodes.h"
#include "librepo/types.h"
#include "librepo/repomd.h"
#include "librepo/util.h"

START_TEST(test_repo_zck_parsing)
{
    int fd;
    gboolean ret;
    LrYumRepoMd *repomd;
    char *repomd_path;
    GError *tmp_err = NULL;

    repomd_path = lr_pathconcat(test_globals.testdata_dir,
                                "repo_yum_03/repodata/repomd.xml",
                                NULL);
    repomd = lr_yum_repomd_init();
    fail_if(!repomd);
    fd = open(repomd_path, O_RDONLY);
    fail_if(fd < 0);

    ret = lr_yum_repomd_parse_file(repomd, fd, NULL, NULL, &tmp_err);
    close(fd);

    fail_if(!ret);
    fail_if(tmp_err);
    fail_if(g_slist_length(repomd->records) != 12);
    fail_if(!lr_yum_repomd_get_record(repomd, "primary"));
    fail_if(!lr_yum_repomd_get_record(repomd, "filelists"));
    fail_if(!lr_yum_repomd_get_record(repomd, "other"));

    fail_if(lr_yum_repomd_get_record(repomd, "foo"));
    fail_if(lr_yum_repomd_get_record(repomd, "bar"));

    lr_yum_repomd_free(repomd);
    lr_free(repomd_path);
}
END_TEST

Suite *
repo_zck_suite(void)
{
    Suite *s = suite_create("repo_zck");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_repo_zck_parsing);
    suite_add_tcase(s, tc);
    return s;
}
