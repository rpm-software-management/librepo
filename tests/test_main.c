#define _GNU_SOURCE
#include <assert.h>
#include <ctype.h>
#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "librepo/util.h"

#include "fixtures.h"
#include "test_checksum.h"
#include "test_downloader.h"
#include "test_gpg.h"
#include "test_handle.h"
#include "test_lrmirrorlist.h"
#include "test_metalink.h"
#include "test_mirrorlist.h"
#include "test_repomd.h"
#include "test_url_substitution.h"
#include "test_util.h"
#include "test_version.h"
#include "testsys.h"


static int
init_test_globals(struct TestGlobals_s *tg, const char *testdata_dir)
{
    tg->testdata_dir = lr_pathconcat(testdata_dir, "/", NULL);
    tg->tmpdir = lr_strdup(UNITTEST_DIR);
    if (mkdtemp(tg->tmpdir) == NULL)
        return 1;
    return 0;
}

static void
free_test_globals(struct TestGlobals_s *tg)
{
    lr_free(tg->tmpdir);
    lr_free(tg->testdata_dir);
}

void
print_help(const char *prog)
{
    fprintf(stderr, "usage: %s [-v] [-d] <repos_directory>\n", prog);
    fprintf(stderr, \
            "\n"\
            "-v    Verbose output.\n"\
            "-d    Do downloading tests (Needs internet connection).\n"\
            "\n");
}

int
main(int argc, char **argv)
{
    int c;
    int verbose = 0;
    int downloading = 0;
    int number_failed;
    struct stat s;
    const char *repos_dir;

    while ((c = getopt (argc, argv, "vd")) != -1)
        switch (c) {
        case 'v':
            verbose = 1;
            break;
        case 'd':
            downloading = 1;
            break;
        default:
            print_help(argv[0]);
            exit(1);
        }

    if ((argc - optind) != 1) {
        print_help(argv[0]);
        exit(1);
    }

    repos_dir = argv[optind];

    if (verbose) {
        // Setup logging
        g_log_set_handler("librepo", G_LOG_LEVEL_ERROR |
                                     G_LOG_LEVEL_CRITICAL |
                                     G_LOG_LEVEL_DEBUG |
                                     G_LOG_LEVEL_WARNING,
                          test_log_handler_cb, NULL);
    }

    if (stat(repos_dir, &s) || !S_ISDIR(s.st_mode)) {
        fprintf(stderr, "can not read repos at '%s'.\n", repos_dir);
        exit(1);
    }
    if (init_test_globals(&test_globals, repos_dir)) {
        fprintf(stderr, "failed initializing test engine.\n");
        exit(1);
    }
    printf("Tests using directory: %s\n", test_globals.tmpdir);

    SRunner *sr = srunner_create(checksum_suite());
    if (downloading)
        srunner_add_suite(sr, downloader_suite());
    srunner_add_suite(sr, gpg_suite());
    srunner_add_suite(sr, handle_suite());
    srunner_add_suite(sr, lrmirrorlist_suite());
    srunner_add_suite(sr, metalink_suite());
    srunner_add_suite(sr, mirrorlist_suite());
    srunner_add_suite(sr, repomd_suite());
    srunner_add_suite(sr, url_substitution_suite());
    srunner_add_suite(sr, util_suite());
    srunner_add_suite(sr, version_suite());
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    free_test_globals(&test_globals);
    return (number_failed == 0) ? 0 : 1;
}
