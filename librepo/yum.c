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
#define _DEFAULT_SOURCE
#define  BITS_IN_BYTE 8

#include <stdio.h>
#include <libgen.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef WITH_ZCHUNK
#include <zck.h>
#endif /* WITH_ZCHUNK */

#include "util.h"
#include "metalink.h"
#include "repomd.h"
#include "downloader.h"
#include "handle_internal.h"
#include "result_internal.h"
#include "yum_internal.h"
#include "downloader_internal.h"
#include "gpg.h"
#include "cleanup.h"
#include "librepo.h"

/* helper functions for YumRepo manipulation */

LrYumRepo *
lr_yum_repo_init(void)
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

static char *
get_type(LrYumRepo *repo, const char *type)
{
    if (!repo->use_zchunk)
        return g_strdup(type);

    gchar *chk_type = g_strconcat(type, "_zck", NULL);

    for (GSList *elem = repo->paths; elem; elem = g_slist_next(elem)) {
        LrYumRepoPath *yumrepopath = elem->data;
        assert(yumrepopath);
        if (!strcmp(yumrepopath->type, chk_type))
            return chk_type;
    }
    g_free(chk_type);
    return g_strdup(type);
}

static const char *
yum_repo_path(LrYumRepo *repo, const char *type)
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

const char *
lr_yum_repo_path(LrYumRepo *repo, const char *type)
{
    assert(repo);

    gchar *chk_type = get_type(repo, type);
    const char *path = yum_repo_path(repo, chk_type);
    g_free(chk_type);
    return path;
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

/* main business logic */

gint
compare_records(gconstpointer a, gconstpointer b)
{
    LrYumRepoMdRecord* yum_record = (LrYumRepoMdRecord*) a;
    char *type1 = (char *) yum_record->type;
    char *type2 = (char *) b;
    return g_strcmp0(type1, type2);
}

static void
lr_yum_switch_to_zchunk(LrHandle *handle, LrYumRepoMd *repomd)
{
    if (handle->yumdlist) {
        int x = 0;
        while (handle->yumdlist[x]) {
            char *check_type = g_strconcat(handle->yumdlist[x], "_zck", NULL);
            assert(check_type);

            /* Check whether we already want the zchunk version of this record */
            int found = FALSE;
            int y = 0;
            while (handle->yumdlist[y]) {
                if (y == x) {
                    y++;
                    continue;
                }
                if (strcmp(handle->yumdlist[y], check_type) == 0) {
                    found = TRUE;
                    break;
                }
                y++;
            }
            if (found) {
                g_free(check_type);
                x++;
                continue;
            }

            found = FALSE;
            /* Check whether the zchunk version of this record exists */
            for (GSList *elem = repomd->records; elem; elem = g_slist_next(elem)) {
                LrYumRepoMdRecord *record = elem->data;

                if (strcmp(record->type, check_type) == 0) {
                    g_debug("Found %s so using instead of %s", check_type,
                            handle->yumdlist[x]);
                    g_free(handle->yumdlist[x]);
                    handle->yumdlist[x] = check_type;
                    found = TRUE;
                    break;
                }
            }
            if (!found)
                g_free(check_type);
            x++;
        }
    }
    return;
}

static gboolean
lr_yum_repomd_record_enabled(LrHandle *handle, const char *type, GSList* records)
{
    // Check for records that shouldn't be downloaded
    if (handle->yumblist) {
        int x = 0;
        while (handle->yumblist[x]) {
            if (!strcmp(handle->yumblist[x], type))
                return FALSE;
            x++;
        }
    }

    // Check for records that should be downloaded
    if (handle->yumdlist) {
        int x = 0;
        while (handle->yumdlist[x]) {
            if (!strcmp(handle->yumdlist[x], type))
                return TRUE;
            x++;
        }
        // Substitution check
        if (handle->yumslist) {
            for (GSList *elem = handle->yumslist; elem; elem = g_slist_next(elem)) {
                LrVar* subs = elem->data;
                if (!g_strcmp0(subs->val, type)) {
                    char *orig = subs->var;
                    for (guint i = 0; handle->yumdlist[i]; i++) {
                        if (!g_strcmp0(orig, handle->yumdlist[i]) &&
                            !g_slist_find_custom(records, orig, (GCompareFunc) compare_records))
                            return TRUE;
                    }
                    return FALSE;
                }
            }
        }
        return FALSE;
    }
    return TRUE;
}

static CbData *cbdata_new(void *userdata,
                          void *cbdata,
                          LrProgressCb progresscb,
                          LrHandleMirrorFailureCb hmfcb,
                          const char *metadata)
{
    CbData *data = calloc(1, sizeof(*data));
    data->userdata = userdata;
    data->cbdata = cbdata;
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

int
hmfcb(void *clientp, const char *msg, const char *url)
{
    CbData *data = clientp;
    if (data->hmfcb)
        return data->hmfcb(data->userdata, msg, url, data->metadata);
    return LR_CB_OK;
}

gboolean
lr_prepare_repodata_dir(LrHandle *handle,
                        GError **err)
{
    int rc;
    int create_repodata_dir = 1;
    char *path_to_repodata;

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
    g_free(path_to_repodata);

    return TRUE;
}

gboolean
lr_store_mirrorlist_files(LrHandle *handle,
                          LrYumRepo *repo,
                          GError **err)
{
    int fd;
    int rc;

    if (handle->mirrorlist_fd != -1) {
        char *ml_file_path = lr_pathconcat(handle->destdir,
                                           "mirrorlist", NULL);
        fd = open(ml_file_path, O_CREAT|O_TRUNC|O_RDWR, 0666);
        if (fd < 0) {
            g_debug("%s: Cannot create: %s", __func__, ml_file_path);
            g_set_error(err, LR_YUM_ERROR, LRE_IO,
                        "Cannot create %s: %s", ml_file_path, g_strerror(errno));
            g_free(ml_file_path);
            return FALSE;
        }
        rc = lr_copy_content(handle->mirrorlist_fd, fd);
        close(fd);
        if (rc != 0) {
            g_debug("%s: Cannot copy content of mirrorlist file", __func__);
            g_set_error(err, LR_YUM_ERROR, LRE_IO,
                        "Cannot copy content of mirrorlist file %s: %s",
                        ml_file_path, g_strerror(errno));
            g_free(ml_file_path);
            return FALSE;
        }
        repo->mirrorlist = ml_file_path;
    }

    return TRUE;
}

gboolean
lr_copy_metalink_content(LrHandle *handle,
                         LrYumRepo *repo,
                         GError **err)
{
    int fd;
    int rc;

    if (handle->metalink_fd != -1) {
        char *ml_file_path = lr_pathconcat(handle->destdir,
                                           "metalink.xml", NULL);
        fd = open(ml_file_path, O_CREAT|O_TRUNC|O_RDWR, 0666);
        if (fd < 0) {
            g_debug("%s: Cannot create: %s", __func__, ml_file_path);
            g_set_error(err, LR_YUM_ERROR, LRE_IO,
                        "Cannot create %s: %s", ml_file_path, g_strerror(errno));
            g_free(ml_file_path);
            return FALSE;
        }
        rc = lr_copy_content(handle->metalink_fd, fd);
        close(fd);
        if (rc != 0) {
            g_debug("%s: Cannot copy content of metalink file", __func__);
            g_set_error(err, LR_YUM_ERROR, LRE_IO,
                        "Cannot copy content of metalink file %s: %s",
                        ml_file_path, g_strerror(errno));
            g_free(ml_file_path);
            return FALSE;
        }
        repo->metalink = ml_file_path;
    }

    return TRUE;
}

int
lr_prepare_repomd_xml_file(LrHandle *handle,
                           char **path,
                           GError **err)
{
    int fd;

    *path = lr_pathconcat(handle->destdir, "/repodata/repomd.xml", NULL);
    fd = open(*path, O_CREAT|O_TRUNC|O_RDWR, 0666);
    if (fd == -1) {
        g_set_error(err, LR_YUM_ERROR, LRE_IO,
                    "Cannot open %s: %s", *path, g_strerror(errno));
        g_free(*path);
        return -1;
    }

    return fd;
}

/** Check repomd.xml.asc if available.
 * Try to download and verify GPG signature (repomd.xml.asc).
 * Try to download only from the mirror where repomd.xml itself was
 * downloaded. It is because most of yum repositories are not signed
 * and try every mirror for signature is non effective.
 * Every mirror would be tried because mirrored_download function have
 * no clue if 404 for repomd.xml.asc means that no signature exists or
 * it is just error on the mirror and should try the next one.
 **/
gboolean
lr_check_repomd_xml_asc_availability(LrHandle *handle,
                                     LrYumRepo *repo,
                                     int fd,
                                     char *path,
                                     GError **err)
{
    GError *tmp_err = NULL;
    gboolean ret;

    if (handle->checks & LR_CHECK_GPG) {
        int fd_sig;
        char *url, *signature;

        signature = lr_pathconcat(handle->destdir, "repodata/repomd.xml.asc", NULL);
        fd_sig = open(signature, O_CREAT | O_TRUNC | O_RDWR, 0666);
        if (fd_sig == -1) {
            g_debug("%s: Cannot open: %s", __func__, signature);
            g_set_error(err, LR_YUM_ERROR, LRE_IO,
                        "Cannot open %s: %s", signature, g_strerror(errno));
            g_free(signature);
            return FALSE;
        }

        url = lr_pathconcat(handle->used_mirror, "repodata/repomd.xml.asc", NULL);
        ret = lr_download_url(handle, url, fd_sig, &tmp_err);
        g_free(url);
        close(fd_sig);
        if (!ret) {
            // Error downloading signature
            g_set_error(err, LR_YUM_ERROR, LRE_BADGPG,
                        "GPG verification is enabled, but GPG signature "
                        "is not available. This may be an error or the "
                        "repository does not support GPG verification: %s", tmp_err->message);
            g_clear_error(&tmp_err);
            unlink(signature);
            g_free(signature);
            return FALSE;
        } else {
            // Signature downloaded
            repo->signature = g_strdup(signature);
            ret = lr_gpg_check_signature(signature,
                                         path,
                                         handle->gnupghomedir,
                                         &tmp_err);
            g_free(signature);
            if (!ret) {
                g_debug("%s: GPG signature verification failed: %s",
                        __func__, tmp_err->message);
                g_propagate_prefixed_error(err, tmp_err,
                                           "repomd.xml GPG signature verification error: ");
                return FALSE;
            }
            g_debug("%s: GPG signature successfully verified", __func__);
        }
    }

    return TRUE;
}

void
lr_get_best_checksum(const LrMetalink *metalink,
                     GSList **checksums)
{
    gboolean ret;
    LrChecksumType ch_type;
    gchar *ch_value;

    // From the metalink itself
    ret = lr_best_checksum(metalink->hashes, &ch_type, &ch_value);
    if (ret)
    {
        LrDownloadTargetChecksum *dtch;
        dtch = lr_downloadtargetchecksum_new(ch_type, ch_value);
        *checksums = g_slist_prepend(*checksums, dtch);
        g_debug("%s: Expected checksum for repomd.xml: (%s) %s",
                __func__, lr_checksum_type_to_str(ch_type), ch_value);
    }

    // From the alternates entries
    for (GSList *elem = metalink->alternates; elem; elem = g_slist_next(elem))
    {
        LrMetalinkAlternate *alt = elem->data;
        ret = lr_best_checksum(alt->hashes, &ch_type, &ch_value);
        if (ret) {
            LrDownloadTargetChecksum *dtch;
            dtch = lr_downloadtargetchecksum_new(ch_type, ch_value);
            *checksums = g_slist_prepend(*checksums, dtch);
            g_debug("%s: Expected alternate checksum for repomd.xml: (%s) %s",
                    __func__, lr_checksum_type_to_str(ch_type), ch_value);
        }
    }
}

CbData *
lr_get_metadata_failure_callback(const LrHandle *handle)
{
    CbData *cbdata = NULL;
    if (handle->hmfcb) {
        cbdata = cbdata_new(handle->user_data,
                            NULL,
                            NULL,
                            handle->hmfcb,
                            "repomd.xml");
    }
    return cbdata;
}

gboolean
lr_yum_download_url(LrHandle *lr_handle, const char *url, int fd,
                    gboolean no_cache, gboolean is_zchunk, GError **err)
{
    gboolean ret;
    LrDownloadTarget *target;
    GError *tmp_err = NULL;
    CbData *cbdata = NULL;

    assert(url);
    assert(!err || *err == NULL);

    if (lr_handle != NULL)
        cbdata = cbdata_new(lr_handle->user_data,
                            NULL,
                            lr_handle->user_cb,
                            lr_handle->hmfcb,
                            url);

    // Prepare target
    target = lr_downloadtarget_new(lr_handle,
                                   url, NULL, fd, NULL,
                                   NULL, 0, 0,(lr_handle && lr_handle->user_cb) ? progresscb : NULL, cbdata,
                                   NULL, (lr_handle && lr_handle->hmfcb) ? hmfcb : NULL, NULL, 0, 0,
                                   NULL, no_cache, is_zchunk);

    // Download the target
    ret = lr_download_target(target, &tmp_err);

    assert(ret || tmp_err);
    assert(!(target->err) || !ret);
    if (cbdata)
        cbdata_free(cbdata);

    if (!ret)
        g_propagate_error(err, tmp_err);

    lr_downloadtarget_free(target);

    lseek(fd, 0, SEEK_SET);

    return ret;
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
        lr_get_best_checksum(metalink, &checksums);
    }

    CbData *cbdata = cbdata_new(handle->user_data,
                            NULL,
                            handle->user_cb,
                            handle->hmfcb,
                            "repomd.xml");

    LrDownloadTarget *target = lr_downloadtarget_new(handle,
                                                     "repodata/repomd.xml",
                                                     NULL,
                                                     fd,
                                                     NULL,
                                                     checksums,
                                                     0,
                                                     0,
                                                     (handle->user_cb) ? progresscb : NULL,
                                                     cbdata,
                                                     NULL,
                                                     (handle->hmfcb) ? hmfcb : NULL,
                                                     NULL,
                                                     0,
                                                     0,
                                                     NULL,
                                                     TRUE,
                                                     FALSE);

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

gboolean
prepare_repo_download_std_target(LrHandle *handle,
                                 LrYumRepoMdRecord *record,
                                 char **path,
                                 int *fd,
                                 GSList **checksums,
                                 GSList **targets,
                                 GError **err)
{
    *path = lr_pathconcat(handle->destdir, record->location_href, NULL);
    *fd = open(*path, O_CREAT|O_TRUNC|O_RDWR, 0666);
    if (*fd < 0) {
        g_debug("%s: Cannot create/open %s (%s)",
                __func__, *path, g_strerror(errno));
        g_set_error(err, LR_YUM_ERROR, LRE_IO,
                    "Cannot create/open %s: %s", *path, g_strerror(errno));
        g_free(*path);
        g_slist_free_full(*targets, (GDestroyNotify) lr_downloadtarget_free);
        return FALSE;
    }

    if (handle->checks & LR_CHECK_CHECKSUM) {
        // Select proper checksum type only if checksum check is enabled
        LrDownloadTargetChecksum *checksum;
        checksum = lr_downloadtargetchecksum_new(
                       lr_checksum_type(record->checksum_type),
                       record->checksum);
        *checksums = g_slist_prepend(*checksums, checksum);
    }
    return TRUE;
}

#ifdef WITH_ZCHUNK
gboolean
prepare_repo_download_zck_target(LrHandle *handle,
                                 LrYumRepoMdRecord *record,
                                 char **path,
                                 int *fd,
                                 GSList **checksums,
                                 GSList **targets,
                                 GError **err)
{
    *path = lr_pathconcat(handle->destdir, record->location_href, NULL);
    *fd = open(*path, O_CREAT|O_RDWR, 0666);
    if (*fd < 0) {
        g_debug("%s: Cannot create/open %s (%s)",
                __func__, *path, g_strerror(errno));
        g_set_error(err, LR_YUM_ERROR, LRE_IO,
                    "Cannot create/open %s: %s", *path, g_strerror(errno));
        g_free(*path);
        g_slist_free_full(*targets, (GDestroyNotify) lr_downloadtarget_free);
        return FALSE;
    }

    if (handle->checks & LR_CHECK_CHECKSUM) {
        // Select proper checksum type only if checksum check is enabled
        LrDownloadTargetChecksum *checksum;
        checksum = lr_downloadtargetchecksum_new(
                       lr_checksum_type(record->header_checksum_type),
                       record->header_checksum);
        *checksums = g_slist_prepend(*checksums, checksum);
    }
    return TRUE;
}
#endif /* WITH_ZCHUNK */

gboolean
prepare_repo_download_targets(LrHandle *handle,
                              LrYumRepo *repo,
                              LrYumRepoMd *repomd,
                              LrMetadataTarget *mdtarget,
                              GSList **targets,
                              GSList **cbdata_list,
                              GError **err)
{
    char *destdir;  /* Destination dir */

    destdir = handle->destdir;
    assert(destdir);
    assert(strlen(destdir));
    assert(!err || *err == NULL);

    if(handle->cachedir) {
        lr_yum_switch_to_zchunk(handle, repomd);
        repo->use_zchunk = TRUE;
    } else {
        g_debug("%s: Cache directory not set, disabling zchunk", __func__);
        repo->use_zchunk = FALSE;
    }

    for (GSList *elem = repomd->records; elem; elem = g_slist_next(elem)) {
        int fd;
        char *path;
        LrDownloadTarget *target;
        LrYumRepoMdRecord *record = elem->data;
        CbData *cbdata = NULL;
        void *user_cbdata = NULL;
        LrEndCb endcb = NULL;

        if (mdtarget != NULL) {
            user_cbdata = mdtarget->cbdata;
            endcb = mdtarget->endcb;
        }

        assert(record);

        if (!lr_yum_repomd_record_enabled(handle, record->type, repomd->records))
            continue;

        char *location_href = record->location_href;

        char *dest_dir = realpath(handle->destdir, NULL);
        path = lr_pathconcat(handle->destdir, record->location_href, NULL);
        char *requested_dir = realpath(dirname(path), NULL);
        g_free(path);
        if (!g_str_has_prefix(requested_dir, dest_dir)) {
            g_debug("%s: Invalid path: %s", __func__, location_href);
            g_set_error(err, LR_YUM_ERROR, LRE_IO, "Invalid path: %s", location_href);
            g_slist_free_full(*targets, (GDestroyNotify) lr_downloadtarget_free);
            free(requested_dir);
            free(dest_dir);
            return FALSE;
        }
        free(requested_dir);
        free(dest_dir);

        gboolean is_zchunk = FALSE;
        #ifdef WITH_ZCHUNK
        if (handle->cachedir && record->header_checksum)
            is_zchunk = TRUE;
        #endif /* WITH_ZCHUNK */

        GSList *checksums = NULL;
        if (is_zchunk) {
            #ifdef WITH_ZCHUNK
            if(!prepare_repo_download_zck_target(handle, record, &path, &fd,
                                                 &checksums, targets, err))
                return FALSE;
            #endif /* WITH_ZCHUNK */
        } else {
            if(!prepare_repo_download_std_target(handle, record, &path, &fd,
                                                 &checksums, targets, err))
                return FALSE;
        }

        if (handle->user_cb || handle->hmfcb) {
            cbdata = cbdata_new(handle->user_data,
                                user_cbdata,
                                handle->user_cb,
                                handle->hmfcb,
                                record->type);
            *cbdata_list = g_slist_append(*cbdata_list, cbdata);
        }

        target = lr_downloadtarget_new(handle,
                                       location_href,
                                       record->location_base,
                                       fd,
                                       NULL,
                                       checksums,
                                       0,
                                       0,
                                       NULL,
                                       cbdata,
                                       endcb,
                                       NULL,
                                       NULL,
                                       0,
                                       0,
                                       NULL,
                                       FALSE,
                                       is_zchunk);

        if(is_zchunk) {
            #ifdef WITH_ZCHUNK
            target->expectedsize = record->size_header;
            target->zck_header_size = record->size_header;
            #endif /* WITH_ZCHUNK */
        }

        if (mdtarget != NULL)
            mdtarget->repomd_records_to_download++;
        *targets = g_slist_append(*targets, target);

        /* Because path may already exists in repo (while update) */
        lr_yum_repo_update(repo, record->type, path);
        g_free(path);
    }

    return TRUE;
}

gboolean
error_handling(GSList *targets, GError **dest_error, GError *src_error)
{
    if (src_error) {
        g_propagate_prefixed_error(dest_error, src_error,
                                   "Downloading error: ");
        return FALSE;
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
                    char *tmp = error_summary;
                    error_summary = g_strconcat(error_summary,
                                                "; ",
                                                target->path,
                                                " - ",
                                                target->err,
                                                NULL);
                    g_free(tmp);
                }
            }

            close(target->fd);
        }

        if (code != LRE_OK) {
            // At least one target failed
            g_set_error(dest_error, LR_DOWNLOADER_ERROR, code,
                        "Downloading error(s): %s", error_summary);
            g_free(error_summary);
            return FALSE;
        }
    }

    return TRUE;
}

