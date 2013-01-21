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

/** \defgroup   types   Basic types and constants
 */

/** \ingroup types
 * Handle object containing configration for repository metadata and
 * package downloading.
 */
typedef struct _lr_Handle *lr_Handle;

/** \ingroup types
 * Result object containing information about downloaded/located repository.
 */
typedef struct _lr_Result *lr_Result;

/** \ingroup types
 * Flags for available checks.
 */
typedef enum {
    LR_CHECK_GPG        = (1<<0),   /*!< GPG check */
    LR_CHECK_CHECKSUM   = (1<<1),   /*!< Checksum check */
} lr_Checks;

/** \ingroup types
 * Repo types flags.
 */
typedef enum {
    LR_YUMREPO          = (1<<1),   /*!< Yum repository */
    LR_SUSEREPO         = (1<<2),   /*!< YaST2 repository - Not implemented yet */
    LR_DEBREPO          = (1<<3),   /*!< Debian repository - Not implemented yet */
} lr_Repotype;

/* YUM related types */

/* Some common used arrays for LRO_YUMDLIST */

/** \ingroup types
 * Predefined value for LRO_YUMDLIST option - Download whole repo.
 */
#define LR_YUM_FULL         NULL

/** \ingroup types
 * Predefined value for LRO_YUMDLIST option - Download only repomd.xml.
 */
#define LR_YUM_REPOMDONLY   [NULL]

/** \ingroup types
 * Predefined value for LRO_YUMDLIST option - Download only base xml files.
 */
#define LR_YUM_BASEXML      ["primary", "filelists", "other", NULL]

/** \ingroup types
 * Predefined value for LRO_YUMDLIST option - Download only base db files.
 */
#define LR_YUM_BASEDB       ["primary_db", "filelists_db", "other_db", NULL]

/** \ingroup types
 * Predefined value for LRO_YUMDLIST option - Download only primary,
 * filelists and prestodelta.
 */
#define LR_YUM_HAWKEY       ["primary", "filelists", "prestodelta", NULL]

/** \ingroup types
 * Yum repomd distro tag.
 */
struct _lr_YumDistroTag {
    char *cpeid;    /*!< Tag cpeid value or NULL. */
    char *value;    /*!< Tag value. */
};

/** \ingroup types
 * Pointer to ::_lr_YumDistroTag.
 */
typedef struct _lr_YumDistroTag *lr_YumDistroTag;

/** \ingroup types
 * Yum repomd record.
 */
struct _lr_YumRepoMdRecord {
    char *type;                 /*!< Type of record (e.g. "primary") */
    char *location_href;        /*!< Location href attribute */
    char *location_base;        /*!< Location base attribute */
    char *checksum;             /*!< Checksum value */
    char *checksum_type;        /*!< Type of checksum */
    char *checksum_open;        /*!< Checksum of uncompressed file */
    char *checksum_open_type;   /*!< Type of checksum of uncompressed file */
    long timestamp;             /*!< File timestamp */
    long size;                  /*!< File size */
    long size_open;             /*!< Size of uncompressed file */
    int db_version;             /*!< Version of database */
};

/** \ingroup types
 * Pointer to ::_lr_YumRepoMdRecord
 */
typedef struct _lr_YumRepoMdRecord *lr_YumRepoMdRecord;

/** \ingroup types
 * Yum repomd.xml.
 */
struct _lr_YumRepoMd {
    char *revision;                 /*!< Revision string*/
    char **repo_tags;               /*!< List of repo tags */
    lr_YumDistroTag *distro_tags;   /*!< List of distro tags */
    char **content_tags;            /*!< List of content tags */
    lr_YumRepoMdRecord *records;    /*!< List of repomd records */

    int nort;   /*!< Number of repo tags */
    int nodt;   /*!< Number of distro tags */
    int noct;   /*!< Number of content tags */
    int nor;    /*!< Number of records */
};

/** \ingroup types
 * Pointer to ::_lr_YumRepoMd
 */
typedef struct _lr_YumRepoMd *lr_YumRepoMd;

/** \ingroup types
 * Path to single metadata file from repomd.xml.
 */
struct _lr_YumRepoPath {
    char *type;  /*!< Type of record (e.g. "primary") */
    char *path;  /*!< Path to the file (e.g. foo/bar/repodata/primary.xml) */
};

/** \ingroup types
 * Pointer to ::_lr_YumRepoPath
 */
typedef struct _lr_YumRepoPath *lr_YumRepoPath;

/** \ingroup types
 * Yum repository
 */
struct _lr_YumRepo {
    int nop;                /*!< Number of paths */
    lr_YumRepoPath *paths;  /*!< Paths to repo files */

    char *repomd;           /*!< Path to repomd.xml */
    char *url;              /*!< URL from where repo was downloaded */
    char *destdir;          /*!< Local path to the repo */
};

/** \ingroup types
 * Pointer to ::_lr_YumRepo
 */
typedef struct _lr_YumRepo *lr_YumRepo;

/** \ingroup types
 * Progress callback prototype
 */
typedef int (*lr_ProgressCb)(void *clientp,
                              double total_to_download,
                              double now_downloaded);

#ifdef __cplusplus
}
#endif

#endif
