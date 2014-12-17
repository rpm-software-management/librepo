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

/** List of yum repo configurations */
typedef struct _LrYumRepoConfs LrYumRepoConfs;

/** Yum repo config */
typedef struct {
    gchar          *_source;        /*!< Local path to .repo file */

    gchar          *id;             /*!< ID (short name) of the repo */
    gchar          *name;           /*!< Pretty name of the repo */
    gboolean        enabled;        /*!< Is repo enabled? */
    gchar          **baseurl;       /*!< List of base URLs */
    gchar          *mirrorlist;     /*!< Mirrorlist URL */
    gchar          *metalink;       /*!< Metalink URL */

    gchar          *mediaid;        /*!< Media ID */
    gchar          **gpgkey;        /*!< URL of GPG key */
    gchar          **gpgcakey;      /*!< GPG CA key */
    gchar          **exclude;       /*!< List of exluded packages */
    gchar          **include;       /*!< List of included packages */

    gboolean        fastestmirror;  /*!< Fastest mirror determination */
    gchar          *proxy;          /*!< Proxy addres */
    gchar          *proxy_username; /*!< Proxy username */
    gchar          *proxy_password; /*!< Proxy password */
    gchar          *username;       /*!< Username */
    gchar          *password;       /*!< Password */

    gboolean        gpgcheck;       /*!< GPG check for packages */
    gboolean        repo_gpgcheck;  /*!< GPG check for repodata */
    gboolean        enablegroups;   /*!< Use groups */

    guint64         bandwidth;      /*!< Bandwidth - Number of bytes */
    gchar          *throttle;       /*!< Throttle string */
    LrIpResolveType ip_resolve;     /*!< Ip resolve type */

    gint64          metadata_expire;/*!< Interval in secs for metadata expiration */
    gint            cost;           /*!< Repo cost */
    gint            priority;       /*!< Repo priority */

    gchar          *sslcacert;      /*!< SSL Certification authority cert */
    gboolean        sslverify;      /*!< SSL verification */
    gchar          *sslclientcert;  /*!< SSL Client certificate */
    gchar          *sslclientkey;   /*!< SSL Client key */

    gchar          **deltarepobaseurl;/*!< Deltarepo mirror URLs */
} LrYumRepoConf;

/** Return new empty LrYumRepoConfs
 * @return          Newly allocated LrYumRepoConfs
 */
LrYumRepoConfs *
lr_yum_repoconfs_init(void);

/** Frees LrYumRepoConfs
 * @param confs     NULL or LrYumRepoConfs
 */
void
lr_yum_repoconfs_free(LrYumRepoConfs *confs);

/** Get GSList of LrYumRepoConf from a LrYumRepoConfs.
 * @param confs     LrYumRepoConfs (not NULL!)
 * @return          Pointer to internal GSList in LrYumRepoConfs
 */
GSList *
lr_yum_repoconfs_get_list(LrYumRepoConfs *confs, GError **err);

/** Parse a *.repo file
 * @param confs     LrYumRepoConfs
 * @param filename  Path to *.repo file
 * @return          TRUE if everything is ok, FALSE if err is set.
 */
gboolean
lr_yum_repoconfs_parse(LrYumRepoConfs *confs,
                       const char *filename,
                       GError **err);

/** Load a directory with *.repo files (e.g. /etc/yum.repos.d/)
 * @param confs     LrYumRepoConfs
 * @param path      Path to a directory with *.repo files
 * @return          TRUE if everything is ok, FALSE if err is set.
 */
gboolean
lr_yum_repoconfs_load_dir(LrYumRepoConfs *confs,
                          const char *path,
                          GError **err);

/** Create a copy of LrYumRepoConf.
 * @param repoconf  LrYumRepoConf
 * @return          Deep copy of input LrYumRepoConf
 */
LrYumRepoConf *
lr_yum_repoconf_copy(LrYumRepoConf *repoconf);

/** @} */

G_END_DECLS

#endif
