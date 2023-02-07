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

#ifndef __LR_UTIL_H__
#define __LR_UTIL_H__

#include <glib.h>
#include <stdlib.h>
#include <stdarg.h>
#include <curl/curl.h>

#ifdef WITH_ZCHUNK
#include <zck.h>
#endif /* WITH_ZCHUNK */

#include <librepo/checksum.h>
#include <librepo/xmlparser.h>
#include <librepo/downloadtarget.h>

G_BEGIN_DECLS

/** \defgroup   util    Utility functions and macros
 *  \addtogroup util
 *  @{
 */

/** Macro for curl version check.
 * @param major     Major version
 * @param minor     Minor version
 * @param patch     Patch version
 * @return          True if current curl version is higher or equal
 */
#define LR_CURL_VERSION_CHECK(major,minor,patch)    \
    (LIBCURL_VERSION_MAJOR > (major) || \
     (LIBCURL_VERSION_MAJOR == (major) && LIBCURL_VERSION_MINOR > (minor)) || \
     (LIBCURL_VERSION_MAJOR == (major) && LIBCURL_VERSION_MINOR == (minor) && \
      LIBCURL_VERSION_PATCH >= (patch)))

/** Initialize librepo library.
 * This is called automatically to initialize librepo.
 * You normally don't have to call this function manually.
 */
void lr_global_init(void);

/** Clean up librepo library.
void lr_global_cleanup();
*/

/** Log a debug message with Librepo version and current time.
 */
void lr_log_librepo_summary(void);

/** Print "Out of memory" message to stderr and abort program execution.
 * This function is used when malloc call fails.
 */
void lr_out_of_memory(void);

/** Allocate len bytes of memory.
 * @param len           Number of bytes to be allocated.
 * @return              Pointer to allocated memory.
 */
void *lr_malloc(size_t len);

/** Allocate len bytes of memory. The allocated memory is set to zero.
 * @param len           Number of bytes to be allocated.
 * @return              Pointer to allocated memory.
 */
void *lr_malloc0(size_t len);

/** Change size of block memory pointed by ptr to the new len.
 * @param ptr           Pointer to block of memory or NULL.
 * @param len           New len of the block.
 * @return              New pointer to the reallocated memory.
 */
void *lr_realloc(void *ptr, size_t len);

/** Free the memory block.
 * @param mem           Pointer to block of memory.
 */
void lr_free(void *mem);

/** Create temporary librepo file in /tmp directory.
 * @return              File descriptor.
 */
int lr_gettmpfile(void);

/** Create temporary directory in /tmp directory.
 * @return              Path to directory.
 */
char *lr_gettmpdir(void);

/** Concatenate all of given part of path.
 * If last chunk is "" then separator will be appended to the result.
 * @param str           First part of the path.
 * @param ...           NULL terminated list of strings.
 * @return              Concatenated path.
 */
char *lr_pathconcat(const char *str, ...) G_GNUC_NULL_TERMINATED;

/** Recursively remove directory.
 * @param path          Path to the directory.
 * @return              0 on succes, -1 on error.
 */
int lr_remove_dir(const char *path);

/** Copy content from source file descriptor to the dest file descriptor.
 * @param source        Source opened file descriptor
 * @param dest          Destination opened file descriptor
 * @return              0 on succes, -1 on error
 */
int lr_copy_content(int source, int dest);

/** If protocol is specified ("http://foo") return copy of path.
 * If path is absolute ("/foo/bar/") return path with "file://" prefix.
 * If path is relative ("bar/") return absolute path with "file://" prefix.
 * @param               path
 * @return              url with protocol
 */
char *lr_prepend_url_protocol(const char *path);

/** Same as g_string_chunk_insert, but allows NULL as string.
 * If the string is NULL, then returns NULL and do nothing.
 * @param chunk         String chunk
 * @param string        String or NULL
 * @return              a pointer to the copy of string within the chunk
 */
gchar *
lr_string_chunk_insert(GStringChunk *chunk, const gchar *string);

/** Warning callback to print warnings via GLib logger
 * For more info take a look at ::LrXmlParserWarningCb
 */
int
lr_xml_parser_warning_logger(LrXmlParserWarningType type G_GNUC_UNUSED,
                             char *msg,
                             void *cbdata,
                             GError **err G_GNUC_UNUSED) G_GNUC_UNUSED;


/** From the GSList of pointers to LrMetalinkHash objects, select the
 * strongest one which librepo could calculate.
 * @param list      List of LrMetalinkHash*
 * @param type      Variable to store checksum type.
 * @param value     Variable to store pointer to value (pointer to original
 *                  value from the list, NOT A COPY).
 * @return          TRUE if usable checksum found, FALSE otherwise
 */
