#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <librepo/librepo.h>

#define URL "http://curl.haxx.se/libcurl/c/curl_easy_setopt.html"

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
    LrDownloadTarget *target;
    GError *tmp_err = NULL;

    // Setup logging (optional step)
    g_log_set_handler("librepo", G_LOG_LEVEL_ERROR |
                                 G_LOG_LEVEL_CRITICAL |
                                 G_LOG_LEVEL_DEBUG |
                                 G_LOG_LEVEL_WARNING,
                      log_handler_cb, NULL);

    // Download something
    GSList *possible_checksums = NULL;
/*
    LrDownloadTargetChecksum *chksm = lr_downloadtargetchecksum_new(
        LR_CHECKSUM_SHA256,
        "8d98b9e32651ae881c7d5bf7cf19ccaee3991bf0ca93b2ca58771c9df80bf077");

    possible_checksums = g_slist_prepend(possible_checksums, chksm);
*/

    target = lr_downloadtarget_new(
                NULL,   // LrHandle
                URL,    // Whole or relative part of URL
                NULL,   // Base URL use this in case of relative part in the prev argument
                -1,     // Target fd (opened of course). Note: 0 means stdout
                "downloaded.html",  // Target fn
                possible_checksums,   // List of possible checksums
                0,      // Expected size
                FALSE,  // Resume
                NULL,   // Progress callback
                NULL,   // User data for progress callback
                NULL,   // End callback
                NULL,   // Mirror failure callback
                NULL,   // User's data for the end and mirror failure callbacks
                0,      // Start of byterange
                3788,   // End of byterange
                NULL,   // Range string to download; Overrides Start and End of byterange
                FALSE,  // TRUE = Tell proxy server that we don't want to use cache for this request
                FALSE   // TRUE = This target is a zchunk file; FALSE = is not zchuk file
            );

    ret = lr_download_target(target, &tmp_err);
    if (!ret) {
        fprintf(stderr, "Error encountered: %d: %s\n",
                tmp_err->code, tmp_err->message);
        rc = EXIT_FAILURE;
        g_error_free(tmp_err);
    }

    // Check statuses
    printf("%s: %s\n", target->effectiveurl, target->err ? target->err : "OK");

    // Clean up
    lr_downloadtarget_free(target);

    return rc;
}
