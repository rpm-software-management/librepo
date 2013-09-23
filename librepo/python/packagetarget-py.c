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

#include "handle-py.h"
#include "packagetarget-py.h"
#include "exception-py.h"
#include "packagedownloader-py.h"

typedef struct {
    PyObject_HEAD
    LrPackageTarget *target;
    /* Handle */
    PyObject *handle;
    /* Callback */
    PyObject *cb_data;
    PyObject *progress_cb;
    PyObject *end_cb;
    PyObject *failure_cb;
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
    _PackageTargetObject *self;
    PyObject *user_data, *arglist, *result;

    self = (_PackageTargetObject *)data;
    if (!self->progress_cb)
        return 0;

    if (self->cb_data)
        user_data = self->cb_data;
    else
        user_data = Py_None;

    arglist = Py_BuildValue("(Odd)", user_data, total_to_download, now_downloaded);
    if (arglist == NULL)
        return 0;

    assert(self->handle);
    EndAllowThreads(self->state);
    result = PyObject_CallObject(self->progress_cb, arglist);
    BeginAllowThreads(self->state);

    Py_DECREF(arglist);
    Py_XDECREF(result);
    return 0;
}

static void
packagetarget_end_callback(void *data)
{
    _PackageTargetObject *self;
    PyObject *user_data, *arglist, *result;

    self = (_PackageTargetObject *)data;
    if (!self->end_cb)
        return;

    if (self->cb_data)
        user_data = self->cb_data;
    else
        user_data = Py_None;

    arglist = Py_BuildValue("(O)", user_data);
    if (arglist == NULL)
        return;

    assert(self->handle);
    EndAllowThreads(self->state);
    result = PyObject_CallObject(self->end_cb, arglist);
    BeginAllowThreads(self->state);

    Py_DECREF(arglist);
    Py_XDECREF(result);
    return;
}

static int
packagetarget_failure_callback(void *data, const char *msg)
{
    _PackageTargetObject *self;
    PyObject *user_data, *arglist, *result;

    self = (_PackageTargetObject *)data;
    if (!self->failure_cb)
        return 0;

    if (self->cb_data)
        user_data = self->cb_data;
    else
        user_data = Py_None;

    arglist = Py_BuildValue("(Os)", user_data, msg);
    if (arglist == NULL)
        return 0;

    assert(self->handle);
    EndAllowThreads(self->state);
    result = PyObject_CallObject(self->failure_cb, arglist);
    BeginAllowThreads(self->state);

    Py_DECREF(arglist);
    Py_XDECREF(result);
    return 0;
}

static int
packagetarget_mirrorfailure_callback(void *data, const char *msg)
{
    _PackageTargetObject *self;
    PyObject *user_data, *arglist, *result;

    self = (_PackageTargetObject *)data;
    if (!self->mirrorfailure_cb)
        return 0;

    if (self->cb_data)
        user_data = self->cb_data;
    else
        user_data = Py_None;

    arglist = Py_BuildValue("(Os)", user_data, msg);
    if (arglist == NULL)
        return 0;

    assert(self->handle);
    EndAllowThreads(self->state);
    result = PyObject_CallObject(self->mirrorfailure_cb, arglist);
    BeginAllowThreads(self->state);

    Py_DECREF(arglist);
    Py_XDECREF(result);
    return 0;
}

void
PackageTarget_SetThreadState(PyObject *o, PyThreadState **state)
{
    _PackageTargetObject *self = (_PackageTargetObject *) o;
    if (!self) return;
    self->state = state;
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
        self->failure_cb = NULL;
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
    PY_LONG_LONG expectedsize;
    PyObject *pyhandle, *py_progresscb, *py_cbdata;
    PyObject *py_endcb, *py_failurecb, *py_mirrorfailurecb;
    LrProgressCb progresscb = NULL;
    LrEndCb endcb = NULL;
    LrFailureCb failurecb = NULL;
    LrMirrorFailureCb mirrorfailurecb = NULL;
    LrHandle *handle = NULL;
    GError *tmp_err = NULL;

    if (!PyArg_ParseTuple(args, "OszizLziOOOOO:packagetarget_init",
                          &pyhandle, &relative_url, &dest, &checksum_type,
                          &checksum, &expectedsize, &base_url, &resume,
                          &py_progresscb, &py_cbdata, &py_endcb,
                          &py_failurecb, &py_mirrorfailurecb))
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

    if (!PyCallable_Check(py_endcb) && py_endcb != Py_None) {
        PyErr_SetString(PyExc_TypeError, "endcb must be callable or None");
        return -1;
    }

    if (!PyCallable_Check(py_failurecb) && py_failurecb != Py_None) {
        PyErr_SetString(PyExc_TypeError, "failurecb must be callable or None");
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

    if (py_failurecb != Py_None) {
        failurecb = packagetarget_failure_callback;
        self->failure_cb = py_failurecb;
        Py_XINCREF(self->failure_cb);
    }

    if (py_mirrorfailurecb != Py_None) {
        mirrorfailurecb = packagetarget_mirrorfailure_callback;
        self->mirrorfailure_cb = py_mirrorfailurecb;
        Py_XINCREF(self->mirrorfailure_cb);
    }

    self->target = lr_packagetarget_new_v2(handle, relative_url, dest,
                                           checksum_type, checksum,
                                           (gint64) expectedsize, base_url,
                                           resume, progresscb, self, endcb,
                                           failurecb, mirrorfailurecb,
                                           &tmp_err);

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
    Py_XDECREF(o->failure_cb);
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
    return PyBytes_FromString(str);
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

    if (member_offset == OFFSET(failurecb)) {
        if (!self->failure_cb)
            Py_RETURN_NONE;
        Py_XINCREF(self->failure_cb);
        return self->failure_cb;
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
    {"failurecb",     (getter)get_pythonobj, NULL, NULL, OFFSET(failurecb)},
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