gboolean
lr_best_checksum(GSList *list, LrChecksumType *type, gchar **value);

/** Return malloced string with host part of url (protocol prefix + hostname)
 * @param url       URL
 * @return          Malloced url without path, just protocol and hostname
 */
gchar *
lr_url_without_path(const char *url);

/** Create a copy of NULL-terminated array of strings.
 * All strings in the copy are malloced - returned array
 * must be freed by g_strfreev()
 * @param array     NULL-terminated array of strings or NULL
 * @return          Copy of input array or NULL if input was NULL
 */
gchar **
lr_strv_dup(gchar **array);


/** Check if string looks like a local path.
 * (This function doesn't do realpath resolving and existence checking)
 * If a path is empty, NULL or has protocol other then file:// specified
 * then the path is considered as remote, otherwise TRUE is returned.
 * @param path      A string path
 * @return          TRUE if path string looks like a local path
 */
gboolean
lr_is_local_path(const gchar *path);

/** Re-implementatio of g_key_file_save_to_file,
 * because the function is available since 2.40 but we need
 * to support older glib
 * @param key_file  key file
 * @param filename  filename
 * @param error     GError **
 * @return          TRUE if successful, FALSE otherwise
 */
gboolean
lr_key_file_save_to_file(GKeyFile *key_file,
                         const gchar *filename,
                         GError **error);

#ifdef WITH_ZCHUNK
/** Get LrChecksumType that corresponds to zck_hash
 * @param zck_checksum_type  zck_hash value
 * @return checksum_type     corresponding LrChecksumType value
 *
 * Return value will be LR_CHECKSUM_UNKNOWN if zck_checksum_type isn't available
 * as a LrChecksumType
 */
LrChecksumType
lr_checksum_from_zck_hash(zck_hash zck_checksum_type);

/** Get zck_hash that corresponds to LrChecksumType
 * @param checksum_type       LrChecksumType value
 * @return zck_checksum_type  corresponding zck_hash value
 *
 * Return value will be ZCK_HASH_UNKNOWN if checksum_type isn't available as a
 * zck_hash
 */
zck_hash
lr_zck_hash_from_lr_checksum(LrChecksumType checksum_type);

/** Base function for initializing zchunk file for reading while verifying
 *  header checksum
 * @param checksum            header checksum
 * @param checksum_type       header checksum type
 * @param zck_header_size     header size
 * @param fd                  file descriptor to read from
 * @return zck                zchunk context opened for reading
 *
 * Return value will be NULL if the header checksum doesn't match the provided
 * checksum, is invalid, or if the header can't be read
 */
zckCtx *
lr_zck_init_read_base(const char *checksum, LrChecksumType checksum_type,
                      gint64 zck_header_size, int fd, GError **err);

/** Base function for checking whether zchunk file has valid header checksum
 * @param checksum            header checksum
 * @param checksum_type       header checksum type
 * @param zck_header_size     header size
 * @param fd                  file descriptor to read from
 * @return zck                zchunk context opened for reading
 *
 * Return value will be FALSE if the header checksum doesn't match the provided
 * checksum, is invalid, or if the header can't be read
 */
gboolean
lr_zck_valid_header_base(const char *checksum, LrChecksumType checksum_type,
                         gint64 zck_header_size, int fd, GError **err);

/** Initialize zchunk file for reading while verifying header checksum
 * @param checksum            target containing checksum information
 * @param fd                  file descriptor to read from
 * @return zck                zchunk context opened for reading
 *
 * Return value will be NULL if the header checksum doesn't match the provided
 * checksum, is invalid, or if the header can't be read
 */
zckCtx *
lr_zck_init_read(LrDownloadTarget *target, char *filename, int fd, GError **err);

/** Check whether zchunk file has valid header checksum
 * @param checksum            target containing checksum information
 * @param fd                  file descriptor to read from
 * @return zck                zchunk context opened for reading
 *
 * Return value will be FALSE if the header checksum doesn't match the provided
 * checksum, is invalid, or if the header can't be read
 */
gboolean
lr_zck_valid_header(LrDownloadTarget *target, char *filename, int fd, GError **err);

/** Recursively get list of all files in path that end with extension
 * @param path                path to search
 * @param extension           return files with this extension (including .)
 * @param err                 GError **
 * @return filelist           GSList* of files that have extension
 *
 * Return value will be NULL if no files match or there's an error.
 * err will be set if there's an error
 */
#endif /* WITH_ZCHUNK */

GSList *
lr_get_recursive_files(char *path, char *extension, GError **err);

/** @} */

G_END_DECLS

#endif
