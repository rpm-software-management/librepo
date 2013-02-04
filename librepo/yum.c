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

#include "setup.h"
#include "yum.h"
#include "util.h"
#include "metalink.h"
#include "mirrorlist.h"
#include "repomd.h"
#include "curl.h"
#include "checksum.h"
#include "handle_internal.h"
#include "result_internal.h"
#include "yum_internal.h"
#include "internal_mirrorlist.h"
#include "curltargetlist.h"
#include "gpg.h"

/** TODO:
 * - GPG check
 */

/* helper function for YumRepo manipulation */

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

char *
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
    if (handle->yumdlist) {
        int x = 0;
        while (handle->yumdlist[x]) {
            if (!strcmp(handle->yumdlist[x], type))
                return 1;;
            x++;
        }
        return 0;
    }
    return 1;
}

int
lr_yum_download_repomd(lr_Handle handle,
                       lr_Metalink metalink,
                       int fd)
{
    int rc = LRE_OK;
    lr_ChecksumType checksum_type = LR_CHECKSUM_UNKNOWN;
    char *checksum = NULL;


    DPRINTF("%s: Downloading repomd.xml via mirrorlist\n", __func__);

    if (metalink && (handle->checks & LR_CHECK_CHECKSUM)) {
        /* Select the best checksum type */
        for (int x = 0; x < metalink->noh; x++) {
            lr_ChecksumType mtype;
            lr_MetalinkHash mhash = metalink->hashes[x];

            if (!mhash->type || !mhash->value)
                continue;

            mtype = lr_checksum_type(mhash->type);
            if (mtype != LR_CHECKSUM_UNKNOWN && mtype > checksum_type) {
                checksum_type = mtype;
                checksum = mhash->value;
            }
        }

        DPRINTF("%s: selected repomd.xml checksum to check: (%s) %s\n",
                __func__, lr_checksum_type_to_str(checksum_type), checksum);
    }

    rc = lr_curl_single_mirrored_download(handle,
                                          "repodata/repomd.xml",
                                          fd,
                                          checksum_type,
                                          checksum);

    if (rc != LRE_OK) {
        /* Download of repomd.xml was not successful */
        DPRINTF("%s: repomd.xml download was unsuccessful\n", __func__);
        return rc;
    }

    return LRE_OK;
}

int
lr_yum_download_repo(lr_Handle handle, lr_YumRepo repo, lr_YumRepoMd repomd)
{
    int ret = LRE_OK;
    char *destdir;  /* Destination dir */
    lr_CurlTargetList targets = lr_curltargetlist_new();

    destdir = handle->destdir;
    DEBUGASSERT(destdir);
    DEBUGASSERT(strlen(destdir));

    for (int x = 0; x < repomd->nor; x++) {
        int fd;
        char *path;
        lr_CurlTarget target;
        lr_YumRepoMdRecord record = repomd->records[x];

        if (!lr_yum_repomd_record_enabled(handle, record->type))
            continue;

        path = lr_pathconcat(destdir, record->location_href, NULL);
        fd = open(path, O_CREAT|O_TRUNC|O_RDWR, 0660);
        if (fd < 0) {
            DPRINTF("%s: Cannot create/open %s (%s)\n",
                    __func__, path, strerror(errno));
            lr_free(path);
            return LRE_IO;
        }

        target = lr_curltarget_new();
        target->path = lr_strdup(record->location_href);
        target->fd = fd;
        target->checksum_type = lr_checksum_type(record->checksum_type);
        target->checksum = lr_strdup(record->checksum);
        lr_curltargetlist_append(targets, target);

        /* Becouse path may already exists in repo (while update) */
        lr_yum_repo_update(repo, record->type, path);
        lr_free(path);
    }

    if (lr_curltargetlist_len(targets) > 0)
        ret = lr_curl_multi_download(handle, targets);

    lr_curltargetlist_free(targets);
    return ret;
}

int
lr_yum_check_checksum_of_md_record(lr_YumRepoMdRecord rec, char *path)
{
    int ret, fd;
    char *expected_checksum;
    lr_ChecksumType checksum_type;

    if (!rec || !path)
        return LRE_OK;

    expected_checksum = rec->checksum;
    checksum_type = lr_checksum_type(rec->checksum_type);

    DPRINTF("%s: Checking checksum of %s (expected: %s [%s])\n",
                       __func__, path, expected_checksum, rec->checksum_type);

    if (!expected_checksum) {
        DPRINTF("%s: No checksum in repomd\n", __func__);
        return LRE_OK;  /* Empty checksum - suppose it's ok */
    }

    if (checksum_type == LR_CHECKSUM_UNKNOWN) {
        DPRINTF("%s: Unknown checksum: %s\n", __func__, rec->checksum_type);
        return LRE_UNKNOWNCHECKSUM;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        DPRINTF("%s: Cannot open %s\n", __func__, path);
        return LRE_IO;
    }

    ret = lr_checksum_fd_cmp(checksum_type, fd, expected_checksum);

    close(fd);

    if (ret) {
        DPRINTF("%s: Checksum check - Failed\n", __func__);
        return LRE_BADCHECKSUM;
    }

    DPRINTF("%s: Checksum check - Passed\n", __func__);

    return LRE_OK;
}

