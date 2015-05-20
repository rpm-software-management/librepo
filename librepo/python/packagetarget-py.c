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

#include "handle-py.h"
#include "packagetarget-py.h"
#include "exception-py.h"
#include "downloader-py.h"
#include "packagedownloader-py.h"
#include "typeconversion.h"

typedef struct {
    PyObject_HEAD
    LrPackageTarget *target;
    /* Handle */
    PyObject *handle;
    /* Callback */
    PyObject *cb_data;
    PyObject *progress_cb;
    PyObject *end_cb;
    PyObject *mirrorfailure_cb;
    /* GIL Stuff */
    PyThreadState **state;
} _PackageTargetObject;

LrPackageTarget *
PackageTarget_FromPyObject(PyObject *o)
{
    if (!PackageTargetObject_Check(o)) {
        PyErr_SetString(PyExc_TypeError, "Expected a librepo.PackageTarget object.");
        return NULL;
    }
    return ((_PackageTargetObject *)o)->target;
}

static int
check_PackageTargetStatus(const _PackageTargetObject *self)
{
    assert(self != NULL);
    assert(PackageTargetObject_Check(self));
    if (self->target == NULL) {
        PyErr_SetString(LrErr_Exception, "No librepo target");
        return -1;
    }
    return 0;
}

/* Callback stuff */

static int
packagetarget_progress_callback(void *data, double total_to_download, double now_downloaded)
{
    int ret = LR_CB_OK; // Assume everything will be ok
    _PackageTargetObject *self;
    PyObject *user_data, *result;

    self = (_PackageTargetObject *)data;
    if (!self->progress_cb)
        return ret;

    if (self->cb_data)
        user_data = self->cb_data;
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

static int
packagetarget_end_callback(void *data,
                           LrTransferStatus status,
                           const char *msg)
{
    int ret = LR_CB_OK; // Assume everything will be ok
    _PackageTargetObject *self;
    PyObject *user_data, *result, *py_msg;

    self = (_PackageTargetObject *)data;
    if (!self->end_cb)
        return ret;

    if (self->cb_data)
        user_data = self->cb_data;
    else
        user_data = Py_None;

    py_msg = PyStringOrNone_FromString(msg);

    EndAllowThreads(self->state);
    result = PyObject_CallFunction(self->end_cb,
                                   "(OiO)", user_data, status, py_msg);
    Py_DECREF(py_msg);
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
            PyErr_SetString(PyExc_TypeError, "End callback must return integer number");
            ret = LR_CB_ERROR;
        }
    }

    Py_XDECREF(result);
    BeginAllowThreads(self->state);

    return ret;
}

static int
packagetarget_mirrorfailure_callback(void *data,
                                     const char *msg,
                                     const char *url)
{
    int ret = LR_CB_OK; // Assume everything will be ok
    _PackageTargetObject *self;
    PyObject *user_data, *result, *py_msg, *py_url;

    self = (_PackageTargetObject *)data;
    if (!self->mirrorfailure_cb)
        return ret;

    if (self->cb_data)
        user_data = self->cb_data;
    else
        user_data = Py_None;

    py_msg = PyStringOrNone_FromString(msg);
    py_url = PyStringOrNone_FromString(url);

    EndAllowThreads(self->state);
    result = PyObject_CallFunction(self->mirrorfailure_cb,
                                   "(OOO)", user_data, py_msg, py_url);

    Py_DECREF(py_msg);
    Py_DECREF(py_url);

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
            PyErr_SetString(PyExc_TypeError, "Mirror failure callback must return integer number");
            ret = LR_CB_ERROR;
        }
    }

    Py_XDECREF(result);
    BeginAllowThreads(self->state);

    return ret;
}

void
PackageTarget_SetThreadState(PyObject *o, PyThreadState **state)
{
    _PackageTargetObject *self = (_PackageTargetObject *) o;
    if (!self) return;
    self->state = state;

    // XXX: Little tricky (but not so much)
    // Set the state to the depending handle as well
    // (Needed when fastestmirrorcb is used)
    if (self->handle) {
        Handle_SetThreadState(self->handle, state);
    }
}


/* Function on the type */

static PyObject *
packagetarget_new(PyTypeObject *type,
           G_GNUC_UNUSED PyObject *args,
           G_GNUC_UNUSED PyObject *kwds)
{
    _PackageTargetObject *self = (_PackageTargetObject *)type->tp_alloc(type, 0);

    if (self) {
        self->target = NULL;
        self->handle = NULL;
        self->cb_data = NULL;
        self->progress_cb = NULL;
        self->end_cb = NULL;
        self->mirrorfailure_cb = NULL;
        self->state = NULL;
    }
    return (PyObject *)self;
}

