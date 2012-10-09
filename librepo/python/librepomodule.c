#include <Python.h>

#include "librepo/librepo.h"

#include "exception-py.h"

static PyObject *
py_global_init(PyObject *self, PyObject *noarg)
{
    lr_global_init();
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
py_global_cleanup(PyObject *self, PyObject *noarg)
{
    lr_global_cleanup();
    Py_INCREF(Py_None);
    return Py_None;
}

static struct PyMethodDef librepo_methods[] = {
    { "global_init",    (PyCFunction) py_global_init,
      METH_NOARGS, NULL },
    { "global_cleanup", (PyCFunction)py_global_cleanup,
      METH_NOARGS, NULL },
    { NULL }
};

PyMODINIT_FUNC
init_librepo(void)
{
    PyObject *m = Py_InitModule("_librepo", librepo_methods);
    if (!m)
        return;

    if (!init_exceptions())
        return;
    PyModule_AddObject(m, "Exception", LrErr_Exception);
}
