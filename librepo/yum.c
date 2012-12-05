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
#include "internal_mirrorlist.h"
#include "curltargetlist.h"

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
    lr_free(repo->repomd);
    lr_free(repo->primary);
    lr_free(repo->filelists);
    lr_free(repo->other);
    lr_free(repo->primary_db);
    lr_free(repo->filelists_db);
    lr_free(repo->other_db);
    lr_free(repo->group);
    lr_free(repo->group_gz);
    lr_free(repo->prestodelta);
    lr_free(repo->deltainfo);
    lr_free(repo->updateinfo);
    lr_free(repo->origin);
    lr_free(repo->url);
    lr_free(repo->destdir);
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

/* main bussines logic */

int
lr_yum_download_repomd(lr_Handle handle,
                       lr_Metalink metalink,
                       int fd)
{
    int rc = LRE_OK;
    lr_ChecksumType checksum_type = LR_CHECKSUM_UNKNOWN;
    char *checksum = NULL;


   DEBUGF(fprintf(stderr, "Downloading repomd.xml via mirrorlist\n"));

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
    }

    rc = lr_curl_single_mirrored_download(handle,
                                          "repodata/repomd.xml",
                                          fd,
                                          checksum_type,
                                          checksum);

    if (rc != LRE_OK) {
        /* Download of repomd.xml was not successful */
        DEBUGF(fprintf(stderr, "repomd.xml download was unsuccessful"));
        return rc;
    }

    return LRE_OK;
}

void
lr_add_target(lr_CurlTargetList targets,
              const char *destdir,
              lr_YumRepoMdRecord rec,
              char **local_path,
              int use_this)
{
    int fd;
    lr_CurlTarget target;

    if (!use_this)
        return;

    DEBUGASSERT(destdir);
    DEBUGASSERT(strlen(destdir));

    if (!rec || !rec->location_href)
        return;

    if (*local_path)  /* During update local_path may be already existing */
        lr_free(*local_path);
    *local_path = lr_pathconcat(destdir, rec->location_href, NULL);
    fd = open(*local_path, O_CREAT|O_TRUNC|O_RDWR, 0660);
    if (fd < 0) {
        lr_free(*local_path);
        return;
    }

    target = lr_curltarget_new();
    target->path = lr_strdup(rec->location_href);
    target->fd = fd;
    lr_curltargetlist_append(targets, target);
}

int
lr_yum_download_repo(lr_Handle handle, lr_YumRepo repo, lr_YumRepoMd repomd)
{
    int ret = LRE_OK;
    char *dir;  /* Destination dir */
    lr_CurlTargetList targets = lr_curltargetlist_new();

    dir = handle->destdir;

    lr_add_target(targets, dir, repomd->primary, &repo->primary,
                          handle->yumflags & LR_YUM_PRI);
    lr_add_target(targets, dir, repomd->filelists, &repo->filelists,
                          handle->yumflags & LR_YUM_FIL);
    lr_add_target(targets, dir, repomd->other, &repo->other,
                          handle->yumflags & LR_YUM_OTH);
    lr_add_target(targets, dir, repomd->primary_db, &repo->primary_db,
                          handle->yumflags & LR_YUM_PRIDB);
    lr_add_target(targets, dir, repomd->filelists_db, &repo->filelists_db,
                          handle->yumflags & LR_YUM_FILDB);
    lr_add_target(targets, dir, repomd->other_db, &repo->other_db,
                          handle->yumflags & LR_YUM_OTHDB);
    lr_add_target(targets, dir, repomd->group, &repo->group,
                          handle->yumflags & LR_YUM_GROUP);
    lr_add_target(targets, dir, repomd->group_gz, &repo->group_gz,
                          handle->yumflags & LR_YUM_GROUPGZ);
    lr_add_target(targets, dir, repomd->prestodelta, &repo->prestodelta,
                          handle->yumflags & LR_YUM_PRESTODELTA);
    lr_add_target(targets, dir, repomd->deltainfo, &repo->deltainfo,
                          handle->yumflags & LR_YUM_DELTAINFO);
    lr_add_target(targets, dir, repomd->updateinfo, &repo->updateinfo,
                          handle->yumflags & LR_YUM_UPDATEINFO);
    lr_add_target(targets, dir, repomd->origin, &repo->origin,
                          handle->yumflags & LR_YUM_ORIGIN);

    if (lr_curltargetlist_len(targets) > 0) {
        ret = lr_curl_multi_download(handle, targets);
    }

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

    DEBUGF(fprintf(stderr, "Checking checksum of %s (expected: %s [%s])\n",
                           path, expected_checksum, rec->checksum_type));

    if (!expected_checksum) {
        DEBUGF(fprintf(stderr, "No checksum in repomd\n"));
        return LRE_OK;  /* Empty checksum - suppose it's ok */
    }

    if (checksum_type == LR_CHECKSUM_UNKNOWN) {
        DEBUGF(fprintf(stderr, "Unknown checksum: %s\n", rec->checksum_type));
        return LRE_UNKNOWNCHECKSUM;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        DEBUGF(fprintf(stderr, "Cannot open %s\n", path));
        return LRE_IO;
    }

    ret = lr_checksum_fd_cmp(checksum_type, fd, expected_checksum);

    close(fd);

    if (ret) {
        DEBUGF(fprintf(stderr, "Checksum check - Failed\n"));
        return LRE_BADCHECKSUM;
    }

    DEBUGF(fprintf(stderr, "Checksum check - Passed\n"));

    return LRE_OK;
}

