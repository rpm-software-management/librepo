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
#include <librepo/types.h>

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

/** Configuration for a single repo */
typedef struct _LrYumRepoConf LrYumRepoConf;

/** List of yum repo configurations */
typedef struct _LrYumRepoConfs LrYumRepoConfs;

typedef enum {
    LR_YRC_ID,              /*!<  0 (char *) ID (short name) of the repo */
    LR_YRC_NAME,            /*!<  1 (char *) Pretty name of the repo */
    LR_YRC_ENABLED,         /*!<  2 (long 1 or 0) Is repo enabled? */
    LR_YRC_BASEURL,         /*!<  3 (char **) List of base URLs */
    LR_YRC_MIRRORLIST,      /*!<  4 (char *) Mirrorlist URL */
    LR_YRC_METALINK,        /*!<  5 (char *) Metalink URL */

    LR_YRC_MEDIAID,         /*!<  6 (char *) Media ID */
    LR_YRC_GPGKEY,          /*!<  7 (char **) URL of GPG key */
    LR_YRC_GPGCAKEY,        /*!<  8 (char **) GPG CA key */
    LR_YRC_EXCLUDE,         /*!<  9 (char **) List of excluded packages */
    LR_YRC_INCLUDE,         /*!< 10 (char **) List of included packages */

    LR_YRC_FASTESTMIRROR,   /*!< 11 (long 1 or 0) Fastest mirror determination */
    LR_YRC_PROXY,           /*!< 12 (char *) Proxy address */
    LR_YRC_PROXY_USERNAME,  /*!< 13 (char *) Proxy username */
    LR_YRC_PROXY_PASSWORD,  /*!< 14 (char *) Proxy password */
    LR_YRC_USERNAME,        /*!< 15 (char *) Username */
    LR_YRC_PASSWORD,        /*!< 16 (char *) Password */

    LR_YRC_GPGCHECK,        /*!< 17 (long 1 or 0) GPG check for packages */
    LR_YRC_REPO_GPGCHECK,   /*!< 18 (long 1 or 0) GPG check for repodata */
    LR_YRC_ENABLEGROUPS,    /*!< 19 (long 1 or 0) Use groups */

    LR_YRC_BANDWIDTH,       /*!< 20 (guint64) Bandwidth - Number of bytes */
    LR_YRC_THROTTLE,        /*!< 21 (char *) Throttle string */
    LR_YRC_IP_RESOLVE,      /*!< 22 (LrIpResolveType) Ip resolve type */

    LR_YRC_METADATA_EXPIRE, /*!< 23 (gint64) Interval in secs for metadata expiration */
    LR_YRC_COST,            /*!< 24 (gint) Repo cost */
    LR_YRC_PRIORITY,        /*!< 25 (gint) Repo priority */

    LR_YRC_SSLCACERT,       /*!< 26 (gchar *) SSL Certification authority cert */
    LR_YRC_SSLVERIFY,       /*!< 27 (long 1 or 0) SSL verification */
    LR_YRC_SSLCLIENTCERT,   /*!< 28 (gchar *) SSL Client certificate */
    LR_YRC_SSLCLIENTKEY,    /*!< 29 (gchar *) SSL Client key */

    LR_YRC_DELTAREPOBASEURL,/*!< 30 (char **) Deltarepo mirror URLs */

    LR_YRC_FAILOVERMETHOD,  /*!< 31 (char *) Failover method */
    LR_YRC_SKIP_IF_UNAVAILABLE, /*!< 32 (long 1 or 0) Skip if unavailable */
} LrYumRepoConfOption;

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

/** Add an empty LrYumRepoConf.
 * A file specified by filename don't have to exists - it will be created.
 * @param confs     LrYumRepoConfs
 * @param filename  Path to *.repo file (the file doesn't have to exist)
 * @param ids       IDs of repositories confs there (NULL-terminated list)
 * @param err       GError **
 * @return          TRUE if everything is ok, FALSE if err is set.
 */
gboolean
lr_yum_repoconfs_add_empty_conf(LrYumRepoConfs *confs,
                                const char *filename,
                                const char **ids,
                                GError **err);

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

/** Save all repoconfs loaded in LrYumRepoConfs
 * @param confs         LrYumRepoConfs
 * @param err           GError **
 * @return              TRUE if everything is ok, FALSE if err is set.
 */
gboolean
lr_yum_repoconfs_save(LrYumRepoConfs *confs,
                      GError **err);

/** Save current repoconf (and also all repoconfs loaded from the same file).
 * @param repoconf      LrYumRepoConf
 * @param err           GError **
 * @return              TRUE if everything is ok, FALSE if err is set.
 */
gboolean
lr_yum_repoconf_save(LrYumRepoConf *repoconf,
                     GError **err);

/** Get a value from repo config file *.repo
 * If specified option is not specified in the repo config file,
 * FALSE is returned and error with code LRE_VALUE is set.
 * Note: All returned values are malloced and must be freed by caller.
 * @param repoconf      A repository configuration
 * @param err           GError **
 * @param option        An option
 * @param ...           Appropriate variable for the selected option.
 * @return              TRUE if everything is ok, FALSE if err is set.
 *                      Note value of the target variable passed as vararg
 *                      can be changed and it's state is undefined when
 *                      a FALSE is returned!
 */
gboolean
lr_yum_repoconf_getinfo(LrYumRepoConf *repoconf,
                        GError **err,
                        LrYumRepoConfOption option,
                        ...);

/** Set an option in the config file.
 * Note: This function copies all passed values and
 * caller don't have to keep them around.
 * @param repoconf      A repo configuration
 * @param err           GError **
 * @param option        An option
 * @param ...           A value for an option as an appropriate variable type.
 * @return              TRUE if everything is OK, FALSE if err is set.
 */
gboolean
lr_yum_repoconf_setopt(LrYumRepoConf *repoconf,
                       GError **err,
                       LrYumRepoConfOption option,
                       ...);

/** @} */

G_END_DECLS

#endif