int
lr_yum_check_repo_checksums(lr_YumRepo repo, lr_YumRepoMd repomd)
{
    for (int x=0; x < repomd->nor; x++) {
        int ret;
        lr_YumRepoMdRecord record  = repomd->records[x];
        char *path = lr_yum_repo_path(repo, record->type);
        ret = lr_yum_check_checksum_of_md_record(record, path);
        DPRINTF("%s: Checksum rc: %d (%s)\n", __func__, ret, record->type);
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
    lr_YumRepoMd repomd;

    DPRINTF("%s: Locating repo..\n", __func__);

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
        /* Open and parse repomd */
        path = lr_pathconcat(baseurl, "repodata/repomd.xml", NULL);
        fd = open(path, O_RDONLY);
        if (fd < 0) {
            DPRINTF("%s: open(%s): %s\n", __func__, path, strerror(errno));
            lr_free(path);
            return LRE_IO;
        }

        DPRINTF("%s: Parsing repomd.xml\n", __func__);
        rc = lr_yum_repomd_parse_file(repomd, fd);
        if (rc != LRE_OK) {
            DPRINTF("%s: Parsing unsuccessful (%d)\n", __func__, rc);
            lr_free(path);
            return rc;
        }

        close(fd);

        /* Fill result object */
        result->destdir = lr_strdup(baseurl);
        repo->destdir = lr_strdup(baseurl);
        repo->repomd = path;

        DPRINTF("%s: Repomd revision: %s\n", __func__, repomd->revision);
    }

    /* Locate rest of metadata files */
    for (int x = 0; x < repomd->nor; x++) {
        char *path;
        lr_YumRepoMdRecord record = repomd->records[x];

        if (!lr_yum_repomd_record_enabled(handle, record->type))
            continue;
        if (lr_yum_repo_path(repo, record->type))
            continue; /* This path already exists in repo */

        path = lr_pathconcat(baseurl, record->location_href, NULL);
        if (path)
            lr_yum_repo_append(repo, record->type, path);
    }

    DPRINTF("%s: Repository was successfully located\n", __func__);
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
    lr_YumRepoMd repomd;

    DPRINTF("%s: Downloading/Copying repo..\n", __func__);

    rc = lr_handle_prepare_internal_mirrorlist(handle, "repodata/repomd.xml");
    if (rc != LRE_OK)
        return rc;

    repo   = result->yum_repo;
    repomd = result->yum_repomd;

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
            DPRINTF("%s: Cannot create dir: %s (%s)\n",
                    __func__, path_to_repodata, strerror(errno));
            lr_free(path_to_repodata);
            return LRE_CANNOTCREATEDIR;
        }
    }
    lr_free(path_to_repodata);

    if (!handle->update) {
        /* Prepare repomd.xml file */
        char *path;
        path = lr_pathconcat(handle->destdir, "/repodata/repomd.xml", NULL);
        fd = open(path, O_CREAT|O_TRUNC|O_RDWR, 0660);
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
            fd_sig = open(signature, O_CREAT|O_TRUNC|O_RDWR, 0660);
            if (fd_sig == -1) {
                DPRINTF("%s: Cannot open: %s\n", __func__, signature);
                close(fd);
                lr_free(path);
                lr_free(signature);
                return LRE_IO;
            }

            url = lr_pathconcat(handle->used_mirror, "repodata/repomd.xml.asc", NULL);
            rc = lr_curl_single_download(handle, url, fd_sig);
            lr_free(url);
            close(fd_sig);
            if (rc != LRE_OK) {
                // Signature doesn't exist
                DPRINTF("%s: GPG signature doesn't exists\n", __func__);
                unlink(signature);
                lr_free(signature);
            } else {
                // Signature downloaded
                repo->signature = lr_strdup(signature);
                rc = lr_gpg_check_signature(signature, path, NULL);
                if (rc != LRE_OK) {
                    DPRINTF("%s: GPG signature verification failed\n", __func__);
                    close(fd);
                    lr_free(path);
                    lr_free(signature);
                    return rc;
                }
                DPRINTF("%s: GPG signature successfully verified\n", __func__);
            }
        }

        lseek(fd, 0, SEEK_SET);

        /* Parse repomd */
        DPRINTF("%s: Parsing repomd.xml\n", __func__);
        rc = lr_yum_repomd_parse_file(repomd, fd);
        close(fd);
        if (rc != LRE_OK) {
            DPRINTF("%s: Parsing unsuccessful (%d)\n", __func__, rc);
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

        DPRINTF("%s: Repomd revision: %s\n", repomd->revision, __func__);
    }

    /* Download rest of metadata files */
    if ((rc = lr_yum_download_repo(handle, repo, repomd)) != LRE_OK)
        return rc;

    DPRINTF("%s: Repository was successfully downloaded\n", __func__);
    return LRE_OK;
}

int
lr_yum_perform(lr_Handle handle, lr_Result result)
{
    int rc = LRE_OK;
    lr_YumRepo repo;
    lr_YumRepoMd repomd;

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
        if (handle->checks & LR_CHECK_CHECKSUM)
            rc = lr_yum_check_repo_checksums(repo, repomd);
    } else {
        /* Download remote/Duplicate local repository */
        rc = lr_yum_download_remote(handle, result);
        /* All checksums are checked while downloading */
    }

    return rc;
}
