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

#include <Python.h>
#include <glib.h>

#include "librepo/librepo.h"

#include "metadatatarget-py.h"
#include "metadatadownloader-py.h"
#include "globalstate-py.h"
#include "downloader-py.h"

PyObject *
py_download_metadata(PyObject *self, PyObject *args)
{
    gboolean ret;
    PyObject *py_list;
    GError *error = NULL;
    PyThreadState *state = NULL;

    if (!PyArg_ParseTuple(args, "O!:download_metadata",
                          &PyList_Type, &py_list))
        return NULL;

    // Convert python list to GSList
    GSList *list = NULL;
    Py_ssize_t len = PyList_Size(py_list);
    for (Py_ssize_t x=0; x < len; x++) {
        PyObject *py_metadatatarget = PyList_GetItem(py_list, x);
        LrMetadataTarget *target = MetadataTarget_FromPyObject(py_metadatatarget);
        if (!target)
            return NULL;
        MetadataTarget_SetThreadState(py_metadatatarget, &state);
        list = g_slist_append(list, target);
    }

    Py_XINCREF(py_list);

    // XXX: GIL Hack
    int hack_rc = gil_logger_hack_begin(&state);
    if (hack_rc == GIL_HACK_ERROR)
        return NULL;

    BeginAllowThreads(&state);
    ret = lr_download_metadata(list, &error);
    EndAllowThreads(&state);

    // XXX: GIL Hack
    if (!gil_logger_hack_end(hack_rc))
        return NULL;

    assert((ret && !error) || (!ret && error));

    Py_XDECREF(py_list);

    if (ret)
        Py_RETURN_NONE; // All fine - Return None

    // Error occurred
    if (PyErr_Occurred()) {
        // Python exception occurred (in a python callback probably)
        return NULL;
    } else if(error->code == LRE_INTERRUPTED) {
        // Interrupted by Ctr+C
        g_error_free(error);
        PyErr_SetInterrupt();
        PyErr_CheckSignals();
        return NULL;
    } else {
        // Return exception created from GError
        RETURN_ERROR(&error, -1, NULL);
    }
}
