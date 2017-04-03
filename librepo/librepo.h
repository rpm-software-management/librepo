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

#ifndef __LR_LIBREPO_H__
#define __LR_LIBREPO_H__

#include <glib.h>

#include "checksum.h"
#include "fastestmirror.h"
#include "gpg.h"
#include "handle.h"
#include "metalink.h"
#include "metadata_downloader.h"
#include "package_downloader.h"
#include "rcodes.h"
#include "repoconf.h"
#include "repomd.h"
#include "repoutil_yum.h"
#include "result.h"
#include "types.h"
#include "url_substitution.h"
#include "util.h"
#include "version.h"
#include "xmlparser.h"
#include "yum.h"

// Low level downloading interface
// (API could be changed significantly between two versions)

#include "downloader.h"
#include "downloadtarget.h"

#endif
