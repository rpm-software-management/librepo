#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <librepo/librepo.h>

static void
log_handler_cb(const gchar *log_domain G_GNUC_UNUSED,
               GLogLevelFlags log_level G_GNUC_UNUSED,
               const gchar *message,
               gpointer user_data G_GNUC_UNUSED)
{
    g_print ("%s\n", message);
}

int
main(int argc, char *argv[])
{
    int rc = EXIT_SUCCESS;
    GSList *list = NULL;
    GError *tmp_err = NULL;

    g_log_set_handler("librepo", G_LOG_LEVEL_ERROR |
                                 G_LOG_LEVEL_CRITICAL |
                                 G_LOG_LEVEL_DEBUG |
                                 G_LOG_LEVEL_WARNING,
                      log_handler_cb, NULL);

    if (argc < 2) {
        g_printerr("Usage: %s <mirror_1> <mirror_2> ...\n", argv[0]);
        return EXIT_FAILURE;
    }

    for (int x = 1; x < argc; x++)
        list = g_slist_prepend(list, argv[x]);
    list = g_slist_reverse(list);

    gboolean ret = lr_fastestmirror(NULL, &list, &tmp_err);
    if (!ret) {
        g_printerr("Error encountered: %s\n", tmp_err->message);
        g_error_free(tmp_err);
        rc = EXIT_FAILURE;
    }

    for (GSList *elem = list; elem; elem = g_slist_next(elem)) {
        gchar *url = elem->data;
        g_print("%s\n", url);
    }

    g_slist_free(list);

    return rc;
}
