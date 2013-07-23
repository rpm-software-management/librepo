/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2012  Tomas Mlcoch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
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

#include "setup.h"
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

/* helper functions for YumRepo manipulation */

lr_YumRepo
lr_yum_repo_init()
{
    return lr_malloc0(sizeof(struct _lr_YumRepo));
}

void
lr_yum_repo_clear(lr_YumRepo repo)
{
    if (!repo)
        return;
    for (int x = 0; x < repo->nop; x++) {
        lr_free(repo->paths[x]->type);
        lr_free(repo->paths[x]->path);
        lr_free(repo->paths[x]);
    }
    lr_free(repo->paths);
    lr_free(repo->repomd);
    lr_free(repo->url);
    lr_free(repo->destdir);
    lr_free(repo->signature);
    lr_free(repo->mirrorlist);
    memset(repo, 0, sizeof(struct _lr_YumRepo));
}

void
lr_yum_repo_free(lr_YumRepo repo)
{
    if (!repo)
        return;
    lr_yum_repo_clear(repo);
    lr_free(repo);
}

const char *
lr_yum_repo_path(lr_YumRepo repo, const char *type)
{
    assert(repo);
    for (int x = 0; x < repo->nop; x++)
        if (!strcmp(repo->paths[x]->type, type))
            return repo->paths[x]->path;
    return NULL;
}

void
lr_yum_repo_append(lr_YumRepo repo, const char *type, const char *path)
{
    assert(repo);
    assert(type);
    assert(path);
    repo->paths = lr_realloc(repo->paths, (repo->nop+1) * sizeof(lr_YumRepoMd));
    repo->paths[repo->nop] = lr_malloc(sizeof(struct _lr_YumRepoPath));
    repo->paths[repo->nop]->type = lr_strdup(type);
    repo->paths[repo->nop]->path = lr_strdup(path);
    repo->nop++;
}

void
lr_yum_repo_update(lr_YumRepo repo, const char *type, const char *path)
{
    assert(repo);
    assert(type);
    assert(path);
    for (int x = 0; x < repo->nop; x++)
        if (!strcmp(repo->paths[x]->type, type)) {
            lr_free(repo->paths[x]->path);
            repo->paths[x]->path = lr_strdup(path);
            return;
        }
    lr_yum_repo_append(repo, type, path);
}

/* main bussines logic */

int
lr_yum_repomd_record_enabled(lr_Handle handle, const char *type)
{
    // Blacklist check
    if (handle->yumblist) {
        int x = 0;
        while (handle->yumblist[x]) {
            if (!strcmp(handle->yumblist[x], type))
                return 0;
            x++;
        }
    }

    // Whitelist check
    if (handle->yumdlist) {
        int x = 0;
        while (handle->yumdlist[x]) {
            if (!strcmp(handle->yumdlist[x], type))
                return 1;
            x++;
        }
        return 0;
    }
    return 1;
}

int
lr_yum_download_repomd(lr_Handle handle,
                       lr_Metalink *metalink,
                       int fd)
{
    int rc = LRE_OK;
    lr_ChecksumType checksum_type = LR_CHECKSUM_UNKNOWN;
    char *checksum = NULL;
    GError *tmp_err = NULL;

    g_debug("%s: Downloading repomd.xml via mirrorlist", __func__);

    if (metalink && (handle->checks & LR_CHECK_CHECKSUM)) {
        /* Select the best checksum type */
        for (GSList *elem = metalink->hashes; elem; elem = g_slist_next(elem)) {
            lr_ChecksumType mtype;
            lr_MetalinkHash *mhash = elem->data;

            if (!mhash->type || !mhash->value)
                continue;

            mtype = lr_checksum_type(mhash->type);
            if (mtype != LR_CHECKSUM_UNKNOWN && mtype > checksum_type) {
                checksum_type = mtype;
                checksum = mhash->value;
            }
        }

        g_debug("%s: selected repomd.xml checksum to check: (%s) %s",
                __func__, lr_checksum_type_to_str(checksum_type), checksum);
    }

    lr_DownloadTarget *target = lr_downloadtarget_new("repodata/repomd.xml",
                                                      NULL,
                                                      fd,
                                                      checksum_type,
                                                      checksum,
                                                      0,
                                                      NULL,
                                                      NULL);

    rc = lr_download_target(handle, target, &tmp_err);

    if (tmp_err) {
        assert(rc == tmp_err->code);  // XXX: DEBUG
        //g_propagate_error(err, tmp_err);
        g_error_free(tmp_err);
    } else if (target->err) {
        rc = target->rcode;
        //g_set_error(err, LR_DOWNLOADER_ERROR, target->rcode, target->err);
    } else {
        // Set mirror used for download a repomd.xml to the handle
        handle->used_mirror = lr_strdup(target->usedmirror);
    }

    lr_downloadtarget_free(target);

    if (rc != LRE_OK) {
        /* Download of repomd.xml was not successful */
        g_debug("%s: repomd.xml download was unsuccessful", __func__);
        return rc;
    }

    return LRE_OK;
}

