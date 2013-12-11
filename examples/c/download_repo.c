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
    LrYumRepoMd *repomd;
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

    lr_result_getinfo(r, &tmp_err, LRR_YUM_REPOMD, &repomd);
    if (!tmp_err) {
        printf("Repomd.xml contains:\n");
        for (GSList *elem = repomd->records; elem; elem = g_slist_next(elem)) {
            LrYumRepoMdRecord *rec = (LrYumRepoMdRecord *) elem->data;
            printf(" Type: %s\n", rec->type);
            printf(" Location href: %s\n", rec->location_href);
            if (rec->location_base)
                printf(" Location base: %s\n", rec->location_base);
            if (rec->checksum)
                printf(" Checksum: %s\n", rec->checksum);
            if (rec->checksum_type)
                printf(" Checksum type: %s\n", rec->checksum_type);
            if (rec->checksum_open)
                printf(" Checksum open: %s\n", rec->checksum_open);
            if (rec->checksum_open_type)
                printf(" Checksum open type: %s\n", rec->checksum_open_type);
            if (rec->timestamp > 0)
                printf(" Timestamp: %"G_GINT64_FORMAT"\n", rec->timestamp);
            if (rec->size > 0)
                printf(" Size: %"G_GINT64_FORMAT"\n", rec->size);
            if (rec->size_open > 0)
                printf(" Size open: %"G_GINT64_FORMAT"\n", rec->size_open);
            if (rec->db_version > 0)
                printf(" Db version: %d\n", rec->db_version);
            printf("------------------------------------------------\n");
        }
    } else {
        fprintf(stderr, "Error encountered: %s\n", tmp_err->message);
        g_error_free(tmp_err);
        rc = EXIT_FAILURE;
    }

    lr_result_free(r);
    lr_handle_free(h);

    return rc;
}
