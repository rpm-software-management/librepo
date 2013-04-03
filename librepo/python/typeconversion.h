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

#ifndef LR_TYPECONVERSION_PY_H
#define LR_TYPECONVERSION_PY_H

#include "librepo/repomd.h"
#include "librepo/yum.h"
#include "librepo/metalink.h"

PyObject *PyObject_FromYumRepo(lr_YumRepo repo);
PyObject *PyObject_FromYumRepoMd(lr_YumRepoMd repomd);
PyObject *PyObject_FromMetalink(lr_Metalink metalink);

#endif
