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

#ifndef __LR_YUMREPO_H__
#define __LR_YUMREPO_H__

#include <glib.h>

#include <librepo/handle.h>
#include <librepo/metalink.h>
#include <librepo/rcodes.h>
#include <librepo/repomd.h>
#include <librepo/result.h>

G_BEGIN_DECLS

/** \defgroup   yum       Yum repo manipulation
 *  \addtogroup yum
 *  @{
 */

/** Path to single metadata file from repomd.xml. */
typedef struct {
    char *type;  /*!< Type of record (e.g. "primary") */
    char *path;  /*!< Path to the file (e.g. foo/bar/repodata/primary.xml) */
} LrYumRepoPath;

/** Yum repository */
typedef struct {
    GSList *paths;      /*!< Paths to repo files. List of ::LrYumRepoPath*s */
    char *repomd;       /*!< Path to repomd.xml */
    char *url;          /*!< URL from where repo was downloaded */
    char *destdir;      /*!< Local path to the repo */
    char *signature;    /*!< Path to signature if available and
                             signature was downloaded (GPG check
                             was enabled during repo downloading) */
    char *mirrorlist;   /*!< Mirrolist filename */
    char *metalink;     /*!< Metalink filename */
    gboolean use_zchunk; /*!< Use zchunk in this repo */
} LrYumRepo;

/** Mirror Failure Callback Data
 */
typedef struct CbData_s {
    void *userdata;                 /*!< User data */
    void *cbdata;                   /*!< User's callback data */
    LrProgressCb progresscb;        /*!< Progress callback */
    LrHandleMirrorFailureCb hmfcb;  /*!< Handle mirror failure callback */
    char *metadata;                 /*!< "primary", "filelists", ... */
} CbData;

/** Allocate new yum repo object.
 * @return              New yum repo object.
 */
LrYumRepo *
lr_yum_repo_init(void);

/** Free yum repo - free its item and the repo itself.
 * @param repo          Yum repo object.
 */
void
lr_yum_repo_free(LrYumRepo *repo);

/** Retruns path for the file from repository.
 * @param repo          Yum repo object.
 * @param type          Type of path. E.g. "primary", "filelists", ...
 * @return              Path or NULL.
 */
const char *
lr_yum_repo_path(LrYumRepo *repo, const char *type);

/**
 * Handle mirror failure callback
 * @param clientp Pointer to user data.
 * @param msg Error message.
 * @param url Mirror URL.
 * @return See LrCbReturnCode codes
 */
int
hmfcb(void *clientp, const char *msg, const char *url);

/** Prepares directory for repo data
 * @param handle        Handle object containing path to repo data
 * @param err           Object for storing errors
 * @return              True on success
 */
gboolean
lr_prepare_repodata_dir(LrHandle *handle, GError **err);


/** Stores mirror list files
 * @param handle        Handle object containing path to mirror list
 * @param repo          Yum repository
 * @param err           Object for storing errors
 * @return              True on success
 */
gboolean
lr_store_mirrorlist_files(LrHandle *handle, LrYumRepo *repo, GError **err);

/** Copies metalink content
 * @param handle        Handle object containing dest dir path
 * @param repo          Yum repository
 * @param err           Object for storing errors
 * @return              True on success
 */
gboolean
lr_copy_metalink_content(LrHandle *handle, LrYumRepo *repo, GError **err);

/** Prepares repomd.xml file
 * @param handle        Handle object containing dest dir path
 * @param path
 * @param err           Object for storing errors
 * @return              File descriptor of repomd.xml file
 */
int
lr_prepare_repomd_xml_file(LrHandle *handle, char **path, GError **err);

gboolean
lr_check_repomd_xml_asc_availability(LrHandle *handle, LrYumRepo *repo, int fd, char *path, GError **err);

/** Stores best checksum on the beginning of @param checksums
 * @param metalink      Metalink
 * @param checksums     List of checksums
 */
void
lr_get_best_checksum(const LrMetalink *metalink, GSList **checksums);

/** Returns mirror failure callback data
 * @param handle        Handle object
 * @return              Mirror Failure Callback Data
 */
CbData *
lr_get_metadata_failure_callback(const LrHandle *handle);

/**
 *
 * @param targets
 * @param err
 * @return return TRUE on success, FALSE otherwise
 */
gboolean
lr_yum_download_repos(GSList *targets,
                      GError **err);

/** @} */

G_END_DECLS

#endif
