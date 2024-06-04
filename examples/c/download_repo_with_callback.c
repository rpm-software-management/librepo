#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <librepo/librepo.h>

#define FEDORA_VERSION  "19"
#define FEDORA_BASEARCH "x86_64"

/* Logging handler. Uncomment if want to use it.
static void
log_handler_cb(const gchar *log_domain G_GNUC_UNUSED,
               GLogLevelFlags log_level G_GNUC_UNUSED,
               const gchar *message,
               gpointer user_data G_GNUC_UNUSED)
{
    g_print ("%s\n", message);
}
*/

static int
progress_callback(G_GNUC_UNUSED void *data,
                  double total_to_download,
                  double now_downloaded)
{
    printf("\r%f / %f", total_to_download, now_downloaded);
    fflush(stdout);
    return 0;
}

int
main(void)
{
    int rc = EXIT_SUCCESS;
    GError *tmp_err = NULL;

    // Setup logging (useful for debugging)
    //g_log_set_handler("librepo", G_LOG_LEVEL_ERROR |
    //                             G_LOG_LEVEL_CRITICAL |
    //                             G_LOG_LEVEL_DEBUG |
    //                             G_LOG_LEVEL_WARNING,
    //                  log_handler_cb, NULL);

    // Prepare list of variable substitutions
    LrUrlVars *urlvars = NULL;
    urlvars = lr_urlvars_set(urlvars, "releasever", FEDORA_VERSION);
    urlvars = lr_urlvars_set(urlvars, "basearch", FEDORA_BASEARCH);

    // Download only this metadata
    char *download_list[] = { "group_gz", "pkgtags", NULL };

    LrHandle *h = lr_handle_init();
    lr_handle_setopt(h, NULL, LRO_METALINKURL,
            "https://mirrors.fedoraproject.org/metalink?repo=updates-released-f$releasever&arch=$basearch");
    lr_handle_setopt(h, NULL, LRO_REPOTYPE, LR_YUMREPO);
    lr_handle_setopt(h, NULL, LRO_YUMDLIST, download_list);
    lr_handle_setopt(h, NULL, LRO_VARSUB, urlvars);
    lr_handle_setopt(h, NULL, LRO_PROGRESSCB, progress_callback);

    LrResult *r = lr_result_init();
    gboolean ret = lr_handle_perform(h, r, &tmp_err);
    printf("\n");

    char *destdir;
    lr_handle_getinfo(h, NULL, LRI_DESTDIR, &destdir);

    if (ret) {
        printf("Download successful (Destination dir: %s)\n", destdir);
    } else {
        fprintf(stderr, "Error encountered: %s\n", tmp_err->message);
        g_error_free(tmp_err);
        rc = EXIT_FAILURE;
    }

    lr_result_free(r);
    lr_handle_free(h);

    return rc;
}