static int
packagetarget_init(_PackageTargetObject *self,
                   PyObject *args,
                   PyObject *kwds G_GNUC_UNUSED)
{
    char *relative_url, *dest, *checksum, *base_url;
    int checksum_type, resume;
    PY_LONG_LONG expectedsize, byterangestart, byterangeend;
    PyObject *pyhandle, *py_progresscb, *py_cbdata;
    PyObject *py_endcb, *py_mirrorfailurecb;
    LrProgressCb progresscb = NULL;
    LrEndCb endcb = NULL;
    LrMirrorFailureCb mirrorfailurecb = NULL;
    LrHandle *handle = NULL;
    GError *tmp_err = NULL;
    PyObject *py_dest = NULL;
    PyObject *tmp_py_str = NULL;

    if (!PyArg_ParseTuple(args, "OsOizLziOOOOLL:packagetarget_init",
                          &pyhandle, &relative_url, &py_dest, &checksum_type,
                          &checksum, &expectedsize, &base_url, &resume,
                          &py_progresscb, &py_cbdata, &py_endcb,
                          &py_mirrorfailurecb, &byterangestart,
                          &byterangeend))
        return -1;

    dest = PyAnyStr_AsString(py_dest, &tmp_py_str);

    if (pyhandle != Py_None) {
        handle = Handle_FromPyObject(pyhandle);
        if (!handle)
            return -1;
        self->handle = pyhandle;
        Py_INCREF(self->handle);
    }

    if (!PyCallable_Check(py_progresscb) && py_progresscb != Py_None) {
        PyErr_SetString(PyExc_TypeError, "progresscb must be callable or None");
        return -1;
    }

    if (!PyCallable_Check(py_endcb) && py_endcb != Py_None) {
        PyErr_SetString(PyExc_TypeError, "endcb must be callable or None");
        return -1;
    }

    if (!PyCallable_Check(py_mirrorfailurecb) && py_mirrorfailurecb != Py_None) {
        PyErr_SetString(PyExc_TypeError, "mirrorfailurecb must be callable or None");
        return -1;
    }

    if (py_cbdata) {
        self->cb_data = py_cbdata;
        Py_XINCREF(self->cb_data);
    }

    if (py_progresscb != Py_None) {
        progresscb = packagetarget_progress_callback;
        self->progress_cb = py_progresscb;
        Py_XINCREF(self->progress_cb);
    }

    if (py_endcb != Py_None) {
        endcb = packagetarget_end_callback;
        self->end_cb = py_endcb;
        Py_XINCREF(self->end_cb);
    }

    if (py_mirrorfailurecb != Py_None) {
        mirrorfailurecb = packagetarget_mirrorfailure_callback;
        self->mirrorfailure_cb = py_mirrorfailurecb;
        Py_XINCREF(self->mirrorfailure_cb);
    }

    if (resume && byterangestart) {
        PyErr_SetString(PyExc_TypeError, "resume cannot be used simultaneously "
                "with the byterangestart param");
        return -1;
    }

    self->target = lr_packagetarget_new_v3(handle, relative_url, dest,
                                           checksum_type, checksum,
                                           (gint64) expectedsize, base_url,
                                           resume, progresscb, self, endcb,
                                           mirrorfailurecb,
                                           (gint64) byterangestart,
                                           (gint64) byterangeend,
                                           &tmp_err);
    Py_XDECREF(tmp_py_str);

    if (self->target == NULL) {
        PyErr_Format(LrErr_Exception,
                     "PackageTarget initialization failed: %s",
                     tmp_err->message);
        g_error_free(tmp_err);
        return -1;
    }
    return 0;
}

static void
packagetarget_dealloc(_PackageTargetObject *o)
{
    if (o->target)
        lr_packagetarget_free(o->target);
    Py_XDECREF(o->cb_data);
    Py_XDECREF(o->progress_cb);
    Py_XDECREF(o->end_cb);
    Py_XDECREF(o->mirrorfailure_cb);
    Py_XDECREF(o->handle);
    Py_TYPE(o)->tp_free(o);
}

static struct
PyMethodDef packagetarget_methods[] = {
    { NULL }
};

// Get/setters
#define OFFSET(member) (void *) offsetof(LrPackageTarget, member)

