/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2013  Tomas Mlcoch
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

#include <Python.h>
#undef NDEBUG
#include <assert.h>

#include "librepo/librepo.h"

#include "typeconversion.h"
#include "exception-py.h"
#include "result-py.h"

PyObject *
py_yum_repomd_get_age(PyObject *self, PyObject *args)
{
    lr_Result res;
    PyObject *py_res;
    double age;

    LR_UNUSED(self);

    if (!PyArg_ParseTuple(args, "O!:py_yum_repomd_get_age",
            &Result_Type, &py_res))
        return NULL;

    res = Result_FromPyObject(py_res);
    if (!res)
        return NULL;

    age = lr_yum_repomd_get_age(res);
    return PyFloat_FromDouble(age);
}
