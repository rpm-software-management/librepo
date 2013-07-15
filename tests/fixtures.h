#ifndef LR_FIXTURES_H
#define LR_FIXTURES_H

#include <glib.h>

struct TestGlobals_s {
    char *testdata_dir;
    char *tmpdir;
};

/* global data used to pass values from fixtures to tests */
extern struct TestGlobals_s test_globals;

void
test_log_handler_cb(const gchar *log_domain, GLogLevelFlags log_level,
                    const gchar *message, gpointer user_data);

#endif /* FIXTURES_H */
