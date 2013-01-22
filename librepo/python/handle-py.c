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
#undef NDEBUG
#include <assert.h>

#include "librepo/librepo.h"

#include "exception-py.h"
#include "handle-py.h"
#include "result-py.h"

#define RETURN_ERROR(res, h) do { \
    int nexterr_num = 0; \
    const char *nexterr = NULL;  \
    if ((h) && (res) == LRE_CURL) { \
        nexterr_num = lr_handle_last_curl_error((h)); \
        nexterr = lr_handle_last_curl_strerror((h)); \
    } \
    if ((h) && (res) == LRE_CURLM) { \
        nexterr_num = lr_handle_last_curlm_error((h)); \
        nexterr = lr_handle_last_curlm_strerror((h)); \
    } \
    if ((h) && (res) == LRE_BADSTATUS) \
        PyErr_Format(LrErr_Exception, "%d: %s: %ld", (res), lr_strerror((res)), lr_handle_last_bad_status_code((h))); \
    else if (nexterr) \
        PyErr_Format(LrErr_Exception, "%d: %s: (%d) %s", (res), lr_strerror((res)), nexterr_num, nexterr); \
    else \
        PyErr_Format(LrErr_Exception, "%d: %s", (res), lr_strerror((res))); \
    return NULL; \
} while (0)

typedef struct {
    PyObject_HEAD
    lr_Handle handle;
    /* Callback */
    PyObject *progress_cb;
    PyObject *progress_cb_data;
} _HandleObject;

lr_Handle
Handle_FromPyObject(PyObject *o)
{
    if (!HandleObject_Check(o)) {
        PyErr_SetString(PyExc_TypeError, "Expected a _librepo.Handle object.");
        return NULL;
    }
    return ((_HandleObject *)o)->handle;
}

static int
check_HandleStatus(const _HandleObject *self)
{
    assert(self != NULL);
    assert(HandleObject_Check(self));
    if (self->handle == NULL) {
        PyErr_SetString(LrErr_Exception, "No librepo handle");
        return -1;
    }
    return 0;
}

/* Callback stuff */

int
progress_callback(void *data, double total_to_download, double now_downloaded)
{
    _HandleObject *self;
    PyObject *user_data, *arglist, *result;

    self = (_HandleObject *)data;
    if (!self->progress_cb)
        return 0;

    if (self->progress_cb_data)
        user_data = self->progress_cb_data;
    else
        // XXX: Ref count for Py_None here??
        user_data = Py_None;

    arglist = Py_BuildValue("(Odd)", user_data, total_to_download, now_downloaded);
    if (arglist == NULL)
        return 0;

    result = PyObject_CallObject(self->progress_cb, arglist);
    Py_DECREF(arglist);
    Py_XDECREF(result);
    return 0;
}

/* Function on the type */

static PyObject *
handle_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _HandleObject *self = (_HandleObject *)type->tp_alloc(type, 0);

    if (self) {
        self->handle = NULL;
        self->progress_cb = NULL;
        self->progress_cb_data = NULL;
    }
    return (PyObject *)self;
}

static int
handle_init(_HandleObject *self, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|", kwlist))
        return -1;

    self->handle = lr_handle_init();
    if (self->handle == NULL) {
        PyErr_SetString(LrErr_Exception, "Handle initialization failed");
        return -1;
    }
    return 0;
}

static void
handle_dealloc(_HandleObject *o)
{
    if (o->handle)
        lr_handle_free(o->handle);
    Py_XDECREF(o->progress_cb);
    Py_XDECREF(o->progress_cb_data);
    Py_TYPE(o)->tp_free(o);
}

