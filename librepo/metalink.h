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

#ifndef LR_METALINK_H
#define LR_METALINK_H

#include <glib.h>

G_BEGIN_DECLS

/** \defgoupt   metalink  Metalink parser
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

/** Metalink */
typedef struct {
    char *filename; /*!< Filename */
    long timestamp; /*!< File timestamp */
    long size;      /*!< File size */
    GSList *hashes; /*!< List of pointers to LrMetalinkHashes (could be NULL) */
    GSList *urls;   /*!< List of pointers to LrMetalinkUrls (could be NULL) */
} LrMetalink;

/** Create new empty metalink object.
 * @return              New metalink object.
 */
LrMetalink *lr_metalink_init();

/** Parse metalink file.
 * @param metalink      Metalink object.
 * @param fd            File descriptor.
 * @param filename      File to look for in metalink file.
 * @param err           GError **
 * @return              Librepo return code ::LrRc.
 */
int lr_metalink_parse_file(LrMetalink *metalink,
                           int fd,
                           const char *filename,
                           GError **err);

/** Free metalink object and all its content.
 * @param metalink      Metalink object.
 */
void lr_metalink_free(LrMetalink *metalink);

/** @} */

G_END_DECLS

#endif
