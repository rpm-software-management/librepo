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

#ifndef LR_TYPES_H
#define LR_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _lr_Handle *lr_Handle;
typedef struct _lr_Result *lr_Result;

typedef enum {
    LR_CHECK_GPG        = (1<<0),
    LR_CHECK_CHECKSUM   = (1<<1),
} lr_Checks;

typedef enum {
    LR_YUMREPO          = (1<<1),
    LR_SUSEREPO         = (1<<2),   /*!< Not implemented yet */
    LR_DEBREPO          = (1<<3),   /*!< Not implemented yet */
} lr_Repotype;

/* YUM related types */

/* Some common used arrays for LRO_YUMDLIST */
#define LR_YUM_FULL         NULL
#define LR_YUM_REPOMDONLY   [NULL]
#define LR_YUM_BASEXML      ["primary", "filelists", "other", NULL]
#define LR_YUM_BASEDB       ["primary_db", "filelists_db", "other_db", NULL]
#define LR_YUM_HAWKEY       ["primary", "filelists", "prestodelta", NULL]

struct _lr_YumDistroTag {
    char *cpeid;
    char *value;
};
typedef struct _lr_YumDistroTag *lr_YumDistroTag;

struct _lr_YumRepoMdRecord {
    char *type;
    char *location_href;
    char *location_base;
    char *checksum;
    char *checksum_type;
    char *checksum_open;
    char *checksum_open_type;
    long timestamp;
    long size;
    long size_open;
    int db_version;
};
typedef struct _lr_YumRepoMdRecord *lr_YumRepoMdRecord;

#define LR_NUM_OF_YUM_REPOMD_RECORDS    12  /*!< number of repomd records
                                                 in _lr_YumRepoMd structure */

struct _lr_YumRepoMd {
    char *revision;
    char **repo_tags;
    lr_YumDistroTag *distro_tags;
    char **content_tags;
    lr_YumRepoMdRecord *records;

    int nort; /* number of repo tags */
    int nodt; /* number of distro tags */
    int noct; /* number of content tags */
    int nor;  /* number of records */
};
typedef struct _lr_YumRepoMd *lr_YumRepoMd;

struct _lr_YumRepoPath {
    char *type;  /* e.g. primary */
    char *path;  /* e.g. foo/bar/repodata/primary.xml */
};
typedef struct _lr_YumRepoPath *lr_YumRepoPath;

struct _lr_YumRepo {
    int nop; /* Number of paths */
    lr_YumRepoPath *paths;

    char *repomd;
    char *url;          /*!< URL from where repo was downloaded */
    char *destdir;      /*!< Local path to the repo */
};
typedef struct _lr_YumRepo *lr_YumRepo;

/* Callbacks */

typedef int (*lr_ProgressCb)(void *clientp,
                              double total_to_download,
                              double now_downloaded);

typedef int (*lr_UpdateCb)(lr_YumRepoMd first, lr_YumRepoMd second);

#ifdef __cplusplus
}
#endif

#endif