static PyObject *
setopt(_HandleObject *self, PyObject *args)
{
    int option;
    PyObject *obj;
    int res = LRE_OK;

    if (!PyArg_ParseTuple(args, "iO:setopt", &option, &obj))
        return NULL;
    if (check_HandleStatus(self))
        return NULL;

    if (option < 0 || option >= LRO_SENTINEL) {
        PyErr_SetString(PyExc_TypeError, "Unknown option");
        return NULL;
    }

    switch (option) {

    /*
     * Options with string arguments (NULL is supported)
     */
    case LRO_URL:
    case LRO_MIRRORLIST:
    case LRO_USERPWD:
    case LRO_PROXY:
    case LRO_PROXYUSERPWD:
    case LRO_DESTDIR: {
        char *str = NULL;

        if (PyString_Check(obj)) {
            str = PyString_AsString(obj);
        } else if (obj != Py_None) {
            PyErr_SetString(PyExc_TypeError, "Only string or None is supported with this option");
            return NULL;
        }

        res = lr_handle_setopt(self->handle, (lr_HandleOption)option, str);
        break;
    }

    /*
     * Options with long/int (boolean) arguments
     */
    case LRO_UPDATE:
    case LRO_LOCAL:
    case LRO_HTTPAUTH:
    case LRO_PROXYSOCK:
    case LRO_PROXYAUTH:
    case LRO_GPGCHECK:
    case LRO_CHECKSUM: {
        PY_LONG_LONG d;

        if (PyInt_Check(obj))
            d = (PY_LONG_LONG) PyInt_AS_LONG(obj);
        else if (PyLong_Check(obj))
            d = PyLong_AsLongLong(obj);
        else if (PyObject_IsTrue(obj) == 1)
            d = 1;
        else if (PyObject_IsTrue(obj) == 0)
            d = 0;
        else {
            PyErr_SetString(PyExc_TypeError, "Only Int, Long or Bool are supported with this option");
            return NULL;
        }

        res = lr_handle_setopt(self->handle, (lr_HandleOption)option, d);
        break;
    }

    /*
     * Options with long/int arguments
     */
    case LRO_PROXYPORT:
    case LRO_RETRIES:
    case LRO_REPOTYPE:
    case LRO_MAXSPEED: {
        PY_LONG_LONG d;

        if (PyInt_Check(obj))
            d = (PY_LONG_LONG) PyInt_AS_LONG(obj);
        else if (PyLong_Check(obj))
            d = PyLong_AsLongLong(obj);
        else {
            PyErr_SetString(PyExc_TypeError, "Only Int or Long are supported with this option");
            return NULL;
        }

        res = lr_handle_setopt(self->handle, (lr_HandleOption)option, d);
        break;
    }

    /*
     * Options with array argument
     */
    case LRO_YUMDLIST: {
        Py_ssize_t len = 0;

        if (!PyList_Check(obj) && obj != Py_None) {
            PyErr_SetString(PyExc_TypeError, "Only List or None type is supported with this option");
            return NULL;
        }

        if (obj == Py_None) {
            res = lr_handle_setopt(self->handle, (lr_HandleOption)option, NULL);
            break;
        }

        len = PyList_Size(obj);
        for (Py_ssize_t x = 0; x < len; x++) {
            PyObject *item = PyList_GetItem(obj, x);
            if (!PyString_Check(item) && item != Py_None) {
                PyErr_SetString(PyExc_TypeError, "Only strings or None is supported in list");
                return NULL;
            }
        }

        char **array[len+1];
        for (Py_ssize_t x = 0; x < len; x++) {
            PyObject *item = PyList_GetItem(obj, x);
            if (PyString_Check(item))
                array[x] = PyString_AsString(item);
            else
                array[x] = NULL;
        }
        array[len] = NULL;

        res = lr_handle_setopt(self->handle, (lr_HandleOption)option, array);
        break;
    }

    /*
     * Options with callable arguments
     */
    case LRO_PROGRESSCB: {
        if (!PyCallable_Check(obj)) {
            PyErr_SetString(PyExc_TypeError, "Only callable arguments are supported with this option");
            return NULL;
        }

        Py_XDECREF(self->progress_cb);
        Py_XINCREF(obj);
        self->progress_cb = obj;
        res = lr_handle_setopt(self->handle, (lr_HandleOption)option, progress_callback);
        if (res != LRE_OK)
            RETURN_ERROR(res, self->handle);
        res = lr_handle_setopt(self->handle, LRO_PROGRESSDATA, self);
        break;
    }

    /*
     * Options with callback data
     */
    case LRO_PROGRESSDATA: {
        Py_XINCREF(obj);
        self->progress_cb_data = obj;
        break;
    }

    /*
     * Unknown options
     */
    default:
        PyErr_SetString(PyExc_TypeError, "Unknown option");
        return NULL;
    }

    if (res != LRE_OK)
        RETURN_ERROR(res, self->handle);
    Py_RETURN_NONE;
}

