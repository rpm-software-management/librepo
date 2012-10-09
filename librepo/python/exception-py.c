#include <Python.h>
#include "exception-py.h"

PyObject *LrErr_Exception = NULL;

int
init_exceptions()
{
    LrErr_Exception = PyErr_NewException("_librepo.Exception", NULL, NULL);
    if (!LrErr_Exception)
        return 0;
    Py_INCREF(LrErr_Exception);

    return 1;
}
