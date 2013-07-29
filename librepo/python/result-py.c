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

#include <glib.h>
#include <Python.h>
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
    int res = LRE_OK;

    if (!PyArg_ParseTuple(args, "i:getinfo", &option))
        return NULL;
    if (check_ResultStatus(self))
        return NULL;

    if (option < 0 || option >= LRR_SENTINEL) {
        PyErr_SetString(PyExc_TypeError, "Unknown option");
        return NULL;
    }

    switch (option) {

    /*
     * YUM related options
     */
    case LRR_YUM_REPO: {
        LrYumRepo *repo;
        res = lr_result_getinfo(self->result, (LrResultInfoOption)option, &repo);
        if (res != LRE_OK)
            RETURN_ERROR(NULL, res, NULL);
        return PyObject_FromYumRepo(repo);
    }

    case LRR_YUM_REPOMD: {
        LrYumRepoMd *repomd;
        res = lr_result_getinfo(self->result, (LrResultInfoOption)option, &repomd);
        if (res != LRE_OK)
            RETURN_ERROR(NULL, res, NULL);
        return PyObject_FromYumRepoMd(repomd);
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
    PyObject_HEAD_INIT(NULL)
    0,                              /* ob_size */
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
