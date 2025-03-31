#include <glib.h>
#include <librepo/librepo.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define DESTDIR "downloaded_metadata"
#define PROGRESSBAR_LEN 50

static int progress_callback(void *data, double total_to_download,
                             double downloaded) {
  if (total_to_download <= 0) {
    return 0;
  }
  int completed = downloaded / (total_to_download / PROGRESSBAR_LEN);

  printf("[%.*s%.*s] %.f/%.f (%s)\n", completed,
         "##################################################", // '#' repeated
         PROGRESSBAR_LEN - completed,
         "--------------------------------------------------", // '-' repeated
         downloaded, total_to_download, (const char *)data);

  fflush(stdout);
  return 0;
}

static int end_callback(void *data, LrTransferStatus status, const char *msg) {
  printf("End status %u: %s (%s)\n", status, msg, (const char *)data);
  return 0;
}

static int hmf_callback(void *data, const char *msg, const char *url) {
  printf("%s: %s (%s)\n", msg, url, (const char *)data);
  return 0;
}

LrHandle *create_handle(const char *repo) {
  // Handle represents a download configuration
  LrHandle *h = lr_handle_init();

  // --- Mandatory arguments -------------------------------------------
  gchar *metalink =
      g_strconcat("https://mirrors.fedoraproject.org/metalink?repo=", repo,
                  "&arch=x86_64", NULL);
  lr_handle_setopt(h, NULL, LRO_METALINKURL, metalink);
  g_free(metalink);
  // Type of repository
  lr_handle_setopt(h, NULL, LRO_REPOTYPE, LR_YUMREPO);

  // --- Optional arguments --------------------------------------------
  // Make download interruptible
  lr_handle_setopt(h, NULL, LRO_INTERRUPTIBLE, true);
  // Destination directory for metadata
  gchar *repo_destdir = g_strconcat(DESTDIR, "/", repo, NULL);
  lr_handle_setopt(h, NULL, LRO_DESTDIR, repo_destdir);
  g_free(repo_destdir);
  // Check checksum of all files (if checksum is available in repomd.xml)
  lr_handle_setopt(h, NULL, LRO_CHECKSUM, true);
  // Download only primary.xml, comps.xml and updateinfo
  // Note: repomd.xml is downloaded implicitly!
  // Note: If LRO_YUMDLIST is None -> all files are downloaded
  char *download_list[] = {"primary", "filelists", "group", "updateinfo", NULL};
  lr_handle_setopt(h, NULL, LRO_YUMDLIST, download_list);

  return h;
}

int main(void) {
  int rc = EXIT_SUCCESS;
  GError *tmp_err = NULL;
  // Prepare destination directory
  struct stat buffer;
  if (stat(DESTDIR, &buffer) == 0) {
    system("rm -rf " DESTDIR);
  }
  mkdir(DESTDIR, 0777);

  const char *cbdata1 = "Callback data from target 1";
  LrHandle *h1 = create_handle("fedora-41");
  LrMetadataTarget *m1 =
      lr_metadatatarget_new2(h1, (void *)cbdata1, progress_callback,
                             hmf_callback, end_callback, NULL, &tmp_err);
  const char *cbdata2 = "Callback data from target 2";
  LrHandle *h2 = create_handle("updates-released-f41");
  LrMetadataTarget *m2 =
      lr_metadatatarget_new2(h2, (void *)cbdata2, progress_callback,
                             hmf_callback, end_callback, NULL, &tmp_err);

  GSList *metadata_targets = NULL;
  metadata_targets = g_slist_append(metadata_targets, m1);
  metadata_targets = g_slist_append(metadata_targets, m2);

  if (!lr_download_metadata(metadata_targets, &tmp_err)) {
    if (tmp_err != NULL) {
      fprintf(stderr, "Error encountered: %s\n", tmp_err->message);
      g_error_free(tmp_err);
    } else {
      fprintf(stderr, "Unknown error encountered\n");
    }
    rc = EXIT_FAILURE;
  }

  if (m1->err) {
    for (GList *elem = m1->err; elem; elem = g_list_next(elem)) {
      const char *msg = elem->data;
      fprintf(stderr, "Error encountered: %s\n", msg);
    }
    rc = EXIT_FAILURE;
  }
  if (m2->err) {
    for (GList *elem = m2->err; elem; elem = g_list_next(elem)) {
      const char *msg = elem->data;
      fprintf(stderr, "Error encountered: %s\n", msg);
    }
    rc = EXIT_FAILURE;
  }

  lr_handle_free(h1);
  lr_handle_free(h2);
  g_slist_free_full(metadata_targets, (GDestroyNotify)lr_metadatatarget_free);

  return rc;
}