int
lr_yum_download_repo(lr_Handle handle, lr_YumRepo repo, lr_YumRepoMd *repomd)
{
    int ret = LRE_OK;
    char *destdir;  /* Destination dir */
    GSList *targets = NULL;

    destdir = handle->destdir;
    DEBUGASSERT(destdir);
    DEBUGASSERT(strlen(destdir));

    for (GSList *elem = repomd->records; elem; elem = g_slist_next(elem)) {
        int fd;
        char *path;
        //lr_CurlTarget target;
        lr_DownloadTarget *target;
        lr_YumRepoMdRecord *record = elem->data;
        lr_ChecksumType checksumtype;

        assert(record);

        if (!lr_yum_repomd_record_enabled(handle, record->type))
            continue;

        path = lr_pathconcat(destdir, record->location_href, NULL);
        fd = open(path, O_CREAT|O_TRUNC|O_RDWR, 0666);
        if (fd < 0) {
            g_debug("%s: Cannot create/open %s (%s)",
                    __func__, path, strerror(errno));
            lr_free(path);
            return LRE_IO;
        }

        if (handle->checks & LR_CHECK_CHECKSUM)
            // Select proper checksum type only if checksum check is enabled
            checksumtype = lr_checksum_type(record->checksum_type);
        else
            checksumtype = LR_CHECKSUM_UNKNOWN;

        target = lr_downloadtarget_new(record->location_href,
                                       NULL,
                                       fd,
                                       checksumtype,
                                       record->checksum,
                                       0,
                                       NULL,
                                       NULL);

        targets = g_slist_append(targets, target);

        /* Because path may already exists in repo (while update) */
        lr_yum_repo_update(repo, record->type, path);
        lr_free(path);
    }

    if (targets)
        ret = lr_download_single_cb(handle,
                                    targets,
                                    handle->user_cb,
                                    handle->user_data,
                                    NULL);

    // TODO: Error handling

    for (GSList *elem = targets; elem; elem = g_slist_next(elem)) {
        lr_DownloadTarget *target = elem->data;
        // TODO: Error handling
        if (ret == LRE_OK && target->rcode != LRE_OK)
            ret = target->rcode;
        lr_downloadtarget_free(target);
    }

    return ret;
}

int
lr_yum_check_checksum_of_md_record(lr_YumRepoMdRecord *rec, const char *path)
{
    int ret, fd;
    char *expected_checksum;
    lr_ChecksumType checksum_type;

    if (!rec || !path)
        return LRE_OK;

    expected_checksum = rec->checksum;
    checksum_type = lr_checksum_type(rec->checksum_type);

    g_debug("%s: Checking checksum of %s (expected: %s [%s])",
                       __func__, path, expected_checksum, rec->checksum_type);

    if (!expected_checksum) {
        g_debug("%s: No checksum in repomd", __func__);
        return LRE_OK;  /* Empty checksum - suppose it's ok */
    }

    if (checksum_type == LR_CHECKSUM_UNKNOWN) {
        g_debug("%s: Unknown checksum: %s", __func__, rec->checksum_type);
        return LRE_UNKNOWNCHECKSUM;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        g_debug("%s: Cannot open %s", __func__, path);
        return LRE_IO;
    }

    ret = lr_checksum_fd_cmp(checksum_type, fd, expected_checksum, 1, NULL);

    close(fd);

    if (ret) {
        g_debug("%s: Checksum check - Failed", __func__);
        return LRE_BADCHECKSUM;
    }

    g_debug("%s: Checksum check - Passed", __func__);

    return LRE_OK;
}

int
lr_yum_check_repo_checksums(lr_YumRepo repo, lr_YumRepoMd *repomd)
{
    for (GSList *elem = repomd->records; elem; elem = g_slist_next(elem)) {
        int ret;
        lr_YumRepoMdRecord *record = elem->data;
        assert(record);
        const char *path = lr_yum_repo_path(repo, record->type);
        ret = lr_yum_check_checksum_of_md_record(record, path);
        g_debug("%s: Checksum rc: %d (%s)", __func__, ret, record->type);
        if (ret != LRE_OK)
            return ret;
    }

    return LRE_OK;
}

