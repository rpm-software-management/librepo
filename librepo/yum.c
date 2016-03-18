/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2012  Tomas Mlcoch
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#define _POSIX_SOURCE
#define _BSD_SOURCE

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "util.h"
#include "metalink.h"
#include "mirrorlist.h"
#include "repomd.h"
#include "downloader.h"
#include "checksum.h"
#include "handle_internal.h"
#include "result_internal.h"
#include "yum_internal.h"
#include "gpg.h"
#include "cleanup.h"

/* helper functions for YumRepo manipulation */

LrYumRepo *
lr_yum_repo_init()
{
    return lr_malloc0(sizeof(LrYumRepo));
}

void
lr_yum_repo_free(LrYumRepo *repo)
{
    if (!repo)
        return;

    for (GSList *elem = repo->paths; elem; elem = g_slist_next(elem)) {
        LrYumRepoPath *yumrepopath = elem->data;
        assert(yumrepopath);
        lr_free(yumrepopath->type);
        lr_free(yumrepopath->path);
        lr_free(yumrepopath);
    }

    g_slist_free(repo->paths);
    lr_free(repo->repomd);
    lr_free(repo->url);
    lr_free(repo->destdir);
    lr_free(repo->signature);
    lr_free(repo->mirrorlist);
    lr_free(repo->metalink);
    lr_free(repo);
}

const char *
lr_yum_repo_path(LrYumRepo *repo, const char *type)
{
    assert(repo);
    for (GSList *elem = repo->paths; elem; elem = g_slist_next(elem)) {
        LrYumRepoPath *yumrepopath = elem->data;
        assert(yumrepopath);
        if (!strcmp(yumrepopath->type, type))
            return yumrepopath->path;
    }
    return NULL;
}

/** Append path to the repository object.
 * @param repo          Yum repo object.
 * @param type          Type of file. E.g. "primary", "filelists", ...
 * @param path          Path to the file.
 */
static void
lr_yum_repo_append(LrYumRepo *repo, const char *type, const char *path)
{
    assert(repo);
    assert(type);
    assert(path);

    LrYumRepoPath *yumrepopath = lr_malloc(sizeof(LrYumRepoPath));
    yumrepopath->type = g_strdup(type);
    yumrepopath->path = g_strdup(path);
    repo->paths = g_slist_append(repo->paths, yumrepopath);
}

static void
lr_yum_repo_update(LrYumRepo *repo, const char *type, const char *path)
{
    assert(repo);
    assert(type);
    assert(path);

    for (GSList *elem = repo->paths; elem; elem = g_slist_next(elem)) {
        LrYumRepoPath *yumrepopath = elem->data;
        assert(yumrepopath);

        if (!strcmp(yumrepopath->type, type)) {
            lr_free(yumrepopath->path);
            yumrepopath->path = g_strdup(path);
            return;
        }
    }

    lr_yum_repo_append(repo, type, path);
}

/* main bussines logic */

static gboolean
lr_yum_repomd_record_enabled(LrHandle *handle, const char *type)
{
    // Blacklist check
    if (handle->yumblist) {
        int x = 0;
        while (handle->yumblist[x]) {
            if (!strcmp(handle->yumblist[x], type))
                return FALSE;
            x++;
        }
    }

    // Whitelist check
    if (handle->yumdlist) {
        int x = 0;
        while (handle->yumdlist[x]) {
            if (!strcmp(handle->yumdlist[x], type))
                return TRUE;
            x++;
        }
        return FALSE;
    }

    return TRUE;
}

/** Mirror Failure Callback Data
 */
typedef struct CbData_s {
    void *userdata;                 /*!< User data */
    LrProgressCb progresscb;        /*!< Progress callback */
    LrHandleMirrorFailureCb hmfcb;  /*!< Handle mirror failure callback */
    char *metadata;                 /*!< "primary", "filelists", ... */
} CbData;

