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

#include "rcodes.h"

const char *
lr_strerror(int rc)
{
    switch (rc) {
    case LRE_OK:
        return "No error";
    case LRE_BADFUNCARG:
        return "Bad function argument(s)";
    case LRE_BADOPTARG:
        return "Bad argument of a handle option";
    case LRE_UNKNOWNOPT:
        return "Unknown option";
    case LRE_CURLSETOPT:
        return "curl_*_setopt() failed (too old curl version?)";
    case LRE_ALREADYUSEDRESULT:
        return "LrResult object is not clean";
    case LRE_INCOMPLETERESULT:
        return "LrResult object doesn't contain all needed info";
    case LRE_CURLDUP:
        return "Cannot duplicate curl handle";
    case LRE_CURL:
        return "An Curl handle error";
    case LRE_CURLM:
        return "An Curl multi handle error";
    case LRE_BADSTATUS:
        return "Error HTTP/FTP status code";
    case LRE_TEMPORARYERR:
        return "Temporary error (operation timeout, ...), next try could work";
    case LRE_NOTLOCAL:
        return "Repository URL is not a local path";
    case LRE_CANNOTCREATEDIR:
        return "Cannot create output directory";
    case LRE_IO:
        return "Input/Output error";
    case LRE_MLBAD:
        return "Bad mirrorlist or metalink file";
    case LRE_MLXML:
        return "Metalink XML parse error";
    case LRE_BADCHECKSUM:
        return "Bad checksum";
    case LRE_REPOMDXML:
        return "repomd.xml XML parse error";
    case LRE_NOURL:
        return "Usable URL not found";
    case LRE_CANNOTCREATETMP:
        return "Cannot create temp directory";
    case LRE_UNKNOWNCHECKSUM:
        return "Unknown type of checksum is needed to verify one or more file(s)";
    case LRE_BADURL:
        return "Bad URL specified";
    case LRE_GPGNOTSUPPORTED:
        return "GPGME protocol is not supported";
    case LRE_GPGERROR:
        return "Error while GPG check";
    case LRE_BADGPG:
        return "Bad GPG signature";
    case LRE_INCOMPLETEREPO:
        return "Repository metadata are not complete";
    case LRE_INTERRUPTED:
        return "Interrupted by SIGINT";
    case LRE_SIGACTION:
        return "Cannot set own signal handler - sigaction system call failed";
    case LRE_ALREADYDOWNLOADED:
        return "File already exists and checksum is ok";
    case LRE_UNFINISHED:
        return "Download wasn't or cannot be finished";
    case LRE_SELECT:
        return "select() call failed";
    case LRE_OPENSSL:
        return "OpenSSL library related error";
    case LRE_MEMORY:
        return "Cannot allocate more memory";
    case LRE_XMLPARSER:
        return "XML parser error";
    case LRE_CBINTERRUPTED:
        return "Interrupted by user cb";
    case LRE_REPOMD:
        return "Error in repomd.xml";
    case LRE_VALUE:
        return "Bad value (no value, unknown unit, etc.)";
    case LRE_NOTSET:
        return "Requested option/value is not set";
    case LRE_FILE:
        return "File operation error";
    case LRE_KEYFILE:
        return "Key file parsing error";
    }

    return "Unknown error";
}

GQuark
lr_checksum_error_quark(void)
{
    return g_quark_from_static_string("lr_checksum_error");
}

GQuark
lr_downloader_error_quark(void)
{
    return g_quark_from_static_string("lr_downloader_error");
}

GQuark
lr_fastestmirror_error_quark(void)
{
    return g_quark_from_static_string("lr_fastestmirror_error");
}

GQuark
lr_gpg_error_quark(void)
{
    return g_quark_from_static_string("lr_gpg_error");
}

GQuark
lr_handle_error_quark(void)
{
    return g_quark_from_static_string("lr_handle_error");
}

GQuark
lr_metalink_error_quark(void)
{
    return g_quark_from_static_string("lr_metalink_error");
}

GQuark
lr_mirrorlist_error_quark(void)
{
    return g_quark_from_static_string("lr_mirrorlist_error");
}

GQuark
lr_package_downloader_error_quark(void)
{
    return g_quark_from_static_string("lr_package_downloader_error");
}

GQuark
lr_xml_parser_error_quark(void)
{
    return g_quark_from_static_string("lr_xml_parser_error");
}

GQuark
lr_repoconf_error_quark(void)
{
    return g_quark_from_static_string("lr_repoconf_error");
}

GQuark
lr_repomd_error_quark(void)
{
    return g_quark_from_static_string("lr_repomd_error");
}

GQuark
lr_repoutil_yum_error_quark(void)
{
    return g_quark_from_static_string("lr_repoutil_yum_error");
}

GQuark
lr_result_error_quark(void)
{
    return g_quark_from_static_string("lr_result_error");
}

GQuark
lr_yum_error_quark(void)
{
    return g_quark_from_static_string("lr_yum_error");
}