static PyObject *
get_gint64(_PackageTargetObject *self, void *member_offset)
{
    if (check_PackageTargetStatus(self))
        return NULL;
    LrPackageTarget *target = self->target;
    gint64 val = *((gint64 *) ((size_t)target + (size_t) member_offset));
    return PyLong_FromLongLong((PY_LONG_LONG) val);
}

static PyObject *
get_int(_PackageTargetObject *self, void *member_offset)
{
    if (check_PackageTargetStatus(self))
        return NULL;
    LrPackageTarget *target = self->target;
    int val = *((int *) ((size_t)target + (size_t) member_offset));
    return PyLong_FromLong((long) val);
}

static PyObject *
get_str(_PackageTargetObject *self, void *member_offset)
{
    if (check_PackageTargetStatus(self))
        return NULL;
    LrPackageTarget *target = self->target;
    char *str = *((char **) ((size_t) target + (size_t) member_offset));
    if (str == NULL)
        Py_RETURN_NONE;
    return PyStringOrNone_FromString(str);
}

static PyObject *
get_pythonobj(_PackageTargetObject *self, void *member_offset)
{
    if (check_PackageTargetStatus(self))
        return NULL;

    if (member_offset == OFFSET(handle)) {
        if (!self->handle)
            Py_RETURN_NONE;
        Py_XINCREF(self->handle);
        return self->handle;
    }

    if (member_offset == OFFSET(cbdata)) {
        if (!self->cb_data)
            Py_RETURN_NONE;
        Py_XINCREF(self->cb_data);
        return self->cb_data;
    }

    if (member_offset == OFFSET(progresscb)) {
        if (!self->progress_cb)
            Py_RETURN_NONE;
        Py_XINCREF(self->progress_cb);
        return self->progress_cb;
    }

    if (member_offset == OFFSET(endcb)) {
        if (!self->end_cb)
            Py_RETURN_NONE;
        Py_XINCREF(self->end_cb);
        return self->end_cb;
    }

    if (member_offset == OFFSET(mirrorfailurecb)) {
        if (!self->mirrorfailure_cb)
            Py_RETURN_NONE;
        Py_XINCREF(self->mirrorfailure_cb);
        return self->mirrorfailure_cb;
    }

    Py_RETURN_NONE;
}

static PyGetSetDef packagetarget_getsetters[] = {
    {"handle",        (getter)get_pythonobj, NULL, NULL, OFFSET(handle)},
    {"relative_url",  (getter)get_str,       NULL, NULL, OFFSET(relative_url)},
    {"dest",          (getter)get_str,       NULL, NULL, OFFSET(dest)},
    {"base_url",      (getter)get_str,       NULL, NULL, OFFSET(base_url)},
    {"checksum_type", (getter)get_int,       NULL, NULL, OFFSET(checksum_type)},
    {"checksum",      (getter)get_str,       NULL, NULL, OFFSET(checksum)},
    {"expectedsize",  (getter)get_gint64,    NULL, NULL, OFFSET(expectedsize)},
    {"resume",        (getter)get_int,       NULL, NULL, OFFSET(resume)},
    {"cbdata",        (getter)get_pythonobj, NULL, NULL, OFFSET(cbdata)},
    {"progresscb",    (getter)get_pythonobj, NULL, NULL, OFFSET(progresscb)},
    {"endcb",         (getter)get_pythonobj, NULL, NULL, OFFSET(endcb)},
    {"mirrorfailurecb",(getter)get_pythonobj,NULL, NULL, OFFSET(mirrorfailurecb)},
    {"local_path",    (getter)get_str,       NULL, NULL, OFFSET(local_path)},
    {"err",           (getter)get_str,       NULL, NULL, OFFSET(err)},
    {NULL, NULL, NULL, NULL, NULL} /* sentinel */
};

// Object definition

PyTypeObject PackageTarget_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_librepo.PackageTarget",       /* tp_name */
    sizeof(_PackageTargetObject),   /* tp_basicsize */
    0,                              /* tp_itemsize */
    (destructor) packagetarget_dealloc,/* tp_dealloc */
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
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,/* tp_flags */
    "PackageTarget object",         /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    PyObject_SelfIter,              /* tp_iter */
    0,                              /* tp_iternext */
    packagetarget_methods,          /* tp_methods */
    0,                              /* tp_members */
    packagetarget_getsetters,       /* tp_getset */
    0,                              /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    (initproc) packagetarget_init,  /* tp_init */
    0,                              /* tp_alloc */
    packagetarget_new,              /* tp_new */
    0,                              /* tp_free */
    0,                              /* tp_is_gc */
};