static CbData *cbdata_new(void *userdata,
                          LrProgressCb progresscb,
                          LrHandleMirrorFailureCb hmfcb,
                          const char *metadata)
{
    CbData *data = calloc(1, sizeof(*data));
    data->userdata = userdata;
    data->progresscb = progresscb;
    data->hmfcb = hmfcb;
    data->metadata = g_strdup(metadata);
    return data;
}

static void
cbdata_free(CbData *data)
{
    if (!data) return;
    free(data->metadata);
    free(data);
}

static int
progresscb(void *clientp, double total_to_download, double downloaded)
{
    CbData *data = clientp;
    if (data->progresscb)
        return data->progresscb(data->userdata, total_to_download, downloaded);
    return LR_CB_OK;
}

static int
hmfcb(void *clientp, const char *msg, const char *url)
{
    CbData *data = clientp;
    if (data->hmfcb)
        return data->hmfcb(data->userdata, msg, url, data->metadata);
    return LR_CB_OK;
}

static gboolean
lr_yum_download_repomd(LrHandle *handle,
                       LrMetalink *metalink,
                       int fd,
                       GError **err)
{
    int ret = TRUE;
    GError *tmp_err = NULL;

    assert(!err || *err == NULL);

    g_debug("%s: Downloading repomd.xml via mirrorlist", __func__);

    GSList *checksums = NULL;
    if (metalink && (handle->checks & LR_CHECK_CHECKSUM)) {
        // Select best checksum

        gboolean ret;
        LrChecksumType ch_type;
        gchar *ch_value;

        // From the metalink itself
        ret = lr_best_checksum(metalink->hashes, &ch_type, &ch_value);
        if (ret) {
            LrDownloadTargetChecksum *dtch;
            dtch = lr_downloadtargetchecksum_new(ch_type, ch_value);
            checksums = g_slist_prepend(checksums, dtch);
            g_debug("%s: Expected checksum for repomd.xml: (%s) %s",
                    __func__, lr_checksum_type_to_str(ch_type), ch_value);
        }

        // From the alternates entries
        for (GSList *elem = metalink->alternates; elem; elem = g_slist_next(elem)) {
            LrMetalinkAlternate *alt = elem->data;
            ret = lr_best_checksum(alt->hashes, &ch_type, &ch_value);
            if (ret) {
                LrDownloadTargetChecksum *dtch;
                dtch = lr_downloadtargetchecksum_new(ch_type, ch_value);
                checksums = g_slist_prepend(checksums, dtch);
                g_debug("%s: Expected alternate checksum for repomd.xml: (%s) %s",
                        __func__, lr_checksum_type_to_str(ch_type), ch_value);
            }
        }
    }

    CbData *cbdata = NULL;
    if (handle->hmfcb) {
        cbdata = cbdata_new(handle->user_data,
                            NULL,
                            handle->hmfcb,
                            "repomd.xml");
    }

    LrDownloadTarget *target = lr_downloadtarget_new(handle,
                                                     "repodata/repomd.xml",
                                                     NULL,
                                                     fd,
                                                     NULL,
                                                     checksums,
                                                     0,
                                                     0,
                                                     NULL,
                                                     cbdata,
                                                     NULL,
                                                     (cbdata) ? hmfcb : NULL,
                                                     NULL,
                                                     0,
                                                     0,
                                                     TRUE);

    ret = lr_download_target(target, &tmp_err);
    assert((ret && !tmp_err) || (!ret && tmp_err));

    if (cbdata)
        cbdata_free(cbdata);

    if (tmp_err) {
        g_propagate_prefixed_error(err, tmp_err,
                                   "Cannot download repomd.xml: ");
    } else if (target->err) {
        assert(0); // This should not happen since failfast should be TRUE
        ret = FALSE;
        g_set_error(err, LR_DOWNLOADER_ERROR, target->rcode,
                    "Cannot download repomd.xml: %s",target->err);
    } else {
        // Set mirror used for download a repomd.xml to the handle
        // TODO: Get rid of use_mirror attr
        lr_free(handle->used_mirror);
        handle->used_mirror = g_strdup(target->usedmirror);
    }

    lr_downloadtarget_free(target);

    if (!ret) {
        /* Download of repomd.xml was not successful */
        g_debug("%s: repomd.xml download was unsuccessful", __func__);
    }

    return ret;
}

