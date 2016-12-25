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
#include "typeconversion.h"

PyObject *
PyStringOrNone_FromString(const char *str)
{
    if (str == NULL)
        Py_RETURN_NONE;
    return PyUnicode_FromString(str);
}

/**
 * Set an object to a dict and decref its ref count.
 */
static int
PyDict_SetItemStringAndDecref(PyObject *p, const char *key, PyObject *val)
{
    int ret = PyDict_SetItemString(p, key, val);
    Py_XDECREF(val);
    return ret;
}

/**
 * bytes, basic string or unicode string in Python 2/3 to c string converter,
 * you need to call Py_XDECREF(tmp_py_str) after usage of returned string
 */
char *
PyAnyStr_AsString(PyObject *str, PyObject **tmp_py_str)
{
    char *res = NULL;
    if (PyUnicode_Check(str)) {
        *tmp_py_str = PyUnicode_AsUTF8String(str);
        res = PyBytes_AsString(*tmp_py_str);
    }
#if PY_MAJOR_VERSION < 3
    else if (PyString_Check(str))
        res = PyString_AsString(str);
#else
    else if (PyBytes_Check(str))
        res = PyBytes_AsString(str);
#endif

    return res;
}

PyObject *
PyObject_FromYumRepo(LrYumRepo *repo)
{
    PyObject *dict;

    if (!repo)
        Py_RETURN_NONE;

    if ((dict = PyDict_New()) == NULL)
        return NULL;

    PyDict_SetItemStringAndDecref(dict, "repomd",
            PyStringOrNone_FromString(repo->repomd));
    PyDict_SetItemStringAndDecref(dict, "url",
            PyStringOrNone_FromString(repo->url));
    PyDict_SetItemStringAndDecref(dict, "destdir",
            PyStringOrNone_FromString(repo->destdir));
    PyDict_SetItemStringAndDecref(dict, "signature",
            PyStringOrNone_FromString(repo->signature));
    PyDict_SetItemStringAndDecref(dict, "mirrorlist",
            PyStringOrNone_FromString(repo->mirrorlist));
    PyDict_SetItemStringAndDecref(dict, "metalink",
            PyStringOrNone_FromString(repo->metalink));

    for (GSList *elem = repo->paths; elem; elem = g_slist_next(elem)) {
        LrYumRepoPath *yumrepopath = elem->data;
        if (!yumrepopath || !yumrepopath->type) continue;
        PyDict_SetItemStringAndDecref(dict,
                             yumrepopath->type,
                             PyStringOrNone_FromString(yumrepopath->path));
    }

    return dict;
}

PyObject *
PyObject_FromYumRepo_v2(LrYumRepo *repo)
{
    PyObject *dict, *paths;

    if (!repo)
        Py_RETURN_NONE;

    if ((dict = PyDict_New()) == NULL)
        return NULL;

    PyDict_SetItemStringAndDecref(dict, "repomd",
            PyStringOrNone_FromString(repo->repomd));
    PyDict_SetItemStringAndDecref(dict, "url",
            PyStringOrNone_FromString(repo->url));
    PyDict_SetItemStringAndDecref(dict, "destdir",
            PyStringOrNone_FromString(repo->destdir));
    PyDict_SetItemStringAndDecref(dict, "signature",
            PyStringOrNone_FromString(repo->signature));
    PyDict_SetItemStringAndDecref(dict, "mirrorlist",
            PyStringOrNone_FromString(repo->mirrorlist));
    PyDict_SetItemStringAndDecref(dict, "metalink",
            PyStringOrNone_FromString(repo->metalink));

    if ((paths = PyDict_New()) == NULL)
        return NULL;

    for (GSList *elem = repo->paths; elem; elem = g_slist_next(elem)) {
        LrYumRepoPath *yumrepopath = elem->data;
        if (!yumrepopath || !yumrepopath->type) continue;
        PyDict_SetItemStringAndDecref(paths,
                             yumrepopath->type,
                             PyStringOrNone_FromString(yumrepopath->path));
    }

    PyDict_SetItemStringAndDecref(dict, "paths", paths);

    return dict;
}

