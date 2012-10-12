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

#include <Python.h>

#include "librepo/librepo.h"

#include "exception-py.h"

static PyObject *
py_global_init(PyObject *self, PyObject *noarg)
{
    lr_global_init();
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
py_global_cleanup(PyObject *self, PyObject *noarg)
{
    lr_global_cleanup();
    Py_INCREF(Py_None);
    return Py_None;
}

static struct PyMethodDef librepo_methods[] = {
    { "global_init",    (PyCFunction) py_global_init,
      METH_NOARGS, NULL },
    { "global_cleanup", (PyCFunction)py_global_cleanup,
      METH_NOARGS, NULL },
    { NULL }
};

PyMODINIT_FUNC
init_librepo(void)
{
    PyObject *m = Py_InitModule("_librepo", librepo_methods);
    if (!m)
        return;

    if (!init_exceptions())
        return;
    PyModule_AddObject(m, "Exception", LrErr_Exception);
}