static gboolean
lr_yum_download_repo(LrHandle *handle,
                     LrYumRepo *repo,
                     LrYumRepoMd *repomd,
                     GError **err)
{
    gboolean ret = TRUE;
    char *destdir;  /* Destination dir */
    GSList *targets = NULL;
    GSList *cbdata_list = NULL;
    GError *tmp_err = NULL;

    destdir = handle->destdir;
    assert(destdir);
    assert(strlen(destdir));
    assert(!err || *err == NULL);

    for (GSList *elem = repomd->records; elem; elem = g_slist_next(elem)) {
        int fd;
        char *path;
        LrDownloadTarget *target;
        LrYumRepoMdRecord *record = elem->data;
        CbData *cbdata = NULL;

        assert(record);

        if (!lr_yum_repomd_record_enabled(handle, record->type))
            continue;

        path = lr_pathconcat(destdir, record->location_href, NULL);
        fd = open(path, O_CREAT|O_TRUNC|O_RDWR, 0666);
        if (fd < 0) {
            g_debug("%s: Cannot create/open %s (%s)",
                    __func__, path, g_strerror(errno));
            g_set_error(err, LR_YUM_ERROR, LRE_IO,
                        "Cannot create/open %s: %s", path, g_strerror(errno));
            lr_free(path);
            g_slist_free_full(targets, (GDestroyNotify) lr_downloadtarget_free);
            return FALSE;
        }

        GSList *checksums = NULL;
        if (handle->checks & LR_CHECK_CHECKSUM) {
            // Select proper checksum type only if checksum check is enabled
            LrDownloadTargetChecksum *checksum;
            checksum = lr_downloadtargetchecksum_new(
                                    lr_checksum_type(record->checksum_type),
                                    record->checksum);
            checksums = g_slist_prepend(checksums, checksum);
        }

        if (handle->user_cb || handle->hmfcb) {
            cbdata = cbdata_new(handle->user_data,
                                handle->user_cb,
                                handle->hmfcb,
                                record->type);
            cbdata_list = g_slist_append(cbdata_list, cbdata);
        }

        target = lr_downloadtarget_new(handle,
                                       record->location_href,
                                       record->location_base,
                                       fd,
                                       NULL,
                                       checksums,
                                       0,
                                       0,
                                       NULL,
                                       cbdata,
                                       NULL,
                                       NULL,
                                       NULL,
                                       0,
                                       0,
                                       FALSE);

        targets = g_slist_append(targets, target);

        /* Because path may already exists in repo (while update) */
        lr_yum_repo_update(repo, record->type, path);
        lr_free(path);
    }

    if (!targets)
        return TRUE;

    ret = lr_download_single_cb(targets,
                                FALSE,
                                (cbdata_list) ? progresscb : NULL,
                                (cbdata_list) ? hmfcb : NULL,
                                &tmp_err);

    // Error handling

    assert((ret && !tmp_err) || (!ret && tmp_err));

    if (tmp_err) {
        g_propagate_prefixed_error(err, tmp_err,
                                   "Downloading error: ");
    } else {
        int code = LRE_OK;
        char *error_summary = NULL;

        for (GSList *elem = targets; elem; elem = g_slist_next(elem)) {
            LrDownloadTarget *target = elem->data;
            if (target->rcode != LRE_OK) {
                if (code == LRE_OK) {
                    // First failed download target found
                    code = target->rcode;
                    error_summary = g_strconcat(target->path,
                                                " - ",
                                                target->err,
                                                NULL);
                } else {
                    error_summary = g_strconcat(error_summary,
                                                "; ",
                                                target->path,
                                                " - ",
                                                target->err,
                                                NULL);
                }
            }

            close(target->fd);
        }

        if (code != LRE_OK) {
            // At least one target failed
            ret = FALSE;
            g_set_error(err, LR_DOWNLOADER_ERROR, code,
                        "Downloading error(s): %s", error_summary);
            g_free(error_summary);
        }
    }

    g_slist_free_full(cbdata_list, (GDestroyNotify)cbdata_free);
    g_slist_free_full(targets, (GDestroyNotify)lr_downloadtarget_free);

    return ret;
}