gchar* join_glist_strings(GList *list, const gchar *separator) {
    GString *result = g_string_new(NULL);

    for (GList *l = list; l != NULL; l = l->next) {
        gchar *str = (gchar *)l->data;
        g_string_append(result, str);
        if (l->next != NULL) {
            g_string_append(result, separator);
        }
    }

    return g_string_free(result, FALSE); // FALSE = return the string, not free it
}

gboolean
lr_yum_download_repos(GSList *targets,
                      GError **err)
{
    gboolean ret;
    GSList *download_targets = NULL;
    GSList *cbdata_list = NULL;
    GSList *shared_cbdata_list = NULL;
    GError *download_error = NULL;

    for (GSList *elem = targets; elem; elem = g_slist_next(elem)) {
        LrMetadataTarget *repo_target = elem->data;
        GSList *repo_download_targets = NULL;

        if (!repo_target->handle) {
            continue;
        }

        prepare_repo_download_targets(repo_target->handle,
                                      repo_target->repo,
                                      repo_target->repomd,
                                      repo_target,
                                      &repo_download_targets,
                                      &cbdata_list,
                                      &download_error);


        // Shared data for all targets from a single repository
        LrSharedCallbackData *shared_cbdata = lr_malloc0(sizeof(*shared_cbdata));
        shared_cbdata->cb           = repo_target->progresscb;
        shared_cbdata->mfcb         = repo_target->mirrorfailurecb;
        shared_cbdata->endcb        = repo_target->endcb;
        shared_cbdata->singlecbdata = NULL;
        shared_cbdata->target       = repo_target;
        shared_cbdata_list = g_slist_append(shared_cbdata_list, shared_cbdata);

        for (GSList *elem = repo_download_targets; elem; elem = g_slist_next(elem)) {
            LrDownloadTarget *download_target = elem->data;
            LrCallbackData *lrcbdata = lr_malloc0(sizeof(*lrcbdata));
            lrcbdata->downloaded     = 0.0;
            lrcbdata->total          = 0.0;
            lrcbdata->userdata       = repo_target->cbdata;
            lrcbdata->sharedcbdata   = shared_cbdata;

            download_target->progresscb      = (repo_target->progresscb) ? lr_multi_progress_func : NULL;
            download_target->mirrorfailurecb = (repo_target->mirrorfailurecb) ? lr_multi_mf_func : NULL;
            download_target->endcb           = (repo_target->endcb) ? lr_metadata_target_end_func : NULL;
            download_target->cbdata          = lrcbdata;

            shared_cbdata->singlecbdata = g_slist_append(shared_cbdata->singlecbdata,
                                                        lrcbdata);
        }

        if ((g_slist_length(repo_download_targets) == 0) && (repo_target->endcb)) {
            LrTransferStatus status;
            const char *msg;
            if (g_list_length(repo_target->err) == 0) {
                // If there is nothing to download for this repo_target and
                // there were no erors it is finished.
                // This can happen when downloading just metalink/repomd.xml
                status = LR_TRANSFER_SUCCESSFUL;
                msg = "Successfully downloaded";
            } else {
                // If there were errors (we failed to download/verify/parse repomd)
                // for this repo_target it cannot continue and is finished.
                status = LR_TRANSFER_ERROR;
                const char * err_msg = join_glist_strings(repo_target->err, ",");
                msg = err_msg ? err_msg : "Unknown error.";
            }
            int ret = repo_target->endcb(repo_target->cbdata, status, msg);
            if (ret == LR_CB_ERROR) {
                g_debug("%s: Downloading was aborted by LR_CB_ERROR from end callback", __func__);
                g_set_error(err, LR_DOWNLOADER_ERROR,
                            LRE_CBINTERRUPTED,
                            "Interrupted by LR_CB_ERROR from end callback");
                return FALSE;
            }
        } else {
            download_targets = g_slist_concat(download_targets, repo_download_targets);
        }
    }

    if (!download_targets) {
        if (download_error) {
            g_propagate_error(err, download_error);
        }
        return TRUE;
    }

    ret = lr_download(download_targets,
                      FALSE,
                      &download_error);

    if (!ret && download_error) {
        g_propagate_error(err, download_error);
    }

    // Propagate download target error to its metadata target
    for (GSList *elem = download_targets; elem; elem = g_slist_next(elem)) {
        LrDownloadTarget *target = elem->data;
        if (target->err) {
            LrCallbackData *lrcbdata = target->cbdata;
            LrSharedCallbackData *shared_cbdata = lrcbdata->sharedcbdata;
            LrMetadataTarget *metadata_target = shared_cbdata->target;
            metadata_target->err = g_list_append(metadata_target->err, g_strdup(target->err));
        }
    }

    for (GSList *elem = shared_cbdata_list; elem; elem = g_slist_next(elem)) {
        LrSharedCallbackData *shared_cbdata = elem->data;
        g_slist_free_full(shared_cbdata->singlecbdata, (GDestroyNotify)lr_free);
    }
    g_slist_free_full(shared_cbdata_list, (GDestroyNotify)lr_free);
    g_slist_free_full(cbdata_list, (GDestroyNotify)cbdata_free);
    g_slist_free_full(download_targets, (GDestroyNotify)lr_downloadtarget_free);

    return ret;
}