int
lr_yum_check_repo_checksums(lr_YumRepo repo, lr_YumRepoMd repomd)
{
    int ret;

    ret = lr_yum_check_checksum_of_md_record(repomd->primary,
                                             repo->primary);
    DEBUGF(fprintf(stderr, "Checksum rc: %d (primary)\n", ret));
    if (ret != LRE_OK) return ret;
    ret = lr_yum_check_checksum_of_md_record(repomd->filelists,
                                             repo->filelists);
    DEBUGF(fprintf(stderr, "Checksum rc: %d (filelists)\n", ret));
    if (ret != LRE_OK) return ret;
    ret = lr_yum_check_checksum_of_md_record(repomd->other,
                                             repo->other);
    DEBUGF(fprintf(stderr, "Checksum rc: %d (other)\n", ret));
    if (ret != LRE_OK) return ret;
    ret = lr_yum_check_checksum_of_md_record(repomd->primary_db,
                                             repo->primary_db);
    DEBUGF(fprintf(stderr, "Checksum rc: %d (primary_db)\n", ret));
    if (ret != LRE_OK) return ret;
    ret = lr_yum_check_checksum_of_md_record(repomd->filelists_db,
                                             repo->filelists_db);
    DEBUGF(fprintf(stderr, "Checksum rc: %d (filelists_db)\n", ret));
    if (ret != LRE_OK) return ret;
    ret = lr_yum_check_checksum_of_md_record(repomd->other_db,
                                             repo->other_db);
    DEBUGF(fprintf(stderr, "Checksum rc: %d (other_db)\n", ret));
    if (ret != LRE_OK) return ret;
    ret = lr_yum_check_checksum_of_md_record(repomd->group,
                                             repo->group);
    DEBUGF(fprintf(stderr, "Checksum rc: %d (group)\n", ret));
    if (ret != LRE_OK) return ret;
    ret = lr_yum_check_checksum_of_md_record(repomd->group_gz,
                                             repo->group_gz);
    DEBUGF(fprintf(stderr, "Checksum rc: %d (group_gz)\n", ret));
    if (ret != LRE_OK) return ret;
    ret = lr_yum_check_checksum_of_md_record(repomd->prestodelta,
                                             repo->prestodelta);
    DEBUGF(fprintf(stderr, "Checksum rc: %d (prestodelta)\n", ret));
    if (ret != LRE_OK) return ret;
    ret = lr_yum_check_checksum_of_md_record(repomd->deltainfo,
                                             repo->deltainfo);
    DEBUGF(fprintf(stderr, "Checksum rc: %d (deltainfo)\n", ret));
    if (ret != LRE_OK) return ret;
    ret = lr_yum_check_checksum_of_md_record(repomd->updateinfo,
                                             repo->updateinfo);
    DEBUGF(fprintf(stderr, "Checksum rc: %d (updateinfo)\n", ret));
    if (ret != LRE_OK) return ret;
    ret = lr_yum_check_checksum_of_md_record(repomd->origin,
                                             repo->origin);
    DEBUGF(fprintf(stderr, "Checksum rc: %d (origin)\n", ret));
    if (ret != LRE_OK) return ret;

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

    DEBUGF(fprintf(stderr, "Locating repo..\n"));

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
            DEBUGF(fprintf(stderr, "%s: open(%s): %s\n", __func__, path, strerror(errno)));
            lr_free(path);
            return LRE_IO;
        }

        DEBUGF(fprintf(stderr, "Parsing repomd.xml\n"));
        rc = lr_yum_repomd_parse_file(repomd, fd);
        if (rc != LRE_OK) {
            DEBUGF(fprintf(stderr, "Parsing unsuccessful (%d)\n", rc));
            lr_free(path);
            return rc;
        }

        close(fd);

        /* Fill result object */
        result->destdir = lr_strdup(baseurl);
        repo->destdir = lr_strdup(baseurl);
        repo->repomd = path;

        DEBUGF(fprintf(stderr, "Repomd revision: %s\n", repomd->revision));
    }

    /* Locate rest of metadata files */
    /* First condition !repo->* is for cases when handle->update is true,
     * then we don't need to generate paths we already have */
    if (!repo->primary && repomd->primary && handle->yumflags & LR_YUM_PRI)
        repo->primary = lr_pathconcat(baseurl,
                            repomd->primary->location_href,
                            NULL);
    if (!repo->filelists && repomd->filelists && handle->yumflags & LR_YUM_FIL)
        repo->filelists = lr_pathconcat(baseurl,
                            repomd->filelists->location_href,
                            NULL);
    if (!repo->other && repomd->other && handle->yumflags & LR_YUM_OTH)
        repo->other = lr_pathconcat(baseurl,
                            repomd->other->location_href,
                            NULL);
    if (!repo->primary_db && repomd->primary_db && handle->yumflags & LR_YUM_PRIDB)
        repo->primary_db = lr_pathconcat(baseurl,
                            repomd->primary_db->location_href,
                            NULL);
    if (!repo->filelists_db && repomd->filelists_db && handle->yumflags & LR_YUM_FILDB)
        repo->filelists_db = lr_pathconcat(baseurl,
                            repomd->filelists_db->location_href,
                            NULL);
    if (!repo->other_db && repomd->other_db && handle->yumflags & LR_YUM_OTHDB)
        repo->other_db = lr_pathconcat(baseurl,
                            repomd->other_db->location_href,
                            NULL);
    if (!repo->group && repomd->group && handle->yumflags & LR_YUM_GROUP)
        repo->group = lr_pathconcat(baseurl,
                            repomd->group->location_href,
                            NULL);
    if (!repo->group_gz && repomd->group_gz && handle->yumflags & LR_YUM_GROUPGZ)
        repo->group_gz = lr_pathconcat(baseurl,
                            repomd->group_gz->location_href,
                            NULL);
    if (!repo->prestodelta && repomd->prestodelta && handle->yumflags & LR_YUM_PRESTODELTA)
        repo->prestodelta = lr_pathconcat(baseurl,
                            repomd->prestodelta->location_href,
                            NULL);
    if (!repo->deltainfo && repomd->deltainfo && handle->yumflags & LR_YUM_DELTAINFO)
        repo->deltainfo = lr_pathconcat(baseurl,
                            repomd->deltainfo->location_href,
                            NULL);
    if (!repo->updateinfo && repomd->updateinfo && handle->yumflags & LR_YUM_UPDATEINFO)
        repo->updateinfo = lr_pathconcat(baseurl,
                            repomd->updateinfo->location_href,
                            NULL);
    if (!repo->origin && repomd->origin && handle->yumflags & LR_YUM_ORIGIN)
        repo->origin = lr_pathconcat(baseurl,
                            repomd->origin->location_href,
                            NULL);

    DEBUGF(fprintf(stderr, "Repository was successfully located\n"));
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

    DEBUGF(fprintf(stderr, "Downloading/Copying repo..\n"));

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
        lr_free(path_to_repodata);
        if (rc == -1)
            return LRE_CANNOTCREATEDIR;
    }

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

        lseek(fd, 0, SEEK_SET);

        /* Parse repomd */
        DEBUGF(fprintf(stderr, "Parsing repomd.xml\n"));
        rc = lr_yum_repomd_parse_file(repomd, fd);
        close(fd);
        if (rc != LRE_OK) {
            DEBUGF(fprintf(stderr, "Parsing unsuccessful (%d)\n", rc));
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

        DEBUGF(fprintf(stderr, "Repomd revision: %s\n", repomd->revision));
    }

    /* Download rest of metadata files */
    if ((rc = lr_yum_download_repo(handle, repo, repomd)) != LRE_OK)
        return rc;

    DEBUGF(fprintf(stderr, "Repository was successfully downloaded\n"));
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

    if (handle->local)
        /* Do not duplicate repository, just use the existing local one */
        rc = lr_yum_use_local(handle, result);
    else
        /* Download remote/Duplicate local repository */
        rc = lr_yum_download_remote(handle, result);

    if (rc != LRE_OK)
        return rc;

    /* Check checksums */
    if (handle->checks & LR_CHECK_CHECKSUM) {
        if ((rc = lr_yum_check_repo_checksums(repo, repomd)) != LRE_OK)
            return rc;
        DEBUGF(fprintf(stderr, "All checksums in repository seems to be valid\n"));
    }

    return rc;
}
