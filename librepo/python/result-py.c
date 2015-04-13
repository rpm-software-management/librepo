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
#undef NDEBUG
#include <assert.h>

#include "librepo/librepo.h"

#include "typeconversion.h"
#include "exception-py.h"
#include "result-py.h"

typedef struct {
    PyObject_HEAD
    LrResult *result;
} _ResultObject;

LrResult *
Result_FromPyObject(PyObject *o)
{
    if (!ResultObject_Check(o)) {
        PyErr_SetString(PyExc_TypeError, "Expected a _librepo.Result object.");
        return NULL;
    }
    return ((_ResultObject *)o)->result;
}

/*
int
result_converter(PyObject *o, LrHandle *handle_ptr)
{
    LrHandle handle = handleFromPyObject(o);
    if (handle == NULL)
        return 0;
    *handle_ptr = handle;
    return 1;
}
*/

static int
check_ResultStatus(const _ResultObject *self)
{
    assert(self != NULL);
    assert(ResultObject_Check(self));
    if (self->result == NULL) {
        PyErr_SetString(LrErr_Exception, "No librepo result");
        return -1;
    }
    return 0;
}

/* Function on the type */

static PyObject *
result_new(PyTypeObject *type,
           G_GNUC_UNUSED PyObject *args,
           G_GNUC_UNUSED PyObject *kwds)
{
    _ResultObject *self = (_ResultObject *)type->tp_alloc(type, 0);
    if (self)
        self->result = NULL;
    return (PyObject *)self;
}

static int
result_init(_ResultObject *self, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|", kwlist))
        return -1;

    self->result = lr_result_init();
    if (self->result == NULL) {
        PyErr_SetString(LrErr_Exception, "Result initialization failed");
        return -1;
    }
    return 0;
}

static void
result_dealloc(_ResultObject *o)
{
    if (o->result)
        lr_result_free(o->result);
    Py_TYPE(o)->tp_free(o);
}

static PyObject *
getinfo(_ResultObject *self, PyObject *args)
{
    int option;
    gboolean res = TRUE;

    if (!PyArg_ParseTuple(args, "i:getinfo", &option))
        return NULL;
    if (check_ResultStatus(self))
        return NULL;

    switch (option) {

    /*
     * YUM related options
     */
    case LRR_YUM_REPO: {
        LrYumRepo *repo;
        GError *tmp_err = NULL;
        res = lr_result_getinfo(self->result,
                                &tmp_err,
                                (LrResultInfoOption)option,
                                &repo);
        if (!res)
            RETURN_ERROR(&tmp_err, -1, NULL);
        return PyObject_FromYumRepo(repo);
    }

    case LRR_YUM_REPOMD: {
        LrYumRepoMd *repomd;
        GError *tmp_err = NULL;
        res = lr_result_getinfo(self->result,
                                &tmp_err,
                                (LrResultInfoOption)option,
                                &repomd);
        if (!res)
            RETURN_ERROR(&tmp_err, -1, NULL);
        return PyObject_FromYumRepoMd(repomd);
    }

    case LRR_RPMMD_TIMESTAMP:
    case LRR_YUM_TIMESTAMP: {
        gint64 ts;
        GError *tmp_err = NULL;
        res = lr_result_getinfo(self->result,
                                &tmp_err,
                                (LrResultInfoOption)option,
                                &ts);
        if (!res)
            RETURN_ERROR(&tmp_err, -1, NULL);
        return PyLong_FromLongLong((PY_LONG_LONG) ts);
    }

    case LRR_RPMMD_REPO: {
        LrYumRepo *repo;
        GError *tmp_err = NULL;
        res = lr_result_getinfo(self->result,
                                &tmp_err,
                                (LrResultInfoOption)option,
                                &repo);
        if (!res)
            RETURN_ERROR(&tmp_err, -1, NULL);
        return PyObject_FromYumRepo_v2(repo);
    }

    case LRR_RPMMD_REPOMD: {
        LrYumRepoMd *repomd;
        GError *tmp_err = NULL;
        res = lr_result_getinfo(self->result,
                                &tmp_err,
                                (LrResultInfoOption)option,
                                &repomd);
        if (!res)
            RETURN_ERROR(&tmp_err, -1, NULL);
        PyObject *obj = PyObject_FromYumRepoMd_v2(repomd);
        return obj;
    }

    /*
     * Unknown options
     */
    default:
        PyErr_Format(PyExc_ValueError, "Unknown option (%d)", option);
        return NULL;
    }

    assert(res);
    Py_RETURN_NONE;
}

static PyObject *
clear(_ResultObject *self, G_GNUC_UNUSED PyObject *noarg)
{
    if (check_ResultStatus(self))
        return NULL;
    lr_result_clear(self->result);
    Py_RETURN_NONE;
}

static struct
PyMethodDef result_methods[] = {
    { "getinfo", (PyCFunction)getinfo, METH_VARARGS, NULL },
    { "clear", (PyCFunction)clear, METH_NOARGS, NULL },
    { NULL }
};

PyTypeObject Result_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_librepo.Result",              /* tp_name */
    sizeof(_ResultObject),          /* tp_basicsize */
    0,                              /* tp_itemsize */
    (destructor) result_dealloc,    /* tp_dealloc */
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
    "Result object",                /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    PyObject_SelfIter,              /* tp_iter */
    0,                              /* tp_iternext */
    result_methods,                 /* tp_methods */
    0,                              /* tp_members */
    0,                              /* tp_getset */
    0,                              /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    (initproc) result_init,         /* tp_init */
    0,                              /* tp_alloc */
    result_new,                     /* tp_new */
    0,                              /* tp_free */
    0,                              /* tp_is_gc */
};
