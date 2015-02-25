#ifndef LR_FIXTURES_H
#define LR_FIXTURES_H

#include <glib.h>
#include <check.h>

struct TestGlobals_s {
    char *testdata_dir;
    char *tmpdir;
};

/* global data used to pass values from fixtures to tests */
extern struct TestGlobals_s test_globals;

void
test_log_handler_cb(const gchar *log_domain, GLogLevelFlags log_level,
                    const gchar *message, gpointer user_data);

void
lr_assert_strv_eq(const gchar * const *strv, ...);


#if (CHECK_MAJOR_VERSION == 0) && (CHECK_MINOR_VERSION == 9) && (CHECK_MICRO_VERSION <= 9)
#define ck_assert_msg(exp, ...) lr_assert_msg((int) (exp))
void lr_assert_msg(int exp);
#endif

#endif /* FIXTURES_H */
