#ifndef LR_FIXTURES_H
#define LR_FIXTURES_H


struct TestGlobals_s {
    char *repo_dir;
    char *tmpdir;
};


/* global data used to pass values from fixtures to tests */

extern struct TestGlobals_s test_globals;

/*
void fixture_greedy_only(void);
void fixture_system_only(void);
void fixture_with_main(void);
void fixture_with_updates(void);
void fixture_all(void);
void fixture_yum(void);
void setup_yum_sack(HySack sack, const char *yum_repo_name);
void teardown(void);
*/

#endif /* FIXTURES_H */