static gboolean
lr_yum_check_checksum_of_md_record(LrYumRepoMdRecord *rec,
                                   const char *path,
                                   GError **err)
{
    int fd;
    char *expected_checksum;
    LrChecksumType checksum_type;
    gboolean ret, matches;
    GError *tmp_err = NULL;

    assert(!err || *err == NULL);

    if (!rec || !path)
        return TRUE;

    expected_checksum = rec->checksum;
    checksum_type = lr_checksum_type(rec->checksum_type);

    g_debug("%s: Checking checksum of %s (expected: %s [%s])",
                       __func__, path, expected_checksum, rec->checksum_type);

    if (!expected_checksum) {
        // Empty checksum - suppose it's ok
        g_debug("%s: No checksum in repomd", __func__);
        return TRUE;
    }

    if (checksum_type == LR_CHECKSUM_UNKNOWN) {
        g_debug("%s: Unknown checksum: %s", __func__, rec->checksum_type);
        g_set_error(err, LR_YUM_ERROR, LRE_UNKNOWNCHECKSUM,
                    "Unknown checksum type \"%s\" for %s",
                    rec->checksum_type, path);
        return FALSE;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        g_debug("%s: Cannot open %s", __func__, path);
        g_set_error(err, LR_YUM_ERROR, LRE_IO,
                    "Cannot open %s: %s", path, g_strerror(errno));
        return FALSE;
    }

    ret = lr_checksum_fd_cmp(checksum_type,
                             fd,
                             expected_checksum,
                             1,
                             &matches,
                             &tmp_err);

    close(fd);

    assert(ret || tmp_err);

    if (!ret) {
        // Checksum calculation error
        g_debug("%s: Checksum check %s - Error: %s",
                __func__, path, tmp_err->message);
        g_propagate_prefixed_error(err, tmp_err,
                                   "Checksum error %s: ", path);
        return FALSE;
    } else if (!matches) {
        g_debug("%s: Checksum check %s - Mismatch", __func__, path);
        g_set_error(err, LR_YUM_ERROR, LRE_BADCHECKSUM,
                    "Checksum mismatch %s", path);
        return FALSE;
    }

    g_debug("%s: Checksum check - Passed", __func__);

    return TRUE;
}

static gboolean
lr_yum_check_repo_checksums(LrYumRepo *repo,
                            LrYumRepoMd *repomd,
                            GError **err)
{
    assert(!err || *err == NULL);

    for (GSList *elem = repomd->records; elem; elem = g_slist_next(elem)) {
        gboolean ret;
        LrYumRepoMdRecord *record = elem->data;

        assert(record);

        const char *path = lr_yum_repo_path(repo, record->type);
        ret = lr_yum_check_checksum_of_md_record(record, path, err);
        if (!ret)
            return FALSE;
    }

    return TRUE;
}

