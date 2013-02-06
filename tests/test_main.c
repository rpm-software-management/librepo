#define _GNU_SOURCE
#include <assert.h>
#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "librepo/util.h"

#include "fixtures.h"
#include "testsys.h"
#include "test_checksum.h"
#include "test_internal_mirrorlist.h"
#include "test_repomd.h"
#include "test_gpg.h"
#include "test_util.h"


static int
init_test_globals(struct TestGlobals_s *tg, const char *repo_dir)
{
    tg->repo_dir = lr_pathconcat(repo_dir, "/", NULL);
    tg->tmpdir = lr_strdup(UNITTEST_DIR);
    if (mkdtemp(tg->tmpdir) == NULL)
        return 1;
    return 0;
}

static void
free_test_globals(struct TestGlobals_s *tg)
{
    lr_free(tg->tmpdir);
    lr_free(tg->repo_dir);
}

int
main(int argc, const char **argv)
{
    int number_failed;
    struct stat s;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <repos_directory>\n", argv[0]);
        exit(1);
    }
    if (stat(argv[1], &s) || !S_ISDIR(s.st_mode)) {
        fprintf(stderr, "can not read repos at '%s'.\n", argv[1]);
        exit(1);
    }
    if (init_test_globals(&test_globals, argv[1])) {
        fprintf(stderr, "failed initializing test engine.\n");
        exit(1);
    }
    printf("Tests using directory: %s\n", test_globals.tmpdir);

    SRunner *sr = srunner_create(checksum_suite());
    srunner_add_suite(sr, internal_mirrorlist_suite());
    srunner_add_suite(sr, repomd_suite());
    srunner_add_suite(sr, gpg_suite());
    srunner_add_suite(sr, util_suite());
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    free_test_globals(&test_globals);
    return (number_failed == 0) ? 0 : 1;
}
