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
main(void)
{
    int rc = EXIT_SUCCESS;
    gboolean ret;
    LrHandle *h;
    GSList *packages = NULL;
    LrPackageTarget *target;
    GError *tmp_err = NULL;

    // Setup logging (optional step)

    g_log_set_handler("librepo", G_LOG_LEVEL_ERROR |
                                 G_LOG_LEVEL_CRITICAL |
                                 G_LOG_LEVEL_DEBUG |
                                 G_LOG_LEVEL_WARNING,
                      log_handler_cb, NULL);

    // Prepare handle

    char *urls[] = {"http://beaker-project.org/yum/client-testing/Fedora19/", NULL};

    h = lr_handle_init();
    lr_handle_setopt(h, NULL, LRO_URLS, urls);
    lr_handle_setopt(h, NULL, LRO_REPOTYPE, LR_YUMREPO);

    // Prepare list of targets

    // This target has enabled resume and filled checksum.
    // So if package would localy exists and checksum matches, then
    // no download was performed.
    // Alternatively if you used "truncate -s 1000 beaker-0.14.0-1.fc18.src.rpm"
    // and truncate the package to 1000 bytes. Then during download, no whole
    // package will be downloaded, but download will resume from the 1001st byte
    target = lr_packagetarget_new(h, "beaker-0.14.0-1.fc18.src.rpm",
                                  NULL, LR_CHECKSUM_SHA256,
                                  "737c974110914a073fb6c736cd7021b0d844c9e47e7d21e37d687dbc86d36538",
                                  0, NULL, TRUE, NULL, NULL, &tmp_err);
    packages = g_slist_append(packages, target);

    target = lr_packagetarget_new(h, "beaker-client-0.14.1-1.fc18.noarch.rpm",
                                  NULL, LR_CHECKSUM_UNKNOWN, NULL, 0,
                                  NULL, FALSE, NULL, NULL, &tmp_err);
    packages = g_slist_append(packages, target);

    target = lr_packagetarget_new(h, "rhts-4.56-1.fc17.src.rpm",
                                  NULL, LR_CHECKSUM_UNKNOWN, NULL, 0,
                                  NULL, FALSE, NULL, NULL, &tmp_err);
    packages = g_slist_append(packages, target);

    // Download all targets

    // Note that failfast feature is enabled. This means that if any of
    // downloads will fail, then lr_download_package returns immediately
    // with an error.
    // If the failfast is disabled, then lr_download_packages doesn't returns
    // sooner then all downloads are finished (no matter if successfully
    // or unsuccessfully) or some critical error is meet.
    ret = lr_download_packages(packages,
                               LR_PACKAGEDOWNLOAD_FAILFAST,
                               &tmp_err);
    if (!ret) {
        fprintf(stderr, "Error encountered: %d: %s\n",
                tmp_err->code, tmp_err->message);
        rc = EXIT_FAILURE;
    }

    // Check statuses

    for (GSList *elem = packages; elem; elem = g_slist_next(elem)) {
        LrPackageTarget *target = elem->data;
        printf("%s: %s\n", target->local_path, target->err ? target->err : "OK");
    }

    // Clean up

    g_slist_free_full(packages, (GDestroyNotify) lr_packagetarget_free);
    lr_handle_free(h);

    return rc;
}
