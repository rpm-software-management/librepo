#ifndef LR_FIXTURES_H
#define LR_FIXTURES_H

struct TestGlobals_s {
    char *testdata_dir;
    char *tmpdir;
};

/* global data used to pass values from fixtures to tests */
extern struct TestGlobals_s test_globals;

#endif /* FIXTURES_H */