gboolean
lr_yum_download_repo(LrHandle *handle,
                     LrYumRepo *repo,
                     LrYumRepoMd *repomd,
                     GError **err)
{
    gboolean ret = TRUE;
    GSList *targets = NULL;
    GSList *cbdata_list = NULL;
    GError *tmp_err = NULL;

    assert(!err || *err == NULL);

    prepare_repo_download_targets(handle, repo, repomd, NULL, &targets, &cbdata_list, err);

    if (!targets)
        return TRUE;

    ret = lr_download_single_cb(targets,
                                FALSE,
                                (cbdata_list) ? progresscb : NULL,
                                (cbdata_list) ? hmfcb : NULL,
                                &tmp_err);

    assert((ret && !tmp_err) || (!ret && tmp_err));
    ret = error_handling(targets, err, tmp_err);

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
    gboolean is_zchunk = FALSE;
    GError *tmp_err = NULL;

    assert(!err || *err == NULL);

    if (!rec || !path)
        return TRUE;

    #ifdef WITH_ZCHUNK
    if(rec->header_checksum) {
        expected_checksum = rec->header_checksum;
        checksum_type = lr_checksum_type(rec->header_checksum_type);
        is_zchunk = TRUE;
    } else {
    #endif /* WITH_ZCHUNK */
        expected_checksum = rec->checksum;
        checksum_type = lr_checksum_type(rec->checksum_type);
    #ifdef WITH_ZCHUNK
    }
    #endif /* WITH_ZCHUNK */

    g_debug("%s: Checking checksum of %s (expected: %s [%s])",
                       __func__, path, expected_checksum, rec->checksum_type);

    if (!expected_checksum) {
        // Empty checksum - suppose it's ok
        g_debug("%s: No checksum in repomd", __func__);
        return TRUE;
    }

    if (checksum_type == LR_CHECKSUM_UNKNOWN) {
        g_debug("%s: Unknown checksum", __func__);
        g_set_error(err, LR_YUM_ERROR, LRE_UNKNOWNCHECKSUM,
                    "Unknown checksum type for %s", path);
        return FALSE;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        g_debug("%s: Cannot open %s", __func__, path);
        g_set_error(err, LR_YUM_ERROR, LRE_IO,
                    "Cannot open %s: %s", path, g_strerror(errno));
        return FALSE;
    }

    if (is_zchunk) {
        #ifdef WITH_ZCHUNK
        ret = FALSE;
        matches = FALSE;
        zckCtx *zck = lr_zck_init_read_base(expected_checksum, checksum_type,
                                            rec->size_header, fd, &tmp_err);
        if (!tmp_err) {
            if(zck_validate_data_checksum(zck) < 1) {
                g_set_error(&tmp_err, LR_YUM_ERROR, LRE_ZCK,
                            "Unable to validate zchunk checksums");
            } else {
                ret = TRUE;
                matches = TRUE;
            }
        }
        if (zck)
            zck_free(&zck);
        #endif /* WITH_ZCHUNK */
    } else {
        ret = lr_checksum_fd_cmp(checksum_type,
                                 fd,
                                 expected_checksum,
                                 1,
                                 &matches,
                                 &tmp_err);
    }

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

        const char *path = yum_repo_path(repo, record->type);

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
    _cleanup_fd_close_ int fd = -1;

    if (handle->mirrorlist_fd != -1) {
        // Locate mirrorlist if available.
        gchar *mrl_fn = lr_pathconcat(baseurl, "mirrorlist", NULL);
        if (g_file_test(mrl_fn, G_FILE_TEST_IS_REGULAR)) {
            g_debug("%s: Found local mirrorlist: %s", __func__, mrl_fn);
            repo->mirrorlist = mrl_fn;
        } else {
            repo->mirrorlist = NULL;
            g_free(mrl_fn);
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
            g_free(mtl_fn);
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
            g_set_error(err, LR_YUM_ERROR, LRE_BADGPG,
                        "GPG verification is enabled, but GPG signature "
                        "repomd.xml.asc is not available. This may be an "
                        "error or the repository does not support GPG verification.");
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

    if(handle->cachedir) {
        lr_yum_switch_to_zchunk(handle, repomd);
        repo->use_zchunk = TRUE;
    } else {
        g_debug("%s: Cache directory not set, disabling zchunk", __func__);
        repo->use_zchunk = FALSE;
    }

    // Locate rest of metadata files
    for (GSList *elem = repomd->records; elem; elem = g_slist_next(elem)) {
        _cleanup_free_ char *path = NULL;
        LrYumRepoMdRecord *record = elem->data;

        assert(record);

        if (!lr_yum_repomd_record_enabled(handle, record->type, repomd->records))
            continue; // Caller isn't interested in this record type
        if (yum_repo_path(repo, record->type))
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
    gboolean ret = TRUE;
    int fd;
    LrYumRepo *repo;
    LrYumRepoMd *repomd;
    GError *tmp_err = NULL;

    assert(!err || *err == NULL);

    repo   = result->yum_repo;
    repomd = result->yum_repomd;

    g_debug("%s: Downloading/Copying repo..", __func__);

    if (!lr_prepare_repodata_dir(handle, err))
        return FALSE;

    if (!handle->update) {
        char *path = NULL;

        if (!lr_store_mirrorlist_files(handle, repo, err))
            return FALSE;

        if (!lr_copy_metalink_content(handle, repo, err))
            return FALSE;

        if ((fd = lr_prepare_repomd_xml_file(handle, &path, err)) == -1)
            return FALSE;

        /* Download repomd.xml */
        ret = lr_yum_download_repomd(handle, handle->metalink, fd, err);
        if (!ret) {
            close(fd);
            lr_free(path);
            return FALSE;
        }

        if (!lr_check_repomd_xml_asc_availability(handle, repo, fd, path, err)) {
            close(fd);
            lr_free(path);
            return FALSE;
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
