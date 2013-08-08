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
#include "packagetarget-py.h"
#include "result-py.h"
#include "typeconversion.h"

typedef struct {
    PyObject_HEAD
    LrHandle *handle;
    /* Callback */
    PyObject *progress_cb;
    PyObject *progress_cb_data;
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
    _HandleObject *self;
    PyObject *user_data, *arglist, *result;

    self = (_HandleObject *)data;
    if (!self->progress_cb)
        return 0;

    if (self->progress_cb_data)
        user_data = self->progress_cb_data;
    else
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
handle_new(PyTypeObject *type,
           G_GNUC_UNUSED PyObject *args,
           G_GNUC_UNUSED PyObject *kwds)
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
    case LRO_DESTDIR:
    case LRO_USERAGENT: {
        char *str = NULL;

        if (PyString_Check(obj)) {
            str = PyString_AsString(obj);
        } else if (obj != Py_None) {
            PyErr_SetString(PyExc_TypeError, "Only string or None is supported with this option");
            return NULL;
        }

        res = lr_handle_setopt(self->handle, (LrHandleOption)option, str);
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
    {
        long d;

        if (PyInt_Check(obj))
            d = PyInt_AS_LONG(obj);
        else if (PyLong_Check(obj))
            d = PyLong_AsLong(obj);
        else if (PyObject_IsTrue(obj) == 1)
            d = 1;
        else if (PyObject_IsTrue(obj) == 0)
            d = 0;
        else {
            PyErr_SetString(PyExc_TypeError, "Only Int, Long or Bool are supported with this option");
            return NULL;
        }

        res = lr_handle_setopt(self->handle, (LrHandleOption)option, d);
        break;
    }

    /*
     * Options with long/int arguments
     */
    case LRO_PROXYTYPE:
    case LRO_REPOTYPE:
    {
        int badarg = 0;
        long d;

        if (PyInt_Check(obj))
            d = PyInt_AS_LONG(obj);
        else if (PyLong_Check(obj))
            d = PyLong_AsLong(obj);
        else if (obj == Py_None) {
            // None stands for default value
            if (option == LRO_PROXYTYPE)
                d = LR_PROXY_HTTP;
            else
                badarg = 1;
        } else
            badarg = 1;

        if (badarg) {
            PyErr_SetString(PyExc_TypeError, "Only Int/Long is supported with this option");
            return NULL;
        }

        res = lr_handle_setopt(self->handle, (LrHandleOption)option, d);
        break;
    }

    /*
     * Options with long/int/None arguments
     */
    case LRO_PROXYPORT:
    case LRO_MAXSPEED:
    case LRO_CONNECTTIMEOUT:
    case LRO_MAXMIRRORTRIES:
    {
        long d;

        if (PyInt_Check(obj))
            d = PyInt_AS_LONG(obj);
        else if (PyLong_Check(obj))
            d = PyLong_AsLong(obj);
        else if (obj == Py_None) {
            /* Default options */
            if (option == LRO_PROXYPORT)
                d = 1080;
            else if (option == LRO_MAXSPEED || option == LRO_MAXMIRRORTRIES)
                d = 0;
            else if (option == LRO_CONNECTTIMEOUT)
                d = 300;
            else
                assert(0);
        } else {
            PyErr_SetString(PyExc_TypeError, "Only Int/Long/None is supported with this option");
            return NULL;
        }

        res = lr_handle_setopt(self->handle, (LrHandleOption)option, d);
        break;
    }

    /*
     * Options with array argument
     */
    case LRO_YUMDLIST:
    case LRO_YUMBLIST: {
        Py_ssize_t len = 0;

        if (!PyList_Check(obj) && obj != Py_None) {
            PyErr_SetString(PyExc_TypeError, "Only List or None type is supported with this option");
            return NULL;
        }

        if (obj == Py_None) {
            res = lr_handle_setopt(self->handle, (LrHandleOption)option, NULL);
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

        char *array[len+1];
        for (Py_ssize_t x = 0; x < len; x++) {
            PyObject *item = PyList_GetItem(obj, x);
            if (PyString_Check(item))
                array[x] = PyString_AsString(item);
            else
                array[x] = NULL;
        }
        array[len] = NULL;

        res = lr_handle_setopt(self->handle, (LrHandleOption)option, array);
        break;
    }

    case LRO_VARSUB: {
        Py_ssize_t len = 0;
        LrUrlVars *vars = NULL;

        if (!PyList_Check(obj) && obj != Py_None) {
            PyErr_SetString(PyExc_TypeError, "Only List of tuples or None type is supported with this option");
            return NULL;
        }

        if (obj == Py_None) {
            res = lr_handle_setopt(self->handle, (LrHandleOption)option, NULL);
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
            if (!PyString_Check(PyTuple_GetItem(item, 0)) ||
                (!PyString_Check(tuple_item) &&  tuple_item != Py_None)) {
                PyErr_SetString(PyExc_TypeError, "Bad list format");
                return NULL;
            }
        }

        for (Py_ssize_t x = 0; x < len; x++) {
            PyObject *item = PyList_GetItem(obj, x);
            PyObject *tuple_item;
            char *var, *val;

            tuple_item = PyTuple_GetItem(item, 0);
            var = PyString_AsString(tuple_item);

            tuple_item = PyTuple_GetItem(item, 1);
            val = (tuple_item == Py_None) ? NULL : PyString_AsString(tuple_item);

            vars = lr_urlvars_set(vars, var, val);
        }

        res = lr_handle_setopt(self->handle, (LrHandleOption)option, vars);
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
            res = lr_handle_setopt(self->handle, (LrHandleOption)option, NULL);
            if (res != LRE_OK)
                RETURN_ERROR(NULL, res, NULL);
        } else {
            // New callback object
            Py_XINCREF(obj);
            self->progress_cb = obj;
            res = lr_handle_setopt(self->handle, (LrHandleOption)option, progress_callback);
            if (res != LRE_OK)
                RETURN_ERROR(NULL, res, NULL);
            res = lr_handle_setopt(self->handle, LRO_PROGRESSDATA, self);
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

    /*
     * Unknown options
     */
    default:
        PyErr_SetString(PyExc_TypeError, "Unknown option");
        return NULL;
    }

    if (res != LRE_OK)
        RETURN_ERROR(NULL, res, NULL);
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

    /* char** options */
    case LRI_URL:
    case LRI_MIRRORLIST:
    case LRI_DESTDIR:
    case LRI_USERAGENT:
        res = lr_handle_getinfo(self->handle, (LrHandleInfoOption)option, &str);
        if (res != LRE_OK)
            RETURN_ERROR(NULL, res, NULL);
        if (str == NULL)
            Py_RETURN_NONE;
        return PyString_FromString(str);

    /* long* options */
    case LRI_UPDATE:
    case LRI_LOCAL:
    case LRI_REPOTYPE:
    case LRI_FETCHMIRRORS:
    case LRI_MAXMIRRORTRIES:
        res = lr_handle_getinfo(self->handle, (LrHandleInfoOption)option, &lval);
        if (res != LRE_OK)
            RETURN_ERROR(NULL, res, NULL);
        return PyLong_FromLong(lval);

    case LRI_VARSUB: {
        LrUrlVars *vars;
        PyObject *list;

        res = lr_handle_getinfo(self->handle, (LrHandleInfoOption)option, &vars);
        if (res != LRE_OK)
            RETURN_ERROR(NULL, res, NULL);

        if (vars == NULL)
            Py_RETURN_NONE;

        list = PyList_New(0);
        for (LrUrlVars *elem = vars; elem; elem = g_slist_next(elem)) {
            PyObject *tuple, *obj;
            LrVar *var = elem->data;

            tuple = PyTuple_New(2);
            obj = PyString_FromString(var->var);
            PyTuple_SetItem(tuple, 0, obj);

            if (var->val != NULL) {
                obj = PyString_FromString(var->val);
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
    case LRI_YUMDLIST:
    case LRI_YUMBLIST:
    case LRI_MIRRORS: {
        PyObject *list;
        char **strlist;
        res = lr_handle_getinfo(self->handle, (LrHandleInfoOption)option, &strlist);
        if (res != LRE_OK)
            RETURN_ERROR(NULL, res, NULL);
        if (strlist == NULL) {
            if (option == LRI_MIRRORS) {
                return PyList_New(0);
            } else {
                Py_RETURN_NONE;
            }
        }
        list = PyList_New(0);
        for (int x=0; strlist[x] != NULL; x++) {
            PyList_Append(list, PyString_FromString(strlist[x]));
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

    /* metalink */
    case LRI_METALINK: {
        PyObject *py_metalink;
        LrMetalink *metalink;
        res = lr_handle_getinfo(self->handle, (LrHandleInfoOption)option, &metalink);
        if (res != LRE_OK)
            RETURN_ERROR(NULL, res, NULL);
        if (metalink == NULL)
            Py_RETURN_NONE;
        py_metalink = PyObject_FromMetalink(metalink);
        return py_metalink;
    }

    default:
        PyErr_SetString(PyExc_TypeError, "Unknown option");
        return NULL;
    }

    if (res != LRE_OK)
        RETURN_ERROR(NULL, res, NULL);
    Py_RETURN_NONE;
}

static PyObject *
perform(_HandleObject *self, PyObject *args)
{
    PyObject *result_obj;
    LrResult *result;
    gboolean ret;
    GError *tmp_err = NULL;

    if (!PyArg_ParseTuple(args, "O:perform", &result_obj))
        return NULL;
    if (check_HandleStatus(self))
        return NULL;

    result = Result_FromPyObject(result_obj);

    ret = lr_handle_perform(self->handle, result, &tmp_err);
    assert((ret && !tmp_err) || (!ret && tmp_err));

    if (!ret && tmp_err->code == LRE_INTERRUPTED) {
        g_error_free(tmp_err);
        PyErr_SetInterrupt();
        PyErr_CheckSignals();
        return NULL;
    }

    if (!ret)
        RETURN_ERROR(&tmp_err, -1, NULL);

    Py_RETURN_NONE;
}

static PyObject *
download_package(_HandleObject *self, PyObject *args)
{
    gboolean ret;
    char *relative_url, *checksum, *dest, *base_url;
    int resume, checksum_type;
    GError *tmp_err = NULL;

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
                              checksum, base_url, resume, &tmp_err);

    assert((ret && !tmp_err) || (!ret && tmp_err));

    if (!ret && tmp_err->code == LRE_INTERRUPTED) {
        g_error_free(tmp_err);
        PyErr_SetInterrupt();
        PyErr_CheckSignals();
        return NULL;
    }

    if (!ret)
        RETURN_ERROR(&tmp_err, -1, NULL);

    Py_RETURN_NONE;
}

static PyObject *
download_packages(_HandleObject *self, PyObject *args)
{
    gboolean ret;
    PyObject *py_list;
    int failfast;
    LrPackageDownloadFlag flags = 0;
    GError *tmp_err = NULL;

    if (!PyArg_ParseTuple(args, "O!i:download_packages",
                          &PyList_Type, &py_list, &failfast))
        return NULL;

    if (check_HandleStatus(self))
        return NULL;

    // Convert python list to GSList
    GSList *list = NULL;
    Py_ssize_t len = PyList_Size(py_list);
    for (Py_ssize_t x=0; x < len; x++) {
        PyObject *py_packagetarget = PyList_GetItem(py_list, x);
        LrPackageTarget *target = PackageTarget_FromPyObject(py_packagetarget);
        if (!target)
            return NULL;
        list = g_slist_append(list, target);
    }

    Py_XINCREF(py_list);

    if (failfast)
        flags |= LR_PACKAGEDOWNLOAD_FAILFAST;

    ret = lr_download_packages(self->handle, list, flags, &tmp_err);

    assert((ret && !tmp_err) || (!ret && tmp_err));

    if (!ret && tmp_err->code == LRE_INTERRUPTED) {
        Py_XDECREF(py_list);
        g_error_free(tmp_err);
        PyErr_SetInterrupt();
        PyErr_CheckSignals();
        return NULL;
    }

    Py_XDECREF(py_list);

    if (!ret)
        RETURN_ERROR(&tmp_err, -1, NULL);

    Py_RETURN_NONE;
}

static struct
PyMethodDef handle_methods[] = {
    { "setopt", (PyCFunction)setopt, METH_VARARGS, NULL },
    { "getinfo", (PyCFunction)getinfo, METH_VARARGS, NULL },
    { "perform", (PyCFunction)perform, METH_VARARGS, NULL },
    { "download_package", (PyCFunction)download_package, METH_VARARGS, NULL },
    { "download_packages", (PyCFunction)download_packages, METH_VARARGS, NULL },
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