static gboolean
lr_yum_use_local_load_base(LrHandle *handle,
                           LrResult *result,
                           LrYumRepo *repo,
                           LrYumRepoMd *repomd,
                           const gchar *baseurl,
                           GError **err)
{
    gboolean ret;
    GError *tmp_err = NULL;
    _cleanup_free_ gchar *path = NULL;
    _cleanup_free_ gchar *sig = NULL;
    _cleanup_file_close_ int fd = -1;

    if (handle->mirrorlist_fd != -1) {
        // Locate mirrorlist if available.
        gchar *mrl_fn = lr_pathconcat(baseurl, "mirrorlist", NULL);
        if (g_file_test(mrl_fn, G_FILE_TEST_IS_REGULAR)) {
            g_debug("%s: Found local mirrorlist: %s", __func__, mrl_fn);
            repo->mirrorlist = mrl_fn;
        } else {
            repo->mirrorlist = NULL;
            lr_free(mrl_fn);
        }
    }

    if (handle->metalink_fd != -1) {
        // Locate metalink.xml if available.
        gchar *mtl_fn = lr_pathconcat(baseurl, "metalink.xml", NULL);
        if (g_file_test(mtl_fn, G_FILE_TEST_IS_REGULAR)) {
            g_debug("%s: Found local metalink: %s", __func__, mtl_fn);
            repo->metalink = mtl_fn;
        } else {
            repo->metalink = NULL;
            lr_free(mtl_fn);
        }
    }

    // Open repomd.xml
    path = lr_pathconcat(baseurl, "repodata/repomd.xml", NULL);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        g_debug("%s: open(%s): %s", __func__, path, g_strerror(errno));
        g_set_error(err, LR_YUM_ERROR, LRE_IO,
                    "Cannot open %s: %s", path, g_strerror(errno));
        return FALSE;
    }

    // Parse repomd.xml
    g_debug("%s: Parsing repomd.xml", __func__);
    ret = lr_yum_repomd_parse_file(repomd, fd, lr_xml_parser_warning_logger,
                                   "Repomd xml parser", &tmp_err);
    if (!ret) {
        g_debug("%s: Parsing unsuccessful: %s", __func__, tmp_err->message);
        g_propagate_prefixed_error(err, tmp_err,
                                   "repomd.xml parser error: ");
        return FALSE;
    }

    // Fill result object
    result->destdir = g_strdup(baseurl);
    repo->destdir = g_strdup(baseurl);
    repo->repomd = g_strdup(path);

    // Check if signature file exists
    sig = lr_pathconcat(baseurl, "repodata/repomd.xml.asc", NULL);
    if (access(sig, F_OK) == 0)
        repo->signature = g_strdup(sig);

    // Signature checking
    if (handle->checks & LR_CHECK_GPG) {

        if (!repo->signature) {
            // Signature doesn't exist
            g_debug("%s: GPG signature doesn't exists", __func__);
            g_set_error(err, LR_YUM_ERROR, LRE_BADGPG,
                        "GPG verification is enabled, but GPG signature "
                        "repomd.xml.asc is not available");
            return FALSE;
        }

        ret = lr_gpg_check_signature(repo->signature,
                                     repo->repomd,
                                     handle->gnupghomedir,
                                     &tmp_err);
        if (!ret) {
            g_debug("%s: repomd.xml GPG signature verification failed: %s",
                    __func__, tmp_err->message);
            g_propagate_prefixed_error(err, tmp_err,
                        "repomd.xml GPG signature verification failed: ");
            return FALSE;
        }
    }

    // Done - repomd is loaded and checked
    g_debug("%s: Repomd revision: %s", __func__, repomd->revision);

    return TRUE;
}

