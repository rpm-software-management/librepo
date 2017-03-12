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

#include "exception-py.h"
#include "handle-py.h"
#include "packagetarget-py.h"
#include "result-py.h"
#include "typeconversion.h"
#include "packagedownloader-py.h"
#include "downloader-py.h"

#include "globalstate-py.h"  // GIL Hack

typedef struct {
    PyObject_HEAD
    LrHandle *handle;
    /* Callbacks */
    PyObject *progress_cb;
    PyObject *progress_cb_data;
    PyObject *fastestmirror_cb;
    PyObject *fastestmirror_cb_data;
    PyObject *hmf_cb;
    /* GIL stuff */
    // See: http://docs.python.org/2/c-api/init.html#releasing-the-gil-from-extension-code
    PyThreadState **state;
} _HandleObject;

LrHandle *
Handle_FromPyObject(PyObject *o)
{
    if (!HandleObject_Check(o)) {
        PyErr_SetString(PyExc_TypeError, "Expected a _librepo.Handle object.");
        return NULL;
    }
    return ((_HandleObject *)o)->handle;
}

void
Handle_SetThreadState(PyObject *o, PyThreadState **state)
{
    _HandleObject *self = (_HandleObject *) o;
    if (!self) return;
    self->state = state;
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

static int
progress_callback(void *data, double total_to_download, double now_downloaded)
{
    int ret = LR_CB_OK; // Assume everything will be ok
    _HandleObject *self;
    PyObject *user_data, *result;

    self = (_HandleObject *)data;
    if (!self->progress_cb)
        return LR_CB_OK;

    if (self->progress_cb_data)
        user_data = self->progress_cb_data;
    else
        user_data = Py_None;

    EndAllowThreads(self->state);
    result = PyObject_CallFunction(self->progress_cb,
                        "(Odd)", user_data, total_to_download, now_downloaded);

    if (!result) {
        // Exception raised in callback leads to the abortion
        // of whole downloading (it is considered fatal)
        ret = LR_CB_ERROR;
    } else {
        if (result == Py_None) {
            // Assume that None means that everything is ok
            ret = LR_CB_OK;
#if PY_MAJOR_VERSION < 3
        } else if (PyInt_Check(result)) {
            ret = PyInt_AS_LONG(result);
#endif
        } else if (PyLong_Check(result)) {
            ret = (int) PyLong_AsLong(result);
        } else {
            // It's an error if result is None neither int
            PyErr_SetString(PyExc_TypeError, "Progress callback must return integer number");
            ret = LR_CB_ERROR;
        }
    }

    Py_XDECREF(result);
    BeginAllowThreads(self->state);

    return ret;
}

static void
fastestmirror_callback(void *data, LrFastestMirrorStages stage, void *ptr)
{
    _HandleObject *self;
    PyObject *user_data, *result, *pydata;

    self = (_HandleObject *)data;
    if (!self->fastestmirror_cb)
        return;

    if (self->fastestmirror_cb_data)
        user_data = self->fastestmirror_cb_data;
    else
        user_data = Py_None;

    if (!ptr) {
        pydata = Py_None;
    } else {
        switch (stage) {
        case LR_FMSTAGE_CACHELOADING:
        case LR_FMSTAGE_CACHELOADINGSTATUS:
        case LR_FMSTAGE_STATUS:
            pydata = PyStringOrNone_FromString((char *) ptr);
            pydata = PyUnicode_FromString((char *) ptr);
            break;
        case LR_FMSTAGE_DETECTION:
            pydata = PyLong_FromLong(*((long *) ptr));
            break;
        default:
            pydata = Py_None;
        }
    }

    EndAllowThreads(self->state);
    result = PyObject_CallFunction(self->fastestmirror_cb,
                        "(OlO)", user_data, (long) stage, pydata);
    Py_XDECREF(result);
    BeginAllowThreads(self->state);

    if (pydata != Py_None)
        Py_XDECREF(pydata);

    return;
}

static int
hmf_callback(void *data, const char *msg, const char *url, const char *metadata)
{
    int ret = LR_CB_OK; // Assume everything will be ok
    _HandleObject *self;
    PyObject *user_data, *result, *py_msg, *py_url, *py_metadata;

    self = (_HandleObject *)data;
    if (!self->hmf_cb)
        return LR_CB_OK;

    if (self->progress_cb_data)
        user_data = self->progress_cb_data;
    else
        user_data = Py_None;

    py_msg = PyStringOrNone_FromString(msg);
    py_url = PyStringOrNone_FromString(url);
    py_metadata = PyStringOrNone_FromString(metadata);

    EndAllowThreads(self->state);
    result = PyObject_CallFunction(self->hmf_cb,
                        "(OOOO)", user_data, py_msg, py_url, py_metadata);

    Py_DECREF(py_msg);
    Py_DECREF(py_url);
    Py_DECREF(py_metadata);

    if (!result) {
        // Exception raised in callback leads to the abortion
        // of whole downloading (it is considered fatal)
        ret = LR_CB_ERROR;
    } else {
        if (result == Py_None) {
            // Assume that None means that everything is ok
            ret = LR_CB_OK;
#if PY_MAJOR_VERSION < 3
        } else if (PyInt_Check(result)) {
            ret = PyInt_AS_LONG(result);
#endif
        } else if (PyLong_Check(result)) {
            ret = (int) PyLong_AsLong(result);
        } else {
            // It's an error if result is None neither int
            PyErr_SetString(PyExc_TypeError, "HandleMirrorFailure callback must return integer number");
            ret = LR_CB_ERROR;
        }
    }

    Py_XDECREF(result);
    BeginAllowThreads(self->state);

    return ret;
}

/* Function on the type */

static PyObject *
handle_new(PyTypeObject *type,
           G_GNUC_UNUSED PyObject *args,
           G_GNUC_UNUSED PyObject *kwds)
{
    _HandleObject *self = (_HandleObject *)type->tp_alloc(type, 0);

    if (self) {
        self->handle = NULL;
        self->progress_cb = NULL;
        self->progress_cb_data = NULL;
        self->fastestmirror_cb = NULL;
        self->fastestmirror_cb_data = NULL;
        self->hmf_cb = NULL;
        self->state = NULL;
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
    Py_XDECREF(o->fastestmirror_cb);
    Py_XDECREF(o->fastestmirror_cb_data);
    Py_XDECREF(o->hmf_cb);
    Py_TYPE(o)->tp_free(o);
}

static PyObject *
py_setopt(_HandleObject *self, PyObject *args)
{
    int option;
    PyObject *obj;
    gboolean res = TRUE;
    GError *tmp_err = NULL;

    if (!PyArg_ParseTuple(args, "iO:py_setopt", &option, &obj))
        return NULL;
    if (check_HandleStatus(self))
        return NULL;

    switch (option) {

    /*
     * Options with string arguments (NULL is supported)
     */
    case LRO_MIRRORLIST:
    case LRO_MIRRORLISTURL:
    case LRO_METALINKURL:
    case LRO_USERPWD:
    case LRO_PROXY:
    case LRO_PROXYUSERPWD:
    case LRO_DESTDIR:
    case LRO_USERAGENT:
    case LRO_FASTESTMIRRORCACHE:
    case LRO_GNUPGHOMEDIR:
    case LRO_SSLCLIENTCERT:
    case LRO_SSLCLIENTKEY:
    case LRO_SSLCACERT:
    {
        char *str = NULL, *alloced = NULL;

        if (PyUnicode_Check(obj)) {
            PyObject *bytes = PyUnicode_AsUTF8String(obj);
            if (!bytes) return NULL;
            str = alloced = g_strdup(PyBytes_AsString(bytes));
            Py_XDECREF(bytes);
        } else if (PyBytes_Check(obj)) {
            str = PyBytes_AsString(obj);
        } else if (obj != Py_None) {
            PyErr_SetString(PyExc_TypeError,
                        "Only string or None is supported with this option");
            return NULL;
        }

        res = lr_handle_setopt(self->handle,
                               &tmp_err,
                               (LrHandleOption)option,
                               str);
        g_free(alloced);
        break;
    }

    /*
     * Options with double arguments
     */
    case LRO_FASTESTMIRRORTIMEOUT:
    {
        double d;
        int badarg = 0;

        if (PyFloat_Check(obj))
            d = PyFloat_AS_DOUBLE(obj);
        else if (obj == Py_None) {
            // None stands for default value
            switch (option) {
            case LRO_FASTESTMIRRORTIMEOUT:
                d = LRO_FASTESTMIRRORTIMEOUT_DEFAULT;
                break;
            default:
                badarg = 1;
            }
        } else {
            badarg = 1;
        }

        if (badarg) {
            PyErr_SetString(PyExc_TypeError, "Only float or None is supported with this option");
            return NULL;
        }

        res = lr_handle_setopt(self->handle,
                               &tmp_err,
                               (LrHandleOption)option,
                               d);
        break;
    }

    /*
     * Options with long/int (boolean) arguments
     */
    case LRO_UPDATE:
    case LRO_LOCAL:
    case LRO_HTTPAUTH:
    case LRO_PROXYAUTH:
    case LRO_GPGCHECK:
    case LRO_IGNOREMISSING:
    case LRO_CHECKSUM:
    case LRO_INTERRUPTIBLE:
    case LRO_FETCHMIRRORS:
    case LRO_FASTESTMIRROR:
    case LRO_SSLVERIFYPEER:
    case LRO_SSLVERIFYHOST:
    case LRO_ADAPTIVEMIRRORSORTING:
    case LRO_FTPUSEEPSV:
    case LRO_OFFLINE:
    {
        long d;

        // Default values for None attribute
        if (obj == Py_None && (option == LRO_SSLVERIFYPEER ||
                               option == LRO_SSLVERIFYHOST))
        {
            d = 1;
        } else if (obj == Py_None && option == LRO_ADAPTIVEMIRRORSORTING) {
            d = LRO_ADAPTIVEMIRRORSORTING_DEFAULT;
        } else if (obj == Py_None && option == LRO_FTPUSEEPSV) {
            d = LRO_FTPUSEEPSV_DEFAULT;
        // end of default attributes
        } else if (PyObject_IsTrue(obj) == 1)
            d = 1;
        else if (PyObject_IsTrue(obj) == 0)
            d = 0;
        else {
            PyErr_SetString(PyExc_TypeError, "Only Int, Long or Bool are supported with this option");
            return NULL;
        }

        res = lr_handle_setopt(self->handle,
                               &tmp_err,
                               (LrHandleOption)option,
                               d);
        break;
    }

    /*
     * Options with long/int arguments
     */
    case LRO_PROXYTYPE:
    case LRO_REPOTYPE:
    case LRO_FASTESTMIRRORMAXAGE:
    case LRO_LOWSPEEDTIME:
    case LRO_LOWSPEEDLIMIT:
    case LRO_IPRESOLVE:
    case LRO_ALLOWEDMIRRORFAILURES:
    {
        int badarg = 0;
        long d;

        if (PyLong_Check(obj))
            d = PyLong_AsLong(obj);
#if PY_MAJOR_VERSION < 3
        else if (PyInt_Check(obj))
            d = PyInt_AS_LONG(obj);
#endif
        else if (obj == Py_None) {
            // None stands for default value
            switch (option) {
            case LRO_PROXYTYPE:
                d = LRO_PROXYTYPE_DEFAULT;
                break;
            case LRO_LOWSPEEDTIME:
                d = LRO_LOWSPEEDTIME_DEFAULT;
                break;
            case LRO_LOWSPEEDLIMIT:
                d = LRO_LOWSPEEDLIMIT_DEFAULT;
                break;
            case LRO_FASTESTMIRRORMAXAGE:
                d = LRO_FASTESTMIRRORMAXAGE_DEFAULT;
                break;
            case LRO_IPRESOLVE:
                d = LRO_IPRESOLVE_DEFAULT;
                break;
            case LRO_ALLOWEDMIRRORFAILURES:
                d = LRO_ALLOWEDMIRRORFAILURES_DEFAULT;
                break;
            default:
                badarg = 1;
            }
        } else
            badarg = 1;

        if (badarg) {
            PyErr_SetString(PyExc_TypeError, "Only Int/Long is supported with this option");
            return NULL;
        }

        res = lr_handle_setopt(self->handle,
                               &tmp_err,
                               (LrHandleOption)option,
                               d);
        break;
    }

    /*
     * Options with long/int/None arguments
     */
    case LRO_PROXYPORT:
    case LRO_CONNECTTIMEOUT:
    case LRO_MAXMIRRORTRIES:
    case LRO_MAXPARALLELDOWNLOADS:
    case LRO_MAXDOWNLOADSPERMIRROR:
    case LRO_HTTPAUTHMETHODS:
    case LRO_PROXYAUTHMETHODS:
    {
        long d;

        if (PyLong_Check(obj))
            d = PyLong_AsLong(obj);
#if PY_MAJOR_VERSION < 3
        else if (PyInt_Check(obj))
            d = PyInt_AS_LONG(obj);
#endif
        else if (obj == Py_None) {
            /* Default options */
            if (option == LRO_PROXYPORT)
                d = LRO_PROXYPORT_DEFAULT;
            else if (option == LRO_MAXMIRRORTRIES)
                d = LRO_MAXMIRRORTRIES_DEFAULT;
            else if (option == LRO_CONNECTTIMEOUT)
                d = LRO_CONNECTTIMEOUT_DEFAULT;
            else if (option == LRO_MAXPARALLELDOWNLOADS)
                d = LRO_MAXPARALLELDOWNLOADS_DEFAULT;
            else if (option == LRO_MAXDOWNLOADSPERMIRROR)
                d = LRO_MAXDOWNLOADSPERMIRROR_DEFAULT;
            else if (option == LRO_HTTPAUTHMETHODS)
                d = LRO_HTTPAUTHMETHODS_DEFAULT;
            else if (option == LRO_PROXYAUTHMETHODS)
                d = LRO_PROXYAUTHMETHODS_DEFAULT;
            else
                assert(0);
        } else {
            PyErr_SetString(PyExc_TypeError, "Only Int/Long/None is supported with this option");
            return NULL;
        }

        res = lr_handle_setopt(self->handle,
                               &tmp_err,
                               (LrHandleOption)option,
                               d);
        break;
    }

    /*
     * Options with gint64/None arguments
     */
    case LRO_MAXSPEED:
    {
        gint64 d;

        if (PyLong_Check(obj))
            d = (gint64) PyLong_AsLongLong(obj);
#if PY_MAJOR_VERSION < 3
        else if (PyInt_Check(obj))
            d = (gint64) PyInt_AS_LONG(obj);
#endif
        else if (obj == Py_None) {
            /* Default options */
            if (option == LRO_MAXSPEED)
                d = (gint64) LRO_MAXSPEED_DEFAULT;
            else
                assert(0);
        } else {
            PyErr_SetString(PyExc_TypeError, "Only Int/Long/None is supported with this option");
            return NULL;
        }

        res = lr_handle_setopt(self->handle,
                               &tmp_err,
                               (LrHandleOption)option,
                               d);
        break;
    }

    /*
     * Options with array argument
     */
    case LRO_URLS:
    case LRO_YUMDLIST:
    case LRO_YUMBLIST:
    case LRO_HTTPHEADER:
    {
        Py_ssize_t len = 0;

        if (!PyList_Check(obj) && obj != Py_None) {
            PyErr_SetString(PyExc_TypeError, "Only List or None type is supported with this option");
            return NULL;
        }

        if (obj == Py_None) {
            res = lr_handle_setopt(self->handle,
                                   &tmp_err,
                                   (LrHandleOption)option,
                                   NULL);
            break;
        }

        len = PyList_Size(obj);
        for (Py_ssize_t x = 0; x < len; x++) {
            PyObject *item = PyList_GetItem(obj, x);
            if (!PyBytes_Check(item) && !PyUnicode_Check(item) && item != Py_None) {
                PyErr_SetString(PyExc_TypeError, "Only strings or Nones are supported in list");
                return NULL;
            }
        }

        GStringChunk *chunk = g_string_chunk_new(0);
        GPtrArray *ptrarray = g_ptr_array_sized_new(len + 1);
        for (Py_ssize_t x = 0; x < len; x++) {
            PyObject *item = PyList_GetItem(obj, x);

            if (PyUnicode_Check(item)) {
                PyObject *bytes = PyUnicode_AsUTF8String(item);
                if (!bytes) {
                    g_ptr_array_free(ptrarray, TRUE);
                    g_string_chunk_free(chunk);
                    return NULL;
                }
                char *item_str = g_string_chunk_insert(chunk,
                                                    PyBytes_AsString(bytes));
                Py_XDECREF(bytes);
                g_ptr_array_add(ptrarray, item_str);
            }

            if (PyBytes_Check(item))
                g_ptr_array_add(ptrarray, PyBytes_AsString(item));
        }
        g_ptr_array_add(ptrarray, NULL);

        res = lr_handle_setopt(self->handle,
                               &tmp_err,
                               (LrHandleOption)option,
                               ptrarray->pdata);
        g_string_chunk_free(chunk);
        g_ptr_array_free(ptrarray, TRUE);
        break;
    }

    case LRO_YUMSLIST:
    case LRO_VARSUB: {
        Py_ssize_t len = 0;
        LrUrlVars *vars = NULL;

        if (!PyList_Check(obj) && obj != Py_None) {
            PyErr_SetString(PyExc_TypeError, "Only List of tuples or None type is supported with this option");
            return NULL;
        }

        if (obj == Py_None) {
            res = lr_handle_setopt(self->handle,
                                   &tmp_err,
                                   (LrHandleOption)option,
                                   NULL);
            break;
        }

        // Check all list elements
        len = PyList_Size(obj);
        for (Py_ssize_t x = 0; x < len; x++) {
            PyObject *item = PyList_GetItem(obj, x);
            PyObject *tuple_item;

            if (!PyTuple_Check(item) || PyTuple_Size(item) != 2) {
                PyErr_SetString(PyExc_TypeError, "List elements has to be "
                    "tuples with exactly 2 elements");
                return NULL;
            }

            tuple_item = PyTuple_GetItem(item, 1);
            if ((!PyBytes_Check(PyTuple_GetItem(item, 0))
                && !PyUnicode_Check(PyTuple_GetItem(item, 0))) ||
                (!PyBytes_Check(tuple_item)
                 && !PyUnicode_Check(tuple_item)
                 && tuple_item != Py_None))
            {
                PyErr_SetString(PyExc_TypeError, "Bad list format");
                return NULL;
            }
        }

        GStringChunk *chunk = g_string_chunk_new(0);
        for (Py_ssize_t x = 0; x < len; x++) {
            PyObject *item = PyList_GetItem(obj, x);
            PyObject *tuple_item;
            char *var, *val;

            tuple_item = PyTuple_GetItem(item, 0);
            if (PyBytes_Check(tuple_item)) {
                // PyBytes
                var = PyBytes_AsString(tuple_item);
            } else {
                // PyUnicode
                PyObject *bytes = PyUnicode_AsUTF8String(tuple_item);
                if (!bytes) {
                    lr_urlvars_free(vars);
                    g_string_chunk_free(chunk);
                    return NULL;
                }
                char *item_str = g_string_chunk_insert(chunk,
                                                    PyBytes_AsString(bytes));
                Py_XDECREF(bytes);
                var = item_str;
            }

            tuple_item = PyTuple_GetItem(item, 1);
            if (tuple_item == Py_None) {
                // Py_None
                val = NULL;
            } else if (PyBytes_Check(tuple_item)) {
                // PyBytes
                val = PyBytes_AsString(tuple_item);
            } else {
                // PyUnicode
                PyObject *bytes = PyUnicode_AsUTF8String(tuple_item);
                if (!bytes) {
                    lr_urlvars_free(vars);
                    g_string_chunk_free(chunk);
                    return NULL;
                }
                char *item_str = g_string_chunk_insert(chunk,
                                                    PyBytes_AsString(bytes));
                Py_XDECREF(bytes);
                val = item_str;
            }

            vars = lr_urlvars_set(vars, var, val);
        }

        res = lr_handle_setopt(self->handle,
                               &tmp_err,
                               (LrHandleOption)option,
                               vars);
        g_string_chunk_free(chunk);
        break;
    }

    /*
     * Options with callable arguments
     */
    case LRO_PROGRESSCB: {
        if (!PyCallable_Check(obj) && obj != Py_None) {
            PyErr_SetString(PyExc_TypeError, "Only callable argument or None is supported with this option");
            return NULL;
        }

        Py_XDECREF(self->progress_cb);
        if (obj == Py_None) {
            // None object
            self->progress_cb = NULL;
            res = lr_handle_setopt(self->handle,
                                   &tmp_err,
                                   (LrHandleOption)option,
                                   NULL);
            if (!res)
                RETURN_ERROR(&tmp_err, -1, NULL);
        } else {
            // New callback object
            Py_XINCREF(obj);
            self->progress_cb = obj;
            res = lr_handle_setopt(self->handle,
                                   &tmp_err,
                                   (LrHandleOption)option,
                                   progress_callback);
            if (!res)
                RETURN_ERROR(&tmp_err, -1, NULL);
            res = lr_handle_setopt(self->handle,
                                   &tmp_err,
                                   LRO_PROGRESSDATA,
                                   self);
        }
        break;
    }

    case LRO_FASTESTMIRRORCB: {
        if (!PyCallable_Check(obj) && obj != Py_None) {
            PyErr_SetString(PyExc_TypeError, "Only callable argument or None is supported with this option");
            return NULL;
        }

        Py_XDECREF(self->fastestmirror_cb);
        if (obj == Py_None) {
            // None object
            self->fastestmirror_cb = NULL;
            res = lr_handle_setopt(self->handle,
                                   &tmp_err,
                                   (LrHandleOption)option,
                                   NULL);
            if (!res)
                RETURN_ERROR(&tmp_err, -1, NULL);
        } else {
            // New callback object
            Py_XINCREF(obj);
            self->fastestmirror_cb = obj;
            res = lr_handle_setopt(self->handle,
                                   &tmp_err,
                                   (LrHandleOption)option,
                                   fastestmirror_callback);
            if (!res)
                RETURN_ERROR(&tmp_err, -1, NULL);
            res = lr_handle_setopt(self->handle,
                                   &tmp_err,
                                   LRO_FASTESTMIRRORDATA,
                                   self);
        }
        break;
    }

    case LRO_HMFCB: {
        if (!PyCallable_Check(obj) && obj != Py_None) {
            PyErr_SetString(PyExc_TypeError, "Only callable argument or None is supported with this option");
            return NULL;
        }

        Py_XDECREF(self->hmf_cb);
        if (obj == Py_None) {
            // None object
            self->hmf_cb = NULL;
            res = lr_handle_setopt(self->handle,
                                   &tmp_err,
                                   (LrHandleOption)option,
                                   NULL);
            if (!res)
                RETURN_ERROR(&tmp_err, -1, NULL);
        } else {
            // New callback object
            Py_XINCREF(obj);
            self->hmf_cb = obj;
            res = lr_handle_setopt(self->handle,
                                   &tmp_err,
                                   (LrHandleOption)option,
                                   hmf_callback);
            if (!res)
                RETURN_ERROR(&tmp_err, -1, NULL);
            res = lr_handle_setopt(self->handle,
                                   &tmp_err,
                                   LRO_PROGRESSDATA,
                                   self);
        }
        break;
    }


    /*
     * Options with callback data
     */
    case LRO_PROGRESSDATA: {
        if (obj == Py_None) {
            self->progress_cb_data = NULL;
        } else {
            Py_XINCREF(obj);
            self->progress_cb_data = obj;
        }
        break;
    }

    case LRO_FASTESTMIRRORDATA: {
        if (obj == Py_None) {
            self->fastestmirror_cb_data = NULL;
        } else {
            Py_XINCREF(obj);
            self->fastestmirror_cb_data = obj;
        }
        break;
    }

    /*
     * Unknown options
     */
    default:
        PyErr_SetString(PyExc_ValueError, "Unknown option");
        return NULL;
    }

    if (!res)
        RETURN_ERROR(&tmp_err, -1, NULL);
    Py_RETURN_NONE;
}

static PyObject *
py_getinfo(_HandleObject *self, PyObject *args)
{
    int option;
    gboolean res = TRUE;
    char *str;
    long lval;
    double dval;
    GError *tmp_err = NULL;

    if (!PyArg_ParseTuple(args, "i:py_getinfo", &option))
        return NULL;
    if (check_HandleStatus(self))
        return NULL;

    switch (option) {

    /* char** options */
    case LRI_MIRRORLIST:
    case LRI_MIRRORLISTURL:
    case LRI_METALINKURL:
    case LRI_DESTDIR:
    case LRI_USERAGENT:
    case LRI_FASTESTMIRRORCACHE:
    case LRI_GNUPGHOMEDIR:
    case LRI_SSLCLIENTCERT:
    case LRI_SSLCLIENTKEY:
    case LRI_SSLCACERT:
        res = lr_handle_getinfo(self->handle,
                                &tmp_err,
                                (LrHandleInfoOption)option,
                                &str);
        if (!res)
            RETURN_ERROR(&tmp_err, -1, NULL);
        return PyStringOrNone_FromString(str);

    /* double* options */
    case LRI_FASTESTMIRRORTIMEOUT:
        res = lr_handle_getinfo(self->handle,
                                &tmp_err,
                                (LrHandleInfoOption)option,
                                &dval);
        if (!res)
            RETURN_ERROR(&tmp_err, -1, NULL);
        return PyFloat_FromDouble(dval);

    /* long* options */
    case LRI_UPDATE:
    case LRI_LOCAL:
    case LRI_REPOTYPE:
    case LRI_FETCHMIRRORS:
    case LRI_MAXMIRRORTRIES:
    case LRI_FASTESTMIRROR:
    case LRI_FASTESTMIRRORMAXAGE:
    case LRI_SSLVERIFYPEER:
    case LRI_SSLVERIFYHOST:
    case LRI_ALLOWEDMIRRORFAILURES:
    case LRI_ADAPTIVEMIRRORSORTING:
    case LRI_OFFLINE:
    case LRI_LOWSPEEDTIME:
    case LRI_LOWSPEEDLIMIT:
    case LRI_FTPUSEEPSV:
        res = lr_handle_getinfo(self->handle,
                                &tmp_err,
                                (LrHandleInfoOption)option,
                                &lval);
        if (!res)
            RETURN_ERROR(&tmp_err, -1, NULL);
        return PyLong_FromLong(lval);

    /* LrAuth* option */
    case LRI_HTTPAUTHMETHODS:
    case LRI_PROXYAUTHMETHODS: {
        LrAuth auth = 0;
        res = lr_handle_getinfo(self->handle,
                                &tmp_err,
                                (LrHandleInfoOption)option,
                                &auth);
        if (!res)
            RETURN_ERROR(&tmp_err, -1, NULL);
        return PyLong_FromLong((long) auth);
    }

    /* LrIpResolveType* option  */
    case LRI_IPRESOLVE: {
        LrIpResolveType type;
        res = lr_handle_getinfo(self->handle,
                                &tmp_err,
                                (LrHandleInfoOption)option,
                                &type);
        if (!res)
            RETURN_ERROR(&tmp_err, -1, NULL);
        return PyLong_FromLong((long) type);
    }

    /* List option */
    case LRI_YUMSLIST:
    case LRI_VARSUB: {
        LrUrlVars *vars;
        PyObject *list;

        res = lr_handle_getinfo(self->handle,
                                &tmp_err,
                                (LrHandleInfoOption)option,
                                &vars);
        if (!res)
            RETURN_ERROR(&tmp_err, -1, NULL);

        if (vars == NULL)
            Py_RETURN_NONE;

        list = PyList_New(0);
        for (LrUrlVars *elem = vars; elem; elem = g_slist_next(elem)) {
            PyObject *tuple, *obj;
            LrVar *var = elem->data;

            tuple = PyTuple_New(2);
            obj = PyStringOrNone_FromString(var->var);
            PyTuple_SetItem(tuple, 0, obj);

            if (var->val != NULL) {
                obj = PyStringOrNone_FromString(var->val);
            } else {
                Py_INCREF(Py_None);
                obj = Py_None;
            }

            PyTuple_SetItem(tuple, 1, obj);

            PyList_Append(list, tuple);
        }
        return list;
    }

    /* char*** options */
    case LRI_URLS:
    case LRI_YUMDLIST:
    case LRI_YUMBLIST:
    case LRI_MIRRORS:
    case LRI_HTTPHEADER:
    {
        PyObject *list;
        char **strlist;
        res = lr_handle_getinfo(self->handle,
                                &tmp_err,
                                (LrHandleInfoOption)option,
                                &strlist);
        if (!res)
            RETURN_ERROR(&tmp_err, -1, NULL);
        if (strlist == NULL) {
            if (option == LRI_MIRRORS || option == LRI_URLS) {
                return PyList_New(0);
            } else {
                Py_RETURN_NONE;
            }
        }
        list = PyList_New(0);
        for (int x=0; strlist[x] != NULL; x++) {
            PyList_Append(list, PyStringOrNone_FromString(strlist[x]));
        }

        g_strfreev(strlist);

        return list;
    }

    /* callback option */
    case LRI_PROGRESSCB:
        if (self->progress_cb == NULL)
            Py_RETURN_NONE;
        Py_INCREF(self->progress_cb);
        return self->progress_cb;

    /* callback data options */
    case LRI_PROGRESSDATA:
        if (self->progress_cb_data == NULL)
            Py_RETURN_NONE;
        Py_INCREF(self->progress_cb_data);
        return self->progress_cb_data;

    case LRI_HMFCB:
        if (self->hmf_cb == NULL)
            Py_RETURN_NONE;
        Py_INCREF(self->hmf_cb);
        return self->hmf_cb;

    /* metalink */
    case LRI_METALINK: {
        PyObject *py_metalink;
        LrMetalink *metalink;
        res = lr_handle_getinfo(self->handle,
                                &tmp_err,
                                (LrHandleInfoOption)option,
                                &metalink);
        if (!res)
            RETURN_ERROR(&tmp_err, -1, NULL);
        if (metalink == NULL)
            Py_RETURN_NONE;
        py_metalink = PyObject_FromMetalink(metalink);
        return py_metalink;
    }

    default:
        PyErr_SetString(PyExc_ValueError, "Unknown option");
        return NULL;
    }

    assert(res);
    Py_RETURN_NONE;
}

static PyObject *
py_perform(_HandleObject *self, PyObject *args)
{
    PyObject *result_obj;
    LrResult *result;
    gboolean ret;
    GError *tmp_err = NULL;
    PyThreadState *state = NULL;

    if (!PyArg_ParseTuple(args, "O:py_perform", &result_obj))
        return NULL;
    if (check_HandleStatus(self))
        return NULL;

    result = Result_FromPyObject(result_obj);

    Handle_SetThreadState((PyObject *) self, &state);

    // XXX: GIL Hack
    int hack_rc = gil_logger_hack_begin(&state);
    if (hack_rc == GIL_HACK_ERROR)
        return NULL;

    BeginAllowThreads(&state);
    ret = lr_handle_perform(self->handle, result, &tmp_err);
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

static PyObject *
py_download_package(_HandleObject *self, PyObject *args)
{
    gboolean ret;
    char *relative_url, *checksum, *dest, *base_url;
    int resume, checksum_type;
    PY_LONG_LONG expectedsize;
    GError *tmp_err = NULL;
    PyThreadState *state = NULL;

    if (!PyArg_ParseTuple(args, "szizLzi:py_download_package", &relative_url,
                                                               &dest,
                                                               &checksum_type,
                                                               &checksum,
                                                               &expectedsize,
                                                               &base_url,
                                                               &resume))
        return NULL;
    if (check_HandleStatus(self))
        return NULL;

    Handle_SetThreadState((PyObject *) self, &state);

    // XXX: GIL Hack
    int hack_rc = gil_logger_hack_begin(&state);
    if (hack_rc == GIL_HACK_ERROR)
        return NULL;

    BeginAllowThreads(&state);
    ret = lr_download_package(self->handle, relative_url, dest, checksum_type,
                              checksum, (gint64) expectedsize, base_url,
                              resume, &tmp_err);
    EndAllowThreads(&state);

    // XXX: GIL Hack
    if (!gil_logger_hack_end(hack_rc))
        return NULL;

    assert((ret && !tmp_err) || (!ret && tmp_err));

    if (!ret && tmp_err->code == LRE_INTERRUPTED) {
        g_error_free(tmp_err);
        PyErr_SetInterrupt();
        PyErr_CheckSignals();
        return NULL;
    }

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

static struct
PyMethodDef handle_methods[] = {
    { "setopt", (PyCFunction)py_setopt, METH_VARARGS, NULL },
    { "getinfo", (PyCFunction)py_getinfo, METH_VARARGS, NULL },
    { "perform", (PyCFunction)py_perform, METH_VARARGS, NULL },
    { "download_package", (PyCFunction)py_download_package, METH_VARARGS, NULL },
    { NULL }
};

PyTypeObject Handle_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
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
