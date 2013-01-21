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

#ifdef __cplusplus
extern "C" {
#endif

/** Single checksum for the metalink target file. */
struct _lr_MetalinkHash {
    char *type;     /*!< Type of checksum (e.g. "md5", "sha1", "sha256", ... */
    char *value;    /*!< Value of the checksum */
};

/** Pointer to ::_lr_MetalinkHash */
typedef struct _lr_MetalinkHash * lr_MetalinkHash;

/** Single metalink URL */
struct _lr_MetalinkUrl {
    char *protocol;     /*!< Mirror protocol "http", "ftp", "rsync", ... */
    char *type;         /*!< Mirror type "http", "ftp", "rsync", ... */
    char *location;     /*!< ISO 3166-1 alpha-2 code ("US", "CZ", ..) */
    int preference;     /*!< Integer number 1-100, higher is better */
    char *url;          /*!< URL to the target file */
};

/** Pointer to ::_lr_MetalinkUrl */
typedef struct _lr_MetalinkUrl * lr_MetalinkUrl;

/** Metalink */
struct _lr_Metalink {
    char *filename;             /*!< Filename */
    long timestamp;             /*!< File timestamp */
    long size;                  /*!< File size */
    lr_MetalinkHash *hashes;    /*!< File checksum list (could be NULL) */
    lr_MetalinkUrl *urls;       /*!< Mirrors url list (could be NULL) */

    int noh;                    /*!< Number of hashes */
    int nou;                    /*!< Number of urls */
    int loh;                    /*!< Length of hashes list (allocated len) */
    int lou;                    /*!< Length urls list (allocated len) */
};

/** Pointer to ::_lr_Metalink */
typedef struct _lr_Metalink * lr_Metalink;

/**
 * Create new empty metalink object.
 * @return              New metalink object.
 */
lr_Metalink lr_metalink_init();

/**
 * Parse metalink file.
 * @param metalink      Metalink object.
 * @param fd            File descriptor.
 * @return              Librepo return code ::lr_Rc.
 */
int lr_metalink_parse_file(lr_Metalink metalink, int fd);

/**
 * Free metalink object and all its content.
 * @param metalink      Metalink object.
 */
void lr_metalink_free(lr_Metalink metalink);

#ifdef __cplusplus
}
#endif

#endif