/* Do not duplicate repoata, just locate the local one */
static gboolean
lr_yum_use_local(LrHandle *handle, LrResult *result, GError **err)
{
    char *baseurl;
    LrYumRepo *repo;
    LrYumRepoMd *repomd;

    assert(!err || *err == NULL);

    g_debug("%s: Locating repo..", __func__);

    // Shortcuts
    repo   = result->yum_repo;
    repomd = result->yum_repomd;
    baseurl = handle->urls[0];

    // Skip "file://" prefix if present
    if (g_str_has_prefix(baseurl, "file://"))
        baseurl += 7;
    else if (g_str_has_prefix(baseurl, "file:"))
        baseurl += 5;

    // Check sanity
    if (strstr(baseurl, "://")) {
        g_set_error(err, LR_YUM_ERROR, LRE_NOTLOCAL,
                    "URL: %s doesn't seem to be a local repository",
                    baseurl);
        return FALSE;
    }

    if (!handle->update) {
        // Load repomd.xml and mirrorlist+metalink if locally available
        if (!lr_yum_use_local_load_base(handle, result, repo, repomd, baseurl, err))
            return FALSE;
    }

    // Locate rest of metadata files
    for (GSList *elem = repomd->records; elem; elem = g_slist_next(elem)) {
        _cleanup_free_ char *path = NULL;
        LrYumRepoMdRecord *record = elem->data;

        assert(record);

        if (!lr_yum_repomd_record_enabled(handle, record->type))
            continue; // Caller isn't interested in this record type
        if (lr_yum_repo_path(repo, record->type))
            continue; // This path already exists in repo

        path = lr_pathconcat(baseurl, record->location_href, NULL);
        if (access(path, F_OK) == -1) {
            // A repo file is missing
            if (!handle->ignoremissing) {
                g_debug("%s: Incomplete repository - %s is missing",
                        __func__, path);
                g_set_error(err, LR_YUM_ERROR, LRE_INCOMPLETEREPO,
                            "Incomplete repository - %s is missing",
                            path);
                return FALSE;
            }

            continue;
        }

        lr_yum_repo_append(repo, record->type, path);
    }

    g_debug("%s: Repository was successfully located", __func__);
    return TRUE;
}

