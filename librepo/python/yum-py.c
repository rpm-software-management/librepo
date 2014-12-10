/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2013  Tomas Mlcoch
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

#include <Python.h>
#include <glib.h>
#undef NDEBUG
#include <assert.h>

#include "librepo/librepo.h"

#include "typeconversion.h"
#include "exception-py.h"
#include "result-py.h"

PyObject *
py_yum_repomd_get_age(G_GNUC_UNUSED PyObject *self, PyObject *args)
{
    LrResult *res;
    PyObject *py_res;
    double age;

    if (!PyArg_ParseTuple(args, "O!:py_yum_repomd_get_age",
            &Result_Type, &py_res))
        return NULL;

    res = Result_FromPyObject(py_res);
    if (!res)
        return NULL;

    age = lr_yum_repomd_get_age(res);
    return PyFloat_FromDouble(age);
}