PyObject *
PyObject_FromRepoMdRecord(LrYumRepoMdRecord *rec)
{
    PyObject *dict;

    if (!rec)
        Py_RETURN_NONE;

    if ((dict = PyDict_New()) == NULL)
        return NULL;

    PyDict_SetItemStringAndDecref(dict, "location_href",
            PyStringOrNone_FromString(rec->location_href));
    PyDict_SetItemStringAndDecref(dict, "checksum",
            PyStringOrNone_FromString(rec->checksum));
    PyDict_SetItemStringAndDecref(dict, "checksum_type",
            PyStringOrNone_FromString(rec->checksum_type));
    PyDict_SetItemStringAndDecref(dict, "checksum_open",
            PyStringOrNone_FromString(rec->checksum_open));
    PyDict_SetItemStringAndDecref(dict, "checksum_open_type",
            PyStringOrNone_FromString(rec->checksum_open_type));
    PyDict_SetItemStringAndDecref(dict, "timestamp",
            PyLong_FromLongLong((PY_LONG_LONG) rec->timestamp));
    PyDict_SetItemStringAndDecref(dict, "size",
            PyLong_FromLongLong((PY_LONG_LONG) rec->size));
    PyDict_SetItemStringAndDecref(dict, "size_open",
            PyLong_FromLongLong((PY_LONG_LONG) rec->size_open));
    PyDict_SetItemStringAndDecref(dict, "db_version",
            PyLong_FromLong((long) rec->db_version));

    return dict;
}

PyObject *
PyObject_FromYumRepoMd(LrYumRepoMd *repomd)
{
    PyObject *dict, *list;

    if (!repomd)
        Py_RETURN_NONE;

    if ((dict = PyDict_New()) == NULL)
        return NULL;

    PyDict_SetItemStringAndDecref(dict,
                         "revision",
                         PyStringOrNone_FromString(repomd->revision));

    list = PyList_New(0);
    for (GSList *elem = repomd->repo_tags; elem; elem = g_slist_next(elem)) {
        char *tag = elem->data;
        if (tag)
            PyList_Append(list, PyStringOrNone_FromString(tag));
    }
    PyDict_SetItemStringAndDecref(dict, "repo_tags", list);

    list = PyList_New(0);
    for (GSList *elem = repomd->distro_tags; elem; elem = g_slist_next(elem)) {
        LrYumDistroTag *distrotag = elem->data;

        if (!elem->data)
            continue;

        char *cpeid = distrotag->cpeid;
        char *value = distrotag->tag;

        if (value) {
            PyList_Append(list, Py_BuildValue("(NN)",
                                    PyStringOrNone_FromString(cpeid),
                                    PyStringOrNone_FromString(value)));
        }
    }
    PyDict_SetItemStringAndDecref(dict, "distro_tags", list);

    list = PyList_New(0);
    for (GSList *elem = repomd->content_tags; elem; elem = g_slist_next(elem)) {
        char *tag = elem->data;
        if (tag)
            PyList_Append(list, PyStringOrNone_FromString(tag));
    }
    PyDict_SetItemStringAndDecref(dict, "content_tags", list);

    for (GSList *elem = repomd->records; elem; elem = g_slist_next(elem)) {
        LrYumRepoMdRecord *record = elem->data;

        if (!record)
            continue;

        PyDict_SetItemStringAndDecref(dict,
                            record->type,
                            PyObject_FromRepoMdRecord(record));
    }

    return dict;
}

PyObject *
PyObject_FromYumRepoMd_v2(LrYumRepoMd *repomd)
{
    PyObject *dict, *list, *records;

    if (!repomd)
        Py_RETURN_NONE;

    if ((dict = PyDict_New()) == NULL)
        return NULL;

    PyDict_SetItemStringAndDecref(dict,
                         "revision",
                         PyStringOrNone_FromString(repomd->revision));

    list = PyList_New(0);
    for (GSList *elem = repomd->repo_tags; elem; elem = g_slist_next(elem)) {
        char *tag = elem->data;
        if (tag)
            PyList_Append(list, PyStringOrNone_FromString(tag));
    }
    PyDict_SetItemStringAndDecref(dict, "repo_tags", list);

    list = PyList_New(0);
    for (GSList *elem = repomd->distro_tags; elem; elem = g_slist_next(elem)) {
        LrYumDistroTag *distrotag = elem->data;

        if (!elem->data)
            continue;

        char *cpeid = distrotag->cpeid;
        char *value = distrotag->tag;

        if (value) {
            PyList_Append(list, Py_BuildValue("(NN)",
                                    PyStringOrNone_FromString(cpeid),
                                    PyStringOrNone_FromString(value)));
        }
    }
    PyDict_SetItemStringAndDecref(dict, "distro_tags", list);

    list = PyList_New(0);
    for (GSList *elem = repomd->content_tags; elem; elem = g_slist_next(elem)) {
        char *tag = elem->data;
        if (tag)
            PyList_Append(list, PyStringOrNone_FromString(tag));
    }
    PyDict_SetItemStringAndDecref(dict, "content_tags", list);

    records = PyDict_New();
    for (GSList *elem = repomd->records; elem; elem = g_slist_next(elem)) {
        LrYumRepoMdRecord *record = elem->data;

        if (!record)
            continue;

        PyDict_SetItemStringAndDecref(records,
                            record->type,
                            PyObject_FromRepoMdRecord(record));
    }
    PyDict_SetItemStringAndDecref(dict, "records", records);

    return dict;
}

