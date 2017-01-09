//
// Created by Martin Hatina on 24/11/2016.
//

#ifndef LIBREPO_METADATATARGET_PY_H
#define LIBREPO_METADATATARGET_PY_H

#include <Python.h>
#include <librepo/librepo.h>

extern PyTypeObject MetadataTarget_Type;

#define MetadataTargetObject_Check(o)   PyObject_TypeCheck(o, &MetadataTarget_Type)

LrMetadataTarget *MetadataTarget_FromPyObject(PyObject *o);
void MetadataTarget_SetThreadState(PyObject *o, PyThreadState **state);


#endif //LIBREPO_METADATATARGET_PY_H
