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

#include <err.h>

#include "librepo/librepo.h"
#include "librepo/downloader_internal.h"
#include "metadatatarget-py.h"
#include "handle-py.h"
#include "typeconversion.h"
#include "exception-py.h"
#include "downloader-py.h"

typedef struct {
    PyObject_HEAD
    LrMetadataTarget *target;
    /* Handle */
    PyObject *handle;
    /* Callback */
    PyObject *cb_data;
    PyObject *progress_cb;
    PyObject *mirrorfailure_cb;
    PyObject *end_cb;
    /* GIL Stuff */
    PyThreadState **state;
} _MetadataTargetObject;

LrMetadataTarget *
MetadataTarget_FromPyObject(PyObject *o)
{
    if (!MetadataTargetObject_Check(o)) {
        PyErr_SetString(PyExc_TypeError, "Expected a librepo.MetadataTarget object.");
        return NULL;
    }
    return ((_MetadataTargetObject *)o)->target;
}

void
MetadataTarget_SetThreadState(PyObject *o, PyThreadState **state)
{
    _MetadataTargetObject *self = (_MetadataTargetObject *) o;
    if (!self) return;
    self->state = state;

    if (self->handle) {
        Handle_SetThreadState(self->handle, state);
    }
}

static int
check_MetadataTargetStatus(const _MetadataTargetObject *self)
{
    assert(self != NULL);
    assert(MetadataTargetObject_Check(self));
    if (self->target == NULL) {
        PyErr_SetString(LrErr_Exception, "No librepo target");
        return -1;
    }
    return 0;
}

