/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2014  Tomas Mlcoch
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

#ifndef __LR_REPOCONF_INTERNAL_H__
#define __LR_REPOCONF_INTERNAL_H__

#include <glib.h>
#include "types.h"
#include "repoconf.h"

G_BEGIN_DECLS

/** Single repo file */
struct _LrYumRepoFile {
    gchar          *path;           /*!< Path to the file */
    GKeyFile       *keyfile;        /*!< Parsed content */
    gboolean        modified;       /*!< Unsaved modifications? */
};

typedef struct _LrYumRepoFile LrYumRepoFile;

/** Yum repo config */
struct _LrYumRepoConf {
/*    gchar          *_source;        [>!< Local path to .repo file <]*/

    //gchar          *id;             [>!< ID (short name) of the repo <]
    //gchar          *name;           [>!< Pretty name of the repo <]
    //gboolean        enabled;        [>!< Is repo enabled? <]
    //gchar          **baseurl;       [>!< List of base URLs <]
    //gchar          *mirrorlist;     [>!< Mirrorlist URL <]
    //gchar          *metalink;       [>!< Metalink URL <]

    //gchar          *mediaid;        [>!< Media ID <]
    //gchar          **gpgkey;        [>!< URL of GPG key <]
    //gchar          **gpgcakey;      [>!< GPG CA key <]
    //gchar          **exclude;       [>!< List of exluded packages <]
    //gchar          **include;       [>!< List of included packages <]

    //gboolean        fastestmirror;  [>!< Fastest mirror determination <]
    //gchar          *proxy;          [>!< Proxy addres <]
    //gchar          *proxy_username; [>!< Proxy username <]
    //gchar          *proxy_password; [>!< Proxy password <]
    //gchar          *username;       [>!< Username <]
    //gchar          *password;       [>!< Password <]

    //gboolean        gpgcheck;       [>!< GPG check for packages <]
    //gboolean        repo_gpgcheck;  [>!< GPG check for repodata <]
    //gboolean        enablegroups;   [>!< Use groups <]

    //guint64         bandwidth;      [>!< Bandwidth - Number of bytes <]
    //gchar          *throttle;       [>!< Throttle string <]
    //LrIpResolveType ip_resolve;     [>!< Ip resolve type <]

    //gint64          metadata_expire;[>!< Interval in secs for metadata expiration <]
    //gint            cost;           [>!< Repo cost <]
    //gint            priority;       [>!< Repo priority <]

    //gchar          *sslcacert;      [>!< SSL Certification authority cert <]
    //gboolean        sslverify;      [>!< SSL verification <]
    //gchar          *sslclientcert;  [>!< SSL Client certificate <]
    //gchar          *sslclientkey;   [>!< SSL Client key <]

    //gchar          **deltarepobaseurl;[>!< Deltarepo mirror URLs <]

    // -------------------------------------------------------

    LrYumRepoFile  *file;           /*!< Config file */
    gchar          *id;             /*!< Repo ID (group name in key file) */
};

/** List of repoconfigs */
struct _LrYumRepoConfs {
    GSList         *repos;          /*!< List of LrYumRepoConf */
    GSList         *files;          /*!< List of LrYumRepoFile */
};

G_END_DECLS

#endif