static gboolean
lr_yum_download_remote(LrHandle *handle, LrResult *result, GError **err)
{
    int rc;
    gboolean ret = TRUE;
    int fd;
    int create_repodata_dir = 1;
    char *path_to_repodata;
    LrYumRepo *repo;
    LrYumRepoMd *repomd;
    GError *tmp_err = NULL;

    assert(!err || *err == NULL);

    repo   = result->yum_repo;
    repomd = result->yum_repomd;

    g_debug("%s: Downloading/Copying repo..", __func__);

    path_to_repodata = lr_pathconcat(handle->destdir, "repodata", NULL);

    if (handle->update) {  /* Check if should create repodata/ subdir */
        struct stat buf;
        if (stat(path_to_repodata, &buf) != -1)
            if (S_ISDIR(buf.st_mode))
                create_repodata_dir = 0;
    }

    if (create_repodata_dir) {
        /* Prepare repodata/ subdir */
        rc = mkdir(path_to_repodata, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
        if (rc == -1) {
            g_debug("%s: Cannot create dir: %s (%s)",
                    __func__, path_to_repodata, g_strerror(errno));
            g_set_error(err, LR_YUM_ERROR, LRE_CANNOTCREATEDIR,
                        "Cannot create directory: %s: %s",
                        path_to_repodata, g_strerror(errno));
            lr_free(path_to_repodata);
            return FALSE;
        }
    }
    lr_free(path_to_repodata);

    if (!handle->update) {
        char *path;

        /* Store mirrorlist file(s) */
        if (handle->mirrorlist_fd != -1) {
            char *ml_file_path = lr_pathconcat(handle->destdir,
                                               "mirrorlist", NULL);
            fd = open(ml_file_path, O_CREAT|O_TRUNC|O_RDWR, 0666);
            if (fd < 0) {
                g_debug("%s: Cannot create: %s", __func__, ml_file_path);
                g_set_error(err, LR_YUM_ERROR, LRE_IO,
                        "Cannot create %s: %s", ml_file_path, g_strerror(errno));
                lr_free(ml_file_path);
                return FALSE;
            }
            rc = lr_copy_content(handle->mirrorlist_fd, fd);
            close(fd);
            if (rc != 0) {
                g_debug("%s: Cannot copy content of mirrorlist file", __func__);
                g_set_error(err, LR_YUM_ERROR, LRE_IO,
                        "Cannot copy content of mirrorlist file %s: %s",
                        ml_file_path, g_strerror(errno));
                lr_free(ml_file_path);
                return FALSE;
            }
            repo->mirrorlist = ml_file_path;
        }

        if (handle->metalink_fd != -1) {
            char *ml_file_path = lr_pathconcat(handle->destdir,
                                               "metalink.xml", NULL);
            fd = open(ml_file_path, O_CREAT|O_TRUNC|O_RDWR, 0666);
            if (fd < 0) {
                g_debug("%s: Cannot create: %s", __func__, ml_file_path);
                g_set_error(err, LR_YUM_ERROR, LRE_IO,
                        "Cannot create %s: %s", ml_file_path, g_strerror(errno));
                lr_free(ml_file_path);
                return FALSE;
            }
            rc = lr_copy_content(handle->metalink_fd, fd);
            close(fd);
            if (rc != 0) {
                g_debug("%s: Cannot copy content of metalink file", __func__);
                g_set_error(err, LR_YUM_ERROR, LRE_IO,
                        "Cannot copy content of metalink file %s: %s",
                        ml_file_path, g_strerror(errno));
                lr_free(ml_file_path);
                return FALSE;
            }
            repo->metalink = ml_file_path;
        }

        /* Prepare repomd.xml file */
        path = lr_pathconcat(handle->destdir, "/repodata/repomd.xml", NULL);
        fd = open(path, O_CREAT|O_TRUNC|O_RDWR, 0666);
        if (fd == -1) {
            g_set_error(err, LR_YUM_ERROR, LRE_IO,
                        "Cannot open %s: %s", path, g_strerror(errno));
            lr_free(path);
            return FALSE;
        }

        /* Download repomd.xml */
        ret = lr_yum_download_repomd(handle, handle->metalink, fd, err);
        if (!ret) {
            close(fd);
            lr_free(path);
            return FALSE;
        }

        /* Check repomd.xml.asc if available.
         * Try to download and verify GPG signature (repomd.xml.asc).
         * Try to download only from the mirror where repomd.xml iself was
         * downloaded. It is because most of yum repositories are not signed
         * and try every mirror for signature is non effective.
         * Every mirror would be tried because mirrorded_download function have
         * no clue if 404 for repomd.xml.asc means that no signature exists or
         * it is just error on the mirror and should try the next one.
         **/
        if (handle->checks & LR_CHECK_GPG) {
            int fd_sig;
            char *url, *signature;

            signature = lr_pathconcat(handle->destdir, "repodata/repomd.xml.asc", NULL);
            fd_sig = open(signature, O_CREAT|O_TRUNC|O_RDWR, 0666);
            if (fd_sig == -1) {
                g_debug("%s: Cannot open: %s", __func__, signature);
                g_set_error(err, LR_YUM_ERROR, LRE_IO,
                            "Cannot open %s: %s", signature, g_strerror(errno));
                close(fd);
                lr_free(path);
                lr_free(signature);
                return FALSE;
            }

            url = lr_pathconcat(handle->used_mirror, "repodata/repomd.xml.asc", NULL);
            ret = lr_download_url(handle, url, fd_sig, &tmp_err);
            lr_free(url);
            close(fd_sig);
            if (!ret) {
                // Signature doesn't exist
                g_debug("%s: GPG signature doesn't exists: %s",
                        __func__, tmp_err->message);
                g_set_error(err, LR_YUM_ERROR, LRE_BADGPG,
                            "GPG verification is enabled, but GPG signature "
                            "repomd.xml.asc is not available: %s", tmp_err->message);
                g_clear_error(&tmp_err);
                unlink(signature);
                lr_free(signature);
                return FALSE;
            } else {
                // Signature downloaded
                repo->signature = g_strdup(signature);
                ret = lr_gpg_check_signature(signature,
                                             path,
                                             handle->gnupghomedir,
                                             &tmp_err);
                lr_free(signature);
                if (!ret) {
                    g_debug("%s: GPG signature verification failed: %s",
                            __func__, tmp_err->message);
                    g_propagate_prefixed_error(err, tmp_err,
                            "repomd.xml GPG signature verification error: ");
                    close(fd);
                    lr_free(path);
                    return FALSE;
                }
                g_debug("%s: GPG signature successfully verified", __func__);
            }
        }

        lseek(fd, 0, SEEK_SET);

        /* Parse repomd */
        g_debug("%s: Parsing repomd.xml", __func__);
        ret = lr_yum_repomd_parse_file(repomd, fd, lr_xml_parser_warning_logger,
                                       "Repomd xml parser", &tmp_err);
        close(fd);
        if (!ret) {
            g_debug("%s: Parsing unsuccessful: %s", __func__, tmp_err->message);
            g_propagate_prefixed_error(err, tmp_err,
                                       "repomd.xml parser error: ");
            lr_free(path);
            return FALSE;
        }

        /* Fill result object */
        result->destdir = g_strdup(handle->destdir);
        repo->destdir = g_strdup(handle->destdir);
        repo->repomd = path;
        if (handle->used_mirror)
            repo->url = g_strdup(handle->used_mirror);
        else
            repo->url = g_strdup(handle->urls[0]);

        g_debug("%s: Repomd revision: %s", repomd->revision, __func__);
    }

    /* Download rest of metadata files */
    ret = lr_yum_download_repo(handle, repo, repomd, &tmp_err);
    assert((ret && !tmp_err) || (!ret && tmp_err));

    if (!ret) {
        g_debug("%s: Repository download error: %s", __func__, tmp_err->message);
        g_propagate_prefixed_error(err, tmp_err, "Yum repo downloading error: ");
        return FALSE;
    }

    return TRUE;
}

gboolean
lr_yum_perform(LrHandle *handle, LrResult *result, GError **err)
{
    int ret = TRUE;
    LrYumRepo *repo;
    LrYumRepoMd *repomd;

    assert(handle);
    assert(!err || *err == NULL);

    if (!result) {
        g_set_error(err, LR_YUM_ERROR, LRE_BADFUNCARG,
                    "Missing result parameter");
        return FALSE;
    }

    if (!handle->urls && !handle->mirrorlisturl && !handle->metalinkurl) {
        g_set_error(err, LR_YUM_ERROR, LRE_NOURL,
                "No LRO_URLS, LRO_MIRRORLISTURL nor LRO_METALINKURL specified");
        return FALSE;
    }

    if (handle->local && (!handle->urls || !handle->urls[0])) {
        g_set_error(err, LR_YUM_ERROR, LRE_NOURL,
                    "Localrepo specified, but no LRO_URLS set");
        return FALSE;
    }

    if (handle->update) {
        // Download/Locate only specified files
        if (!result->yum_repo || !result->yum_repomd) {
            g_set_error(err, LR_YUM_ERROR, LRE_INCOMPLETERESULT,
                    "Incomplete result object - "
                    "Cannot update on this result object");
            return FALSE;
        }
    } else {
        // Download/Locate from scratch
        if (result->yum_repo || result->yum_repomd) {
            g_set_error(err, LR_YUM_ERROR, LRE_ALREADYUSEDRESULT,
                        "This result object is not clear - "
                        "Already used result object");
            return FALSE;
        }
        result->yum_repo = lr_yum_repo_init();
        result->yum_repomd = lr_yum_repomd_init();
    }

    repo   = result->yum_repo;
    repomd = result->yum_repomd;

    if (handle->local) {
        // Do not duplicate repository, just use the existing local one

        ret = lr_yum_use_local(handle, result, err);
        if (!ret)
            return FALSE;

        if (handle->checks & LR_CHECK_CHECKSUM)
            ret = lr_yum_check_repo_checksums(repo, repomd, err);
    } else {
        // Download remote/Duplicate local repository
        // Note: All checksums are checked while downloading

        ret = lr_yum_download_remote(handle, result, err);
    }

    return ret;
}
