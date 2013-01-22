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

#ifndef LR_REPOMD_H
#define LR_REPOMD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

/** \defgroup repomd        Repomd parser
 *  \addtogroup repomd
 *  @{
 */

/** Yum repomd distro tag. */
struct _lr_YumDistroTag {
    char *cpeid;    /*!< Tag cpeid value or NULL. */
    char *value;    /*!< Tag value. */
};

/** Pointer to ::_lr_YumDistroTag. */
typedef struct _lr_YumDistroTag *lr_YumDistroTag;

/** Yum repomd record. */
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

/** Pointer to ::_lr_YumRepoMdRecord */
typedef struct _lr_YumRepoMdRecord *lr_YumRepoMdRecord;

/** Yum repomd.xml. */
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

/** Pointer to ::_lr_YumRepoMd */
typedef struct _lr_YumRepoMd *lr_YumRepoMd;

/** Create new empty repomd object.
 * @return              New repomd object.
 */
lr_YumRepoMd lr_yum_repomd_init();

/** Free all repomd content.
 * @param repomd        Repomd object.
 */
void lr_yum_repomd_clear(lr_YumRepoMd repomd);

/** Free repomd content and repomd object itself.
 * @param repomd        Repomd object.
 */
void lr_yum_repomd_free(lr_YumRepoMd repomd);

/** Parse repomd.xml file.
 * @param repomd        Empty repomd object.
 * @param fd            File descriptor.
 * @return              Librepo return code ::lr_Rc.
 */
int lr_yum_repomd_parse_file(lr_YumRepoMd repomd, int fd);

/** Get repomd record from the repomd object.
 * @param repomd        Repomd record.
 * @param type          Type of record e.g. "primary", "filelists", ...
 * @return              Record of desired type or NULL.
 */
lr_YumRepoMdRecord lr_yum_repomd_get_record(lr_YumRepoMd repomd,
                                            const char *type);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
