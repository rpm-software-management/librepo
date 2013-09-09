#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <librepo/librepo.h>

int
main(void)
{
    int rc = EXIT_SUCCESS;
    gboolean ret;
    LrHandle *h;
    LrResult *r;
    char *download_list[] = LR_YUM_HAWKEY;
    GError *tmp_err = NULL;

    h = lr_handle_init();
    r = lr_result_init();

    char *urls[] = {"http://beaker-project.org/yum/client-testing/Fedora19/", NULL};

    lr_handle_setopt(h, NULL, LRO_URLS, urls);
    lr_handle_setopt(h, NULL, LRO_REPOTYPE, LR_YUMREPO);
    lr_handle_setopt(h, NULL, LRO_YUMDLIST, download_list);

    ret = lr_handle_perform(h, r, &tmp_err);
    if (!ret) {
        fprintf(stderr, "Error encountered: %d: %s\n",
                tmp_err->code, tmp_err->message);
        g_error_free(tmp_err);
        rc = EXIT_FAILURE;
    }

    lr_result_free(r);
    lr_handle_free(h);

    return rc;
}
