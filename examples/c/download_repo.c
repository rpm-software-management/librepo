#include <stdlib.h>
#include <stdio.h>
#include <librepo/librepo.h>

int
main(void)
{
    int ret = EXIT_SUCCESS, rc;
    lr_Handle h;
    lr_Result r;
    char *download_list[] = LR_YUM_HAWKEY;

    lr_global_init();

    h = lr_handle_init();
    r = lr_result_init();

    lr_handle_setopt(h, LRO_URL, "http://beaker-project.org/yum/client-testing/Fedora19/");
    lr_handle_setopt(h, LRO_REPOTYPE, LR_YUMREPO);
    lr_handle_setopt(h, LRO_YUMDLIST, download_list);

    rc = lr_handle_perform(h, r);
    if (rc != LRE_OK) {
        fprintf(stderr, "Error encountered: %d (%s)\n", rc, lr_strerror(rc));
        ret = EXIT_FAILURE;
    }

    lr_result_free(r);
    lr_handle_free(h);
    lr_global_cleanup();

    return ret;
}
