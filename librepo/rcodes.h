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

#ifndef __LR_RETURN_CODES_H__
#define __LR_RETURN_CODES_H__

#include <glib.h>

G_BEGIN_DECLS

/** \defgroup   rcodes      Error/Return codes
 *  \addtogroup rcodes
 *  @{
 */

/** Librepo return/error codes
 */
typedef enum {
    LRE_OK, /*!<
        (0) everything is ok */
    LRE_BADFUNCARG, /*!<
        (1) bad function argument */
    LRE_BADOPTARG, /*!<
        (2) bad argument of the option */
    LRE_UNKNOWNOPT, /*!<
        (3) library doesn't know the option */
    LRE_CURLSETOPT, /*!<
        (4) cURL doesn't know the option. Too old curl version? */
    LRE_ALREADYUSEDRESULT, /*!<
        (5) LrResult object is not clean */
    LRE_INCOMPLETERESULT, /*!<
        (6) LrResult doesn't contain all what is needed */
    LRE_CURLDUP, /*!<
        (7) cannot duplicate curl handle */
    LRE_CURL, /*!<
        (8) cURL error */
    LRE_CURLM, /*!<
        (9) cULR multi handle error */
    LRE_BADSTATUS, /*!<
        (10) HTTP or FTP returned status code which do not represent success
        (file doesn't exists, etc.) */
    LRE_TEMPORARYERR, /*!<
        (11) some error that should be temporary and next try could work
        (HTTP status codes 500, 502-504, operation timeout, ...) */
    LRE_NOTLOCAL, /*!<
        (12) URL is not a local address */
    LRE_CANNOTCREATEDIR, /*!<
        (13) cannot create a directory in output dir (already exists?) */
    LRE_IO, /*!<
        (14) input output error */
    LRE_MLBAD, /*!<
        (15) bad mirrorlist/metalink file (metalink doesn't contain needed
        file, mirrorlist doesn't contain urls, ..) */
    LRE_MLXML, /*!<
        (16) metalink XML parse error */
    LRE_BADCHECKSUM, /*!<
        (17) bad checksum */
    LRE_REPOMDXML, /*!<
        (18) repomd XML parse error */
    LRE_NOURL, /*!<
        (19) usable URL not found */
    LRE_CANNOTCREATETMP, /*!<
        (20) cannot create tmp directory */
    LRE_UNKNOWNCHECKSUM, /*!<
        (21) unknown type of checksum is needed for verification */
    LRE_BADURL, /*!<
        (22) bad URL specified */
    LRE_GPGNOTSUPPORTED, /*!<
        (23) OpenPGP protocol is not supported */
    LRE_GPGERROR, /*!<
        (24) GPGME related error */
    LRE_BADGPG, /*!<
        (25) Bad GPG signature */
    LRE_INCOMPLETEREPO, /*!<
        (26) Repository metadata are not complete */
    LRE_INTERRUPTED, /*!<
        (27) Download was interrupted by signal.
        Only if LRO_INTERRUPTIBLE option is enabled. */
    LRE_SIGACTION, /*!<
        (28) sigaction error */
    LRE_ALREADYDOWNLOADED, /*!<
        (29) File already exists and checksum is ok.*/
    LRE_UNFINISHED, /*!<
        (30) The download wasn't or cannot be finished. */
    LRE_SELECT, /*!<
        (31) select() call failed. */
    LRE_OPENSSL, /*!<
        (32) OpenSSL library related error. */
    LRE_MEMORY, /*!<
        (33) Cannot allocate more memory  */
    LRE_XMLPARSER, /*!<
        (34) XML parser error */
    LRE_CBINTERRUPTED, /*!<
        (35) Interrupted by user cb */
    LRE_REPOMD, /*!<
        (36) Error with repomd (bad content, missing expected values, ...) */
    LRE_VALUE, /*!<
        (37) Bad value (e.g. we are expecting bandwidth defined like '1024',
        '1k', etc., but we got something like 'asdf', '1024S', etc.) */
    LRE_NOTSET, /*!<
        (38) Requested option/value is not set. Used for example in
        lr_yum_repoconf_getinfo() */
    LRE_FILE, /*!<
        (39) File operation error (operation not permitted, filename too long,
        no memory available, bad file descriptor, ...) */
    LRE_KEYFILE, /*!<
        (40) Key file error (unknown encoding, ill-formed, file not found,
        key/group not found, ...) */
    LRE_ZCK, /*!<
        (41) Zchunk error (error reading zchunk file, ...) */
    LRE_TRANSCODE, /*!<
        (42) Transcode error (env empty, ...) */
    LRE_UNKNOWNERROR, /*!<
        (xx) unknown error - sentinel of error codes enum */
} LrRc; /*!< Return codes */

/** Converts LrRc return code to error string.
 * @param rc        LrRc code
 * @return          Error string
 */
const char *lr_strerror(int rc);

/** Error domains for GError */
#define LR_CHECKSUM_ERROR           lr_checksum_error_quark()
#define LR_DOWNLOADER_ERROR         lr_downloader_error_quark()
#define LR_FASTESTMIRROR_ERROR      lr_fastestmirror_error_quark()
#define LR_GPG_ERROR                lr_gpg_error_quark()
#define LR_HANDLE_ERROR             lr_handle_error_quark()
#define LR_METALINK_ERROR           lr_metalink_error_quark()
#define LR_MIRRORLIST_ERROR         lr_mirrorlist_error_quark()
#define LR_PACKAGE_DOWNLOADER_ERROR lr_package_downloader_error_quark()
#define LR_REPOCONF_ERROR           lr_repoconf_error_quark()
#define LR_REPOMD_ERROR             lr_repomd_error_quark()
#define LR_REPOUTIL_YUM_ERROR       lr_repoutil_yum_error_quark()
#define LR_RESULT_ERROR             lr_result_error_quark()
#define LR_XML_PARSER_ERROR         lr_xml_parser_error_quark()
#define LR_YUM_ERROR                lr_yum_error_quark()

GQuark lr_checksum_error_quark(void);
GQuark lr_downloader_error_quark(void);
GQuark lr_fastestmirror_error_quark(void);
GQuark lr_gpg_error_quark(void);
GQuark lr_handle_error_quark(void);
GQuark lr_metalink_error_quark(void);
GQuark lr_mirrorlist_error_quark(void);
GQuark lr_package_downloader_error_quark(void);
GQuark lr_repoconf_error_quark(void);
GQuark lr_repomd_error_quark(void);
GQuark lr_repoutil_yum_error_quark(void);
GQuark lr_result_error_quark(void);
GQuark lr_xml_parser_error_quark(void);
GQuark lr_yum_error_quark(void);

/** @} */

G_END_DECLS

#endif