int
lr_yum_use_local(lr_Handle handle, lr_Result result)
{
    char *path;
    int rc = LRE_OK;
    int fd;
    char *baseurl;
    lr_YumRepo repo;
    lr_YumRepoMd *repomd;

    g_debug("%s: Locating repo..", __func__);

    repo   = result->yum_repo;
    repomd = result->yum_repomd;
    baseurl = handle->baseurl;

    /* Do not duplicate repoata, just locate the local one */
    if (strncmp(baseurl, "file://", 7)) {
        if (strstr(baseurl, "://"))
            return LRE_NOTLOCAL;
    } else {
        /* Skip file:// in baseurl */
        baseurl = baseurl+7;
    }

    if (!handle->update) {
        if (handle->mirrorlist_fd != -1) {
            // Locate mirrorlist if available.
            if (handle->metalink)
                path = lr_pathconcat(baseurl, "metalink.xml", NULL);
            else
                path = lr_pathconcat(baseurl, "mirrorlist", NULL);

            if (access(path, F_OK) == 0) {
                g_debug("%s: Found local mirrorlist: %s", __func__, path);
                repo->mirrorlist = path;
            } else {
                repo->mirrorlist = NULL;
                lr_free(path);
            }

        }

        /* Open and parse repomd */
        char *sig;

        path = lr_pathconcat(baseurl, "repodata/repomd.xml", NULL);
        fd = open(path, O_RDONLY);
        if (fd < 0) {
            g_debug("%s: open(%s): %s", __func__, path, strerror(errno));
            lr_free(path);
            return LRE_IO;
        }

        g_debug("%s: Parsing repomd.xml", __func__);
        rc = lr_yum_repomd_parse_file(repomd, fd, NULL, NULL, NULL);
        if (rc != LRE_OK) {
            g_debug("%s: Parsing unsuccessful (%d)", __func__, rc);
            lr_free(path);
            return rc;
        }

        close(fd);

        /* Fill result object */
        result->destdir = lr_strdup(baseurl);
        repo->destdir = lr_strdup(baseurl);
        repo->repomd = path;

        /* Check if signature file exists */
        sig = lr_pathconcat(baseurl, "repodata/repomd.xml.asc", NULL);
        if (access(sig, F_OK) == 0)
            repo->signature = sig;  // File with key exists
        else
            lr_free(sig);

        /* Signature checking */
        if (handle->checks & LR_CHECK_GPG && repo->signature) {
            rc = lr_gpg_check_signature(repo->signature,
                                        repo->repomd,
                                        NULL,
                                        NULL);
            if (rc != LRE_OK) {
                g_debug("%s: GPG signature verification failed", __func__);
                return rc;
            }
        }


        g_debug("%s: Repomd revision: %s", __func__, repomd->revision);
    }

    /* Locate rest of metadata files */
    for (GSList *elem = repomd->records; elem; elem = g_slist_next(elem)) {
        char *path;
        lr_YumRepoMdRecord *record = elem->data;

        assert(record);

        if (!lr_yum_repomd_record_enabled(handle, record->type))
            continue;
        if (lr_yum_repo_path(repo, record->type))
            continue; /* This path already exists in repo */

        path = lr_pathconcat(baseurl, record->location_href, NULL);
        if (path) {
            if (access(path, F_OK) == -1) {
                /* A repo file is missing */
                if (!handle->ignoremissing) {
                    g_debug("%s: Incomplete repository", __func__);
                    lr_free(path);
                    return LRE_INCOMPLETEREPO;
                }
            } else
                lr_yum_repo_append(repo, record->type, path);
            lr_free(path);
        }
    }

    g_debug("%s: Repository was successfully located", __func__);
    return LRE_OK;
}

