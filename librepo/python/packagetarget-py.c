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

typedef struct {
    PyObject_HEAD
    LrPackageTarget *target;
    /* Callback */
    PyObject *progress_cb;
    PyObject *progress_cb_data;
    /* Handle */
    PyObject *handle;
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

    if (self->progress_cb_data)
        user_data = self->progress_cb_data;
    else
        user_data = Py_None;

    arglist = Py_BuildValue("(Odd)", user_data, total_to_download, now_downloaded);
    if (arglist == NULL)
        return 0;

    assert(self->handle);
    PyHandle_EndAllowThreads(self->handle);
    result = PyObject_CallObject(self->progress_cb, arglist);
    PyHandle_BeginAllowThreads(self->handle);

    Py_DECREF(arglist);
    Py_XDECREF(result);
    return 0;
}

void
PackageTarget_SetHandle(PyObject *o, PyObject *handle)
{
    _PackageTargetObject *self = (_PackageTargetObject *) o;
    Py_XDECREF(self->handle);
    if (!self) return;
    self->handle = handle;
    Py_XINCREF(self->handle);
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
        self->progress_cb = NULL;
        self->progress_cb_data = NULL;
        self->handle = NULL;
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
    PyObject *progresscb, *cbdata;
    LrProgressCb c_cb;
    GError *tmp_err = NULL;

    if (!PyArg_ParseTuple(args, "szizLziOO:packagetarget_init",
                          &relative_url, &dest, &checksum_type, &checksum,
                          &expectedsize, &base_url, &resume, &progresscb, &cbdata))
        return -1;

    if (!PyCallable_Check(progresscb) && progresscb != Py_None) {
        PyErr_SetString(PyExc_TypeError, "progresscb must be callable or None");
        return -1;
    }

    if (progresscb == Py_None) {
        c_cb = NULL;
    } else {
        c_cb = packagetarget_progress_callback;
        self->progress_cb = progresscb;
        self->progress_cb_data = cbdata;
        Py_XINCREF(self->progress_cb);
        Py_XINCREF(self->progress_cb_data);
    }

    self->target = lr_packagetarget_new(relative_url, dest, checksum_type,
                                        checksum, (gint64) expectedsize,
                                        base_url, resume, c_cb, self,
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
    Py_XDECREF(o->progress_cb);
    Py_XDECREF(o->progress_cb_data);
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

static PyGetSetDef packagetarget_getsetters[] = {
    {"relative_url",  (getter)get_str, NULL, NULL, OFFSET(relative_url)},
    {"dest",          (getter)get_str, NULL, NULL, OFFSET(dest)},
    {"base_url",      (getter)get_str, NULL, NULL, OFFSET(base_url)},
    {"checksum_type", (getter)get_int, NULL, NULL, OFFSET(checksum_type)},
    {"checksum",      (getter)get_str, NULL, NULL, OFFSET(checksum)},
    {"resume",        (getter)get_int, NULL, NULL, OFFSET(resume)},
//  {"progresscb",    (getter)get_str, NULL, NULL, OFFSET(progresscb)},
//  {"cbdata",        (getter)get_str, NULL, NULL, OFFSET(cbdata)},
    {"local_path",    (getter)get_str, NULL, NULL, OFFSET(local_path)},
    {"err",           (getter)get_str, NULL, NULL, OFFSET(err)},
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
