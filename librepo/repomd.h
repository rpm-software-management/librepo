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

#ifndef __LR_REPOMD_H__
#define __LR_REPOMD_H__

#include <glib.h>

#include "xmlparser.h"
#include "types.h"

G_BEGIN_DECLS

/** \defgroup   repomd        Repomd (repomd.xml) parser
 *  \addtogroup repomd
 *  @{
 */

/** Yum repomd distro tag. */
typedef struct {
    char *cpeid;    /*!< Tag cpeid value or NULL. */
    char *tag;      /*!< Tag value. */
} LrYumDistroTag;

/** Yum repomd record. */
typedef struct {
    char *type;                 /*!< Type of record (e.g. "primary") */
    char *location_href;        /*!< Location href attribute */
    char *location_base;        /*!< Location base attribute */
    char *checksum;             /*!< Checksum value */
    char *checksum_type;        /*!< Type of checksum */
    char *checksum_open;        /*!< Checksum of uncompressed file */
    char *checksum_open_type;   /*!< Type of checksum of uncompressed file */
    char *header_checksum;      /*!< Checksum of zchunk header */
    char *header_checksum_type; /*!< Type of checksum of zchunk header */
    gint64 timestamp;           /*!< File timestamp */
    gint64 size;                /*!< File size */
    gint64 size_open;           /*!< Size of uncompressed file */
    gint64 size_header;         /*!< Size of zchunk header */
    int db_version;             /*!< Version of database */

    GStringChunk *chunk;        /*!< String chunk */
} LrYumRepoMdRecord;

/** Yum repomd.xml. */
typedef struct {
    char *revision;         /*!< Revision string*/
    char *repoid;           /*!< RepoId */
    char *repoid_type;      /*!< RepoId type ("sha256", ...) */
    GSList *repo_tags;      /*!< List of strings */
    GSList *content_tags;   /*!< List of strings */
    GSList *distro_tags;    /*!< List of LrYumDistroTag* */
    GSList *records;        /*!< List with LrYumRepoMdRecords */

    GStringChunk *chunk;    /*!< String chunk for repomd strings
                                 (Note: LrYumRepomdRecord strings are stored
                                 in LrYumRepomdRecord->chunk) */
} LrYumRepoMd;

/** Create new empty repomd object.
 * @return              New repomd object.
 */
LrYumRepoMd *
lr_yum_repomd_init(void);

/** Free repomd content and repomd object itself.
 * @param repomd        Repomd object.
 */
void
lr_yum_repomd_free(LrYumRepoMd *repomd);

/** Parse repomd.xml file.
 * @param repomd            Empty repomd object.
 * @param fd                File descriptor.
 * @param warningcb         Callback for warnings
 * @param warningcb_data    Warning callback user data
 * @param err               GError **
 * @return                  TRUE if everything is ok, FALSE if err is set.
 */
gboolean
lr_yum_repomd_parse_file(LrYumRepoMd *repomd,
                         int fd,
                         LrXmlParserWarningCb warningcb,
                         void *warningcb_data,
                         GError **err);

/** Get repomd record from the repomd object.
 * @param repomd        Repomd record.
 * @param type          Type of record e.g. "primary", "filelists", ...
 * @return              Record of desired type or NULL.
 */
LrYumRepoMdRecord *
lr_yum_repomd_get_record(LrYumRepoMd *repomd,
                         const char *type);

/** Get the highest timestamp from repomd records.
 * @param repomd        Repomd record.
 * @param err           GError **
 * @return              The highest timestamp from repomd records
 *                      or -1 if no records available.
 */
gint64
lr_yum_repomd_get_highest_timestamp(LrYumRepoMd *repomd, GError **err);

/** @} */

G_END_DECLS

#endif