int
lr_yum_download_remote(lr_Handle handle, lr_Result result)
{
    int rc = LRE_OK;
    int fd;
    int create_repodata_dir = 1;
    char *path_to_repodata;
    lr_YumRepo repo;
    lr_YumRepoMd *repomd;

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
                    __func__, path_to_repodata, strerror(errno));
            lr_free(path_to_repodata);
            return LRE_CANNOTCREATEDIR;
        }
    }
    lr_free(path_to_repodata);

    if (!handle->update) {
        int ret;
        char *path;

        /* Store mirrorlist file */
        if (handle->mirrorlist_fd != -1) {
            char *ml_file_path;
            if (handle->metalink)
                ml_file_path = lr_pathconcat(handle->destdir, "metalink.xml", NULL);
            else
                ml_file_path = lr_pathconcat(handle->destdir, "mirrorlist", NULL);
            fd = open(ml_file_path, O_CREAT|O_TRUNC|O_RDWR, 0666);
            if (fd < 0) {
                g_debug("%s: Cannot create: %s", __func__, ml_file_path);
                lr_free(ml_file_path);
                return LRE_IO;
            }
            ret = lr_copy_content(handle->mirrorlist_fd, fd);
            close(fd);
            if (ret != 0) {
                g_debug("%s: Cannot copy content of mirrorlist file", __func__);
                lr_free(ml_file_path);
                return LRE_IO;
            }
            repo->mirrorlist = ml_file_path;
        }

        /* Prepare repomd.xml file */
        path = lr_pathconcat(handle->destdir, "/repodata/repomd.xml", NULL);
        fd = open(path, O_CREAT|O_TRUNC|O_RDWR, 0666);
        if (fd == -1) {
            lr_free(path);
            return LRE_IO;
        }

        /* Download repomd.xml */
        rc = lr_yum_download_repomd(handle, handle->metalink, fd);
        if (rc != LRE_OK) {
            close(fd);
            lr_free(path);
            return rc;
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
                close(fd);
                lr_free(path);
                lr_free(signature);
                return LRE_IO;
            }

            url = lr_pathconcat(handle->used_mirror, "repodata/repomd.xml.asc", NULL);
            rc = lr_download_url(handle, url, fd_sig, NULL);
            lr_free(url);
            close(fd_sig);
            if (rc != LRE_OK) {
                // Signature doesn't exist
                g_debug("%s: GPG signature doesn't exists", __func__);
                unlink(signature);
                lr_free(signature);
            } else {
                // Signature downloaded
                repo->signature = lr_strdup(signature);
                rc = lr_gpg_check_signature(signature, path, NULL, NULL);
                if (rc != LRE_OK) {
                    g_debug("%s: GPG signature verification failed", __func__);
                    close(fd);
                    lr_free(path);
                    lr_free(signature);
                    return rc;
                }
                g_debug("%s: GPG signature successfully verified", __func__);
            }
        }

        lseek(fd, 0, SEEK_SET);

        /* Parse repomd */
        g_debug("%s: Parsing repomd.xml", __func__);
        rc = lr_yum_repomd_parse_file(repomd, fd, NULL, NULL, NULL);
        close(fd);
        if (rc != LRE_OK) {
            g_debug("%s: Parsing unsuccessful (%d)", __func__, rc);
            lr_free(path);
            return rc;
        }

        /* Fill result object */
        result->destdir = lr_strdup(handle->destdir);
        repo->destdir = lr_strdup(handle->destdir);
        repo->repomd = path;
        if (handle->used_mirror)
            repo->url = lr_strdup(handle->used_mirror);
        else
            repo->url = lr_strdup(handle->baseurl);

        g_debug("%s: Repomd revision: %s", repomd->revision, __func__);
    }

    /* Download rest of metadata files */
    rc = lr_yum_download_repo(handle, repo, repomd);
    g_debug("%s: Repository download rc: %d (%s)", __func__, rc, lr_strerror(rc));
    return rc;
}

int
lr_yum_perform(lr_Handle handle, lr_Result result)
{
    int rc = LRE_OK;
    lr_YumRepo repo;
    lr_YumRepoMd *repomd;

    assert(handle);

    if (!result)
        return LRE_BADFUNCARG;

    if (!handle->baseurl && !handle->mirrorlist)
        return LRE_NOURL;

    if (handle->local && !handle->baseurl)
        return LRE_NOURL;

    if (handle->update) {
        /* Download/Locate only specified files */
        if (!result->yum_repo || !result->yum_repomd)
            return LRE_INCOMPLETERESULT;
    } else {
        /* Download/Locate from scratch */
        if (result->yum_repo || result->yum_repomd)
            return LRE_ALREADYUSEDRESULT;
        result->yum_repo = lr_yum_repo_init();
        result->yum_repomd = lr_yum_repomd_init();
    }

    repo   = result->yum_repo;
    repomd = result->yum_repomd;

    if (handle->local) {
        /* Do not duplicate repository, just use the existing local one */
        rc = lr_yum_use_local(handle, result);
        if (rc != LRE_OK)
            return rc;
        if (handle->checks & LR_CHECK_CHECKSUM)
            rc = lr_yum_check_repo_checksums(repo, repomd);
    } else {
        /* Download remote/Duplicate local repository */
        rc = lr_yum_download_remote(handle, result);
        /* All checksums are checked while downloading */
    }

    return rc;
}

double
lr_yum_repomd_get_age(lr_Result r)
{
    assert(r);

    if (!r->yum_repo || !r->yum_repo->repomd)
        return 0.0;

    int rc;
    struct stat st;

    rc = stat(r->yum_repo->repomd, &st);
    if (rc != 0)
        return 0.0;

   return difftime(time(NULL), st.st_mtime);
}
