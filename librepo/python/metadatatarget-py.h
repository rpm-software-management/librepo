/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2016  Martin Hatina
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

#ifndef LIBREPO_METADATATARGET_PY_H
#define LIBREPO_METADATATARGET_PY_H

#include <Python.h>
#include <librepo/librepo.h>

extern PyTypeObject MetadataTarget_Type;

#define MetadataTargetObject_Check(o)   PyObject_TypeCheck(o, &MetadataTarget_Type)

LrMetadataTarget *MetadataTarget_FromPyObject(PyObject *o);
void MetadataTarget_SetThreadState(PyObject *o, PyThreadState **state);


#endif //LIBREPO_METADATATARGET_PY_H