static int
metadatatarget_progress_callback(void *data, double total_to_download, double now_downloaded)
{
    int ret = LR_CB_OK;
    _MetadataTargetObject *self;
    PyObject *user_data, *result;
    LrCallbackData *callback_data = (LrCallbackData *) data;
    CbData *cbdata = (CbData *) callback_data->userdata;

    self = (_MetadataTargetObject *)cbdata->cbdata;
    if (!self || !self->progress_cb)
        return ret;

    if (self->cb_data)
        user_data = self->cb_data;
    else
        user_data = Py_None;

    EndAllowThreads(self->state);
    result = PyObject_CallFunction(self->progress_cb,
                                   "(Odd)", user_data, total_to_download, now_downloaded);

    if (!result) {
        ret = LR_CB_ERROR;
    } else {
        if (result == Py_None) {
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
metadatatarget_mirrorfailure_callback(void *data,
                                      const char *msg,
                                      const char *url)
{
    int ret = LR_CB_OK; // Assume everything will be ok
    _MetadataTargetObject *self;
    PyObject *user_data, *result, *py_msg, *py_url;
    LrCallbackData *callback_data = (LrCallbackData *) data;
    CbData *cbdata = (CbData *) callback_data->userdata;

    self = (_MetadataTargetObject *)cbdata->cbdata;
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

static int
metadatatarget_end_callback(void *data,
                           LrTransferStatus status,
                           const char *msg)
{
    int ret = LR_CB_OK; // Assume everything will be ok
    _MetadataTargetObject *self;
    PyObject *user_data, *result, *py_msg;
    LrCallbackData *callback_data = (LrCallbackData *) data;
    CbData *cbdata = (CbData *) callback_data->userdata;

    self = (_MetadataTargetObject *)cbdata->cbdata;

    LrMetadataTarget *target = self->target;
    target->repomd_records_downloaded++;

    if (target->repomd_records_to_download != target->repomd_records_downloaded)
        return ret;
    else if (!self->end_cb)
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

static PyObject *
metadatatarget_new(PyTypeObject *type,
                  G_GNUC_UNUSED PyObject *args,
                  G_GNUC_UNUSED PyObject *kwds)
{
    _MetadataTargetObject *self = (_MetadataTargetObject *)type->tp_alloc(type, 0);

    if (self) {
        self->target = NULL;
        self->handle = NULL;
        self->cb_data = NULL;
        self->progress_cb = NULL;
        self->mirrorfailure_cb = NULL;
        self->end_cb = NULL;
        self->state = NULL;
    }
    return (PyObject *)self;
}

static int
metadatatarget_init(_MetadataTargetObject *self,
                    PyObject *args,
                    PyObject *kwds G_GNUC_UNUSED)
{
    const char *gnupghomedir;
    PyObject *pyhandle, *py_cbdata;
    PyObject *py_endcb, *py_progresscb;
    PyObject *py_mirrorfailurecb;
    LrEndCb endcb = NULL;
    LrHandle *handle = NULL;
    LrProgressCb progresscb = NULL;
    LrMirrorFailureCb mirrorfailurecb = NULL;
    GError *tmp_err = NULL;

    if (!PyArg_ParseTuple(args, "OOOOOs:metadatatarget_init",
                          &pyhandle, &py_cbdata, &py_progresscb,
                          &py_mirrorfailurecb, &py_endcb, &gnupghomedir))
        return -1;

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

    if (!PyCallable_Check(py_mirrorfailurecb) && py_mirrorfailurecb != Py_None) {
        PyErr_SetString(PyExc_TypeError, "mirrorfailurecb must be callable or None");
        return -1;
    }

    if (!PyCallable_Check(py_endcb) && py_endcb != Py_None) {
        PyErr_SetString(PyExc_TypeError, "endcb must be callable or None");
        return -1;
    }

    if (py_cbdata) {
        self->cb_data = py_cbdata;
        Py_XINCREF(self->cb_data);
    }

    if (py_progresscb != Py_None) {
        progresscb = metadatatarget_progress_callback;
        self->progress_cb = py_progresscb;
        Py_XINCREF(self->progress_cb);
    }

    if (py_mirrorfailurecb != Py_None) {
        mirrorfailurecb = metadatatarget_mirrorfailure_callback;
        self->mirrorfailure_cb = py_mirrorfailurecb;
        Py_XINCREF(self->mirrorfailure_cb);
    }

    if (py_endcb != Py_None) {
        endcb = metadatatarget_end_callback;
        self->end_cb = py_endcb;
        Py_XINCREF(self->end_cb);
    }

    self->target = lr_metadatatarget_new2(handle, self, progresscb, mirrorfailurecb, endcb, gnupghomedir, &tmp_err);

    if (self->target == NULL) {
        PyErr_Format(LrErr_Exception,
                     "MetadataTarget initialization failed: %s",
                     tmp_err->message);
        g_error_free(tmp_err);
        return -1;
    }
    return 0;
}

static void
metadatatarget_dealloc(_MetadataTargetObject *o)
{
    if (o->target)
        lr_metadatatarget_free(o->target);
    Py_XDECREF(o->progress_cb);
    Py_XDECREF(o->mirrorfailure_cb);
    Py_XDECREF(o->cb_data);
    Py_XDECREF(o->handle);
    Py_TYPE(o)->tp_free(o);
}

// Get/setters
#define OFFSET(member) (void *) offsetof(LrMetadataTarget, member)

static PyObject *
get_pythonobj(_MetadataTargetObject *self, void *member_offset)
{
    if (check_MetadataTargetStatus(self))
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

    if (member_offset == OFFSET(err)) {
        LrMetadataTarget *target = self->target;

        if (!target->err)
            Py_RETURN_NONE;

        int i = 0;
        PyObject *pylist = PyTuple_New(g_list_length(target->err));
        for (GList *elem = target->err; elem; i++, elem = g_list_next(elem)) {
            gchar *error_message = (gchar *) elem->data;
            PyObject *str = PyStringOrNone_FromString(error_message);
            PyTuple_SetItem(pylist, i, str);
        }

        Py_XINCREF(target->err);
        return pylist;
    }

    Py_RETURN_NONE;
}

static PyGetSetDef metadatatarget_getsetters[] = {
    {"handle",        (getter)get_pythonobj, NULL, NULL, OFFSET(handle)},
    {"cbdata",        (getter)get_pythonobj, NULL, NULL, OFFSET(cbdata)},
    {"progresscb",    (getter)get_pythonobj, NULL, NULL, OFFSET(progresscb)},
    {"mirrorfailurecb",(getter)get_pythonobj,NULL, NULL, OFFSET(mirrorfailurecb)},
    {"endcb",         (getter)get_pythonobj, NULL, NULL, OFFSET(endcb)},
    {"err",           (getter)get_pythonobj, NULL, NULL, OFFSET(err)},
    {NULL, NULL, NULL, NULL, NULL} /* sentinel */
};

static struct
PyMethodDef metadatatarget_methods[] = {
    { NULL }
};

PyTypeObject MetadataTarget_Type = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "_librepo.MetadataTarget",      /* tp_name */
        sizeof(_MetadataTargetObject),   /* tp_basicsize */
        0,                              /* tp_itemsize */
        (destructor) metadatatarget_dealloc,/* tp_dealloc */
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
        "MetadataTarget object",         /* tp_doc */
        0,                              /* tp_traverse */
        0,                              /* tp_clear */
        0,                              /* tp_richcompare */
        0,                              /* tp_weaklistoffset */
        PyObject_SelfIter,              /* tp_iter */
        0,                              /* tp_iternext */
        metadatatarget_methods,          /* tp_methods */
        0,                              /* tp_members */
        metadatatarget_getsetters,       /* tp_getset */
        0,                              /* tp_base */
        0,                              /* tp_dict */
        0,                              /* tp_descr_get */
        0,                              /* tp_descr_set */
        0,                              /* tp_dictoffset */
        (initproc) metadatatarget_init,  /* tp_init */
        0,                              /* tp_alloc */
        metadatatarget_new,              /* tp_new */
        0,                              /* tp_free */
        0,                              /* tp_is_gc */
};