static PyObject *
getinfo(_HandleObject *self, PyObject *args)
{
    int option;
    int res = LRE_OK;
    char *str;
    long lval;

    if (!PyArg_ParseTuple(args, "i:getinfo", &option))
        return NULL;
    if (check_HandleStatus(self))
        return NULL;

    if (option < 0 || option >= LRI_SENTINEL) {
        PyErr_SetString(PyExc_TypeError, "Unknown option");
        return NULL;
    }

    switch (option) {

    case LRI_URL:
    case LRI_MIRRORLIST:
    case LRI_DESTDIR:
        res = lr_handle_getinfo(self->handle, (lr_HandleInfoOption)option, &str);
        if (res != LRE_OK)
            RETURN_ERROR(res, self->handle);
        if (str == NULL)
            Py_RETURN_NONE;
        return PyString_FromString(str);

    case LRI_UPDATE:
    case LRI_LOCAL:
        res = lr_handle_getinfo(self->handle, (lr_HandleInfoOption)option, &lval);
        if (res != LRE_OK)
            RETURN_ERROR(res, self->handle);
        return PyLong_FromLong(lval);

    case LRI_YUMDLIST: {
        PyObject *yumdlist;
        char **strlist;
        res = lr_handle_getinfo(self->handle, (lr_HandleInfoOption)option, &strlist);
        if (res != LRE_OK)
            RETURN_ERROR(res, self->handle);
        if (strlist == NULL)
            Py_RETURN_NONE;
        yumdlist = PyList_New(0);
        for (int x=0; strlist[x] != NULL; x++)
            PyList_Append(yumdlist, PyString_FromString(strlist[x]));
        return yumdlist;
    }

    default:
        PyErr_SetString(PyExc_TypeError, "Unknown option");
        return NULL;
    }

    printf("+++++++++++++++++++++++++++++++++++++++\n");
    if (res != LRE_OK)
        RETURN_ERROR(res, self->handle);
    Py_RETURN_NONE;
}

static PyObject *
perform(_HandleObject *self, PyObject *args)
{
    PyObject *result_obj;
    lr_Result result;
    int ret;

    if (!PyArg_ParseTuple(args, "O:perform", &result_obj))
        return NULL;
    if (check_HandleStatus(self))
        return NULL;

    result = Result_FromPyObject(result_obj);

    ret = lr_handle_perform(self->handle, result);
    if (ret != LRE_OK)
        RETURN_ERROR(ret, self->handle);

    Py_RETURN_NONE;
}

static PyObject *
download_package(_HandleObject *self, PyObject *args)
{
    int ret;
    char *relative_url, *checksum, *dest, *base_url;
    int resume, checksum_type;

    if (!PyArg_ParseTuple(args, "szizzi:download_package", &relative_url,
                                                           &dest,
                                                           &checksum_type,
                                                           &checksum,
                                                           &base_url,
                                                           &resume))
        return NULL;
    if (check_HandleStatus(self))
        return NULL;

    ret = lr_download_package(self->handle, relative_url, dest, checksum_type,
                              checksum, base_url, resume);
    if (ret != LRE_OK)
        RETURN_ERROR(ret, self->handle);

    Py_RETURN_NONE;
}

static struct
PyMethodDef handle_methods[] = {
    { "setopt", (PyCFunction)setopt, METH_VARARGS, NULL },
    { "getinfo", (PyCFunction)getinfo, METH_VARARGS, NULL },
    { "perform", (PyCFunction)perform, METH_VARARGS, NULL },
    { "download_package", (PyCFunction)download_package, METH_VARARGS, NULL },
    { NULL }
};

PyTypeObject Handle_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                              /* ob_size */
    "_librepo.Handle",              /* tp_name */
    sizeof(_HandleObject),          /* tp_basicsize */
    0,                              /* tp_itemsize */
    (destructor) handle_dealloc,    /* tp_dealloc */
    0,                              /* tp_print */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_compare */
    0,                              /* tp_repr */
    0,                              /* tp_as_number */
    0,                              /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    0,                              /* tp_hash */
    0,                              /* tp_call */
    0,                              /* tp_str */
    0,                              /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    "Handle object",                /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    PyObject_SelfIter,              /* tp_iter */
    0,                              /* tp_iternext */
    handle_methods,                 /* tp_methods */
    0,                              /* tp_members */
    0,                              /* tp_getset */
    0,                              /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    (initproc) handle_init,         /* tp_init */
    0,                              /* tp_alloc */
    handle_new,                     /* tp_new */
    0,                              /* tp_free */
    0,                              /* tp_is_gc */
};
