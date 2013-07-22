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
#include "exception-py.h"

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
return_error(int rc, lr_Handle h)
{
    /* Create an exception based on rc (and handle - optionaly).
     * Exception type is LrErr_Exception and value is tuple:
     * ((long) return_code, (string) error_message, extra_info)
     **/

    PyObject *err;

    if (!h) {
        // No handle specified
        err = Py_BuildValue("(isO)", rc, lr_strerror(rc), Py_None);
        PyErr_SetObject(LrErr_Exception, err);
        return NULL;
    }

    err = Py_BuildValue("isO", rc, lr_strerror(rc), Py_None);

    PyErr_SetObject(LrErr_Exception, err);

    return NULL;
}