PyObject *
PyObject_FromMetalink(LrMetalink *metalink)
{
    PyObject *dict, *sub_list;

    if (!metalink)
        Py_RETURN_NONE;

    if ((dict = PyDict_New()) == NULL)
        return NULL;

    PyDict_SetItemStringAndDecref(dict, "filename",
            PyStringOrNone_FromString(metalink->filename));
    PyDict_SetItemStringAndDecref(dict, "timestamp",
            PyLong_FromLongLong((PY_LONG_LONG)metalink->timestamp));
    PyDict_SetItemStringAndDecref(dict, "size",
            PyLong_FromLongLong((PY_LONG_LONG)metalink->size));

    // Hashes
    if ((sub_list = PyList_New(0)) == NULL) {
        PyDict_Clear(dict);
        return NULL;
    }
    PyDict_SetItemStringAndDecref(dict, "hashes", sub_list);

    for (GSList *elem = metalink->hashes; elem; elem = g_slist_next(elem)) {
        LrMetalinkHash *metalinkhash = elem->data;
        PyObject *tuple;
        if ((tuple = PyTuple_New(2)) == NULL) {
            PyDict_Clear(dict);
            return NULL;
        }
        PyTuple_SetItem(tuple, 0,
                PyStringOrNone_FromString(metalinkhash->type));
        PyTuple_SetItem(tuple, 1,
                PyStringOrNone_FromString(metalinkhash->value));
        PyList_Append(sub_list, tuple);
    }

    // Urls
    if ((sub_list = PyList_New(0)) == NULL) {
        PyDict_Clear(dict);
        return NULL;
    }
    PyDict_SetItemStringAndDecref(dict, "urls", sub_list);

    for (GSList *elem = metalink->urls; elem; elem = g_slist_next(elem)) {
        LrMetalinkUrl *metalinkurl = elem->data;
        PyObject *udict;
        if ((udict = PyDict_New()) == NULL) {
            PyDict_Clear(dict);
            return NULL;
        }
        PyDict_SetItemStringAndDecref(udict, "protocol",
                PyStringOrNone_FromString(metalinkurl->protocol));
        PyDict_SetItemStringAndDecref(udict, "type",
                PyStringOrNone_FromString(metalinkurl->type));
        PyDict_SetItemStringAndDecref(udict, "location",
                PyStringOrNone_FromString(metalinkurl->location));
        PyDict_SetItemStringAndDecref(udict, "preference",
                PyLong_FromLong((long) metalinkurl->preference));
        PyDict_SetItemStringAndDecref(udict, "url",
                PyStringOrNone_FromString(metalinkurl->url));
        PyList_Append(sub_list, udict);
    }

    // Alternates

    if (metalink->alternates) {

        if ((sub_list = PyList_New(0)) == NULL) {
            PyDict_Clear(dict);
            return NULL;
        }
        PyDict_SetItemStringAndDecref(dict, "alternates", sub_list);

        for (GSList *elem = metalink->alternates; elem; elem = g_slist_next(elem)) {
            LrMetalinkAlternate *ma = elem->data;
            PyObject *udict;
            if ((udict = PyDict_New()) == NULL) {
                PyDict_Clear(dict);
                return NULL;
            }
            PyDict_SetItemStringAndDecref(udict, "timestamp",
                PyLong_FromLongLong((PY_LONG_LONG)ma->timestamp));
            PyDict_SetItemStringAndDecref(udict, "size",
                PyLong_FromLongLong((PY_LONG_LONG)ma->size));

            PyObject *usub_list;
            if ((usub_list = PyList_New(0)) == NULL) {
                PyDict_Clear(dict);
                return NULL;
            }
            PyDict_SetItemStringAndDecref(udict, "hashes", usub_list);

            for (GSList *subelem = ma->hashes; subelem; subelem = g_slist_next(subelem)) {
                LrMetalinkHash *metalinkhash = subelem->data;
                PyObject *tuple;
                if ((tuple = PyTuple_New(2)) == NULL) {
                    PyDict_Clear(dict);
                    return NULL;
                }
                PyTuple_SetItem(tuple, 0,
                        PyStringOrNone_FromString(metalinkhash->type));
                PyTuple_SetItem(tuple, 1,
                        PyStringOrNone_FromString(metalinkhash->value));
                PyList_Append(usub_list, tuple);
            }

            PyList_Append(sub_list, udict);
        }
    }

    return dict;
}
