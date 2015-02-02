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
