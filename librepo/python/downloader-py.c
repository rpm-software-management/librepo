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

#include <Python.h>
#undef NDEBUG
#include <assert.h>

#include "librepo/librepo.h"

#include "packagedownloader-py.h"
#include "handle-py.h"
#include "packagetarget-py.h"
#include "exception-py.h"
#include "globalstate-py.h" // GIL Hack

void
BeginAllowThreads(PyThreadState **state)
{
    assert(state);
    assert(*state == NULL);
    (*state) = PyEval_SaveThread();
}

void
EndAllowThreads(PyThreadState **state)
{
    assert(state);
    assert(*state);
    PyEval_RestoreThread(*state);
    (*state) = NULL;
}

PyObject *
py_download_url(G_GNUC_UNUSED PyObject *self, PyObject *args)
{
    gboolean ret;
    PyObject *py_handle;
    LrHandle *handle = NULL;
    char *url;
    int fd;
    GError *tmp_err = NULL;
    PyThreadState *state = NULL;

    if (!PyArg_ParseTuple(args, "Osi:download_url",
                          &py_handle, &url, &fd))
        return NULL;

    if (HandleObject_Check(py_handle)) {
        handle = Handle_FromPyObject(py_handle);
        Handle_SetThreadState(py_handle, &state);
    } else if (py_handle != Py_None) {
        PyErr_SetString(PyExc_TypeError, "Only Handle or None is supported");
        return NULL;
    }

    // XXX: GIL Hack
    int hack_rc = gil_logger_hack_begin(&state);
    if (hack_rc == GIL_HACK_ERROR)
        return NULL;

    BeginAllowThreads(&state);
    ret = lr_download_url(handle, url, fd, &tmp_err);
    EndAllowThreads(&state);

    // XXX: GIL Hack
    if (!gil_logger_hack_end(hack_rc))
        return NULL;

    assert((ret && !tmp_err) || (!ret && tmp_err));

    if (ret)
        Py_RETURN_NONE; // All fine - Return None

    // Error occurred
    if (PyErr_Occurred()) {
        // Python exception occurred (in a python callback probably)
        return NULL;
    } else if(tmp_err->code == LRE_INTERRUPTED) {
        // Interrupted by Ctr+C
        g_error_free(tmp_err);
        PyErr_SetInterrupt();
        PyErr_CheckSignals();
        return NULL;
    } else {
        // Return exception created from GError
        RETURN_ERROR(&tmp_err, -1, NULL);
    }
}
