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

#ifndef __LR_REPOCONF_H__
#define __LR_REPOCONF_H__

#include <glib.h>
#include "types.h"

G_BEGIN_DECLS

/** \defgroup   repoconf    *.repo file
 *  \addtogroup repoconf
 *  @{
 */

#define LR_YUMREPOCONF_BANDWIDTH_DEFAULT        G_GUINT64_CONSTANT(0)
#define LR_YUMREPOCONF_IP_RESOLVE_DEFAULT       LR_IPRESOLVE_WHATEVER
#define LR_YUMREPOCONF_METADATA_EXPIRE_DEFAULT  G_GINT64_CONSTANT(172800) // 60*60*48
#define LR_YUMREPOCONF_COST_DEFAULT             1000
#define LR_YUMREPOCONF_PRIORITY_DEFAULT         99

/** Yum repos */
typedef struct _LrYumRepoConfs LrYumRepoConfs;

/** Yum repo config */
typedef struct {
    gchar          *_source;         // path

    gchar          *id;             // id
    gchar          *name;
    gboolean        enabled;        // bool
    gchar          **baseurl;       // url list
    gchar          *mirrorlist;     // url
    gchar          *metalink;       // url

    gchar          *mediaid;
    gchar          **gpgkey;         // url list
    gchar          **gpgcakey;       // url list
    gchar          **exclude;        // list
    gchar          **include;        // list

    gboolean        fastestmirror;  /*!< Fastest mirror determination */
    gchar          *proxy;          // url
    gchar          *proxy_username;
    gchar          *proxy_password;
    gchar          *username;
    gchar          *password;

    gboolean        gpgcheck;       /*!< GPG check for packages */
    gboolean        repo_gpgcheck;  /*!< GPG check for repodata */
    gboolean        enablegroups;   /*!< Use groups */

    guint64         bandwidth;      /*!< Bandwidth - Number of bytes */
    gchar          *throttle;       /*!< Throttle string */
    LrIpResolveType ip_resolve;     // caselessselection

    gint64          metadata_expire;// seconds
    gint            cost;           // int
    gint            priority;       // int

    gchar          *sslcacert;
    gboolean        sslverify;      // bool
    gchar          *sslclientcert;
    gchar          *sslclientkey;

    gchar          **deltarepobaseurl; // url list
} LrYumRepoConf;

LrYumRepoConfs *
lr_yum_repoconfs_init(void);

void
lr_yum_repoconfs_free(LrYumRepoConfs *repos);

GSList *
lr_yum_repoconfs_get_list(LrYumRepoConfs *repos, GError **err);

gboolean
lr_yum_repoconfs_parse(LrYumRepoConfs *repos,
                       const char *filename,
                       GError **err);

gboolean
lr_yum_repoconfs_load_dir(LrYumRepoConfs *repos,
                          const char *path,
                          GError **err);

LrYumRepoConf *
lr_yum_repoconf_copy(LrYumRepoConf *repoconf);

/** @} */

G_END_DECLS

#endif
