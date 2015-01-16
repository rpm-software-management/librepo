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
#include <glib.h>
#include <glib/gprintf.h>
#include "exception-py.h"
#include "typeconversion.h"

PyObject *LrErr_Exception = NULL;

int
init_exceptions()
{
    LrErr_Exception = PyErr_NewException("librepo.LibrepoException", NULL, NULL);
    if (!LrErr_Exception)
        return 0;
    Py_INCREF(LrErr_Exception);

    return 1;
}

void *
return_error(GError **err, int rc, const char *format, ...)
{
    int ret, code;
    va_list vl;
    gchar *message, *err_message, *usr_message = NULL;
    PyObject *exception_type, *exception_val;

    assert(err || rc > 0);
    assert(!err || *err);

    if (format) {
        // Prepare user message
        va_start(vl, format);
        ret = g_vasprintf(&usr_message, format, vl);
        va_end(vl);

        if (ret < 0) {
            // vasprintf failed - silently ignore this error
            g_free(usr_message);
            usr_message = NULL;
        }
    }

    // Select error message
    if (err)
        err_message = (*err)->message;
    else
        err_message = (char *) lr_strerror(rc);

    // Prepare complete message
    if (usr_message)
        message = g_strdup_printf("%s%s", usr_message, err_message);
    else
        message = g_strdup(err_message);

    g_free(usr_message);

    if (err)
        code = (*err)->code;
    else
        code = rc;

    g_clear_error(err);

    // Select appropriate exception type
    switch (code) {
        case LRE_IO:
        case LRE_CANNOTCREATEDIR:
        case LRE_CANNOTCREATETMP:
            exception_type = PyExc_IOError;
            break;
        case LRE_MEMORY:
            exception_type = PyExc_MemoryError;
            break;
        case LRE_BADFUNCARG:
        case LRE_BADOPTARG:
            exception_type = PyExc_ValueError;
            break;
        default:
            exception_type = LrErr_Exception;
    }

    PyObject *py_msg = PyStringOrNone_FromString(message);
    PyObject *py_strerror = PyStringOrNone_FromString(lr_strerror(code));

    // Set exception
    if (exception_type == PyExc_IOError) {
        // Because of IOError exception has a special formating
        // It Looks like:
        // [Errno unknown] Cannot create output directory: 'Cannot create output directory'
        PyObject *py_unknown = PyStringOrNone_FromString("unknown");
        exception_val = Py_BuildValue("(OOO)",
                                      py_unknown,
                                      py_msg,
                                      py_strerror);
        Py_DECREF(py_unknown);
    } else {
        exception_val = Py_BuildValue("(iOO)",
                                      (int) code,
                                      py_msg,
                                      py_strerror);
    }

    Py_DECREF(py_msg);
    Py_DECREF(py_strerror);

    PyErr_SetObject(exception_type, exception_val);

    g_free(message);

    return NULL;
}
