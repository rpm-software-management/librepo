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

#ifndef __LR_METALINK_H__
#define __LR_METALINK_H__

#include <glib.h>
#include <librepo/xmlparser.h>

G_BEGIN_DECLS

/** \defgroup   metalink  Metalink parser
 *  \addtogroup metalink
 *  @{
 */

/** Single checksum for the metalink target file. */
typedef struct {
    char *type;     /*!< Type of checksum (e.g. "md5", "sha1", "sha256", ... */
    char *value;    /*!< Value of the checksum */
} LrMetalinkHash;

/** Single metalink URL */
typedef struct {
    char *protocol;     /*!< Mirror protocol "http", "ftp", "rsync", ... */
    char *type;         /*!< Mirror type "http", "ftp", "rsync", ... */
    char *location;     /*!< ISO 3166-1 alpha-2 code ("US", "CZ", ..) */
    int preference;     /*!< Integer number 1-100, higher is better */
    char *url;          /*!< URL to the target file */
} LrMetalinkUrl;

/** Alternate */
typedef struct {
    gint64 timestamp; /*!< File timestamp */
    gint64 size;      /*!< File size */
    GSList *hashes;   /*!< List of pointers to LrMetalinkHashes (could be NULL) */
} LrMetalinkAlternate;

/** Metalink */
typedef struct {
    char *filename;   /*!< Filename */
    gint64 timestamp; /*!< Repo timestamp (the highest ts in the repomd.xml) */
    gint64 size;      /*!< File size */
    GSList *hashes;   /*!< List of pointers to LrMetalinkHashes (could be NULL) */
    GSList *urls;     /*!< List of pointers to LrMetalinkUrls (could be NULL) */
    GSList *alternates; /*!< List of pointers to LrMetalinkAlternates (could be NULL) */
} LrMetalink;

/** Create new empty metalink object.
 * @return              New metalink object.
 */
LrMetalink *
lr_metalink_init(void);

/** Parse metalink file.
 * @param metalink          Metalink object.
 * @param fd                File descriptor.
 * @param filename          File to look for in metalink file.
 * @param warningcb         ::LrXmlParserWarningCb function or NULL
 * @param warningcb_data    Warning callback data or NULL
 * @param err               GError **
 * @return                  TRUE if everything is ok, FALSE if err is set.
 */
gboolean
lr_metalink_parse_file(LrMetalink *metalink,
                       int fd,
                       const char *filename,
                       LrXmlParserWarningCb warningcb,
                       void *warningcb_data,
                       GError **err);

/** Free metalink object and all its content.
 * @param metalink      Metalink object.
 */
void
lr_metalink_free(LrMetalink *metalink);

/** @} */

G_END_DECLS

#endif
