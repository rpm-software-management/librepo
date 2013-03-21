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
        return "lr_Result object is not clean";
    case LRE_INCOMPLETERESULT:
        return "lr_Result object doesn't contain all needed info";
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
    case LRE_INCOMPLETEREPO:
        return "Repository metadata are not complete";
    case LRE_INTERRUPTED:
        return "Interrupted by SIGINT";
    case LRE_SIGACTION:
        return "Cannot set own signal handler - sigaction system call failed";
    case LRE_BADGPG:
        return "Bad GPG signature";
    }

    return "Unknown error";
}
