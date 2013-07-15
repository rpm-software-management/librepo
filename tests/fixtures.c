#define _XOPEN_SOURCE

#include <stdio.h>
#include <check.h>
#include <unistd.h>

#include "librepo/util.h"
#include "fixtures.h"
#include "testsys.h"


/* define the global variable */
struct TestGlobals_s test_globals;

void
test_log_handler_cb(const gchar *log_domain, GLogLevelFlags log_level,
                    const gchar *message, gpointer user_data)
{
    time_t rawtime;
    char buffer[255];
    gchar *level = "";

    LR_UNUSED(log_domain);
    LR_UNUSED(user_data);

    switch(log_level) {
        case G_LOG_LEVEL_ERROR:     level = "ERROR"; break;
        case G_LOG_LEVEL_CRITICAL:  level = "CRITICAL"; break;
        case G_LOG_LEVEL_WARNING:   level = "WARNING"; break;
        case G_LOG_LEVEL_MESSAGE:   level = "MESSAGE"; break;
        case G_LOG_LEVEL_INFO:      level = "INFO"; break;
        case G_LOG_LEVEL_DEBUG:     level = "DEBUG"; break;
        default:                    level = "UNKNOWN"; break;
    }

    time(&rawtime);
    strftime(buffer, 254, "%H:%M:%S", localtime(&rawtime));
    g_printerr("%s: %s %s\n", buffer, level, message);
}
