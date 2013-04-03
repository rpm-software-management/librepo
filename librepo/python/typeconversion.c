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
#include "typeconversion.h"

PyObject *
PyStringOrNone_FromString(const char *str)
{
    if (str == NULL)
        Py_RETURN_NONE;
    return PyString_FromString(str);
}

PyObject *
PyObject_FromYumRepo(lr_YumRepo repo)
{
    PyObject *dict;

    if (!repo)
        Py_RETURN_NONE;

    if ((dict = PyDict_New()) == NULL)
        return NULL;

    PyDict_SetItemString(dict, "repomd", PyStringOrNone_FromString(repo->repomd));
    PyDict_SetItemString(dict, "url", PyStringOrNone_FromString(repo->url));
    PyDict_SetItemString(dict, "destdir", PyStringOrNone_FromString(repo->destdir));
    PyDict_SetItemString(dict, "signature", PyStringOrNone_FromString(repo->signature));
    PyDict_SetItemString(dict, "mirrorlist", PyStringOrNone_FromString(repo->mirrorlist));

    for (int x = 0; x < repo->nop; x++) {
        PyDict_SetItemString(dict,
                             repo->paths[x]->type,
                             PyStringOrNone_FromString(repo->paths[x]->path));
    }

    return dict;
}

PyObject *
PyObject_FromRepoMdRecord(lr_YumRepoMdRecord rec)
{
    PyObject *dict;

    if (!rec)
        Py_RETURN_NONE;

    if ((dict = PyDict_New()) == NULL)
        return NULL;

    PyDict_SetItemString(dict, "location_href", PyStringOrNone_FromString(rec->location_href));
    PyDict_SetItemString(dict, "checksum", PyStringOrNone_FromString(rec->checksum));
    PyDict_SetItemString(dict, "checksum_type", PyStringOrNone_FromString(rec->checksum_type));
    PyDict_SetItemString(dict, "checksum_open", PyStringOrNone_FromString(rec->checksum_open));
    PyDict_SetItemString(dict, "checksum_open_type", PyStringOrNone_FromString(rec->checksum_open_type));
    PyDict_SetItemString(dict, "timestamp", PyLong_FromLong(rec->timestamp));
    PyDict_SetItemString(dict, "size", PyLong_FromLong(rec->size));
    PyDict_SetItemString(dict, "size_open", PyLong_FromLong(rec->size_open));
    PyDict_SetItemString(dict, "db_version", PyLong_FromLong((long) rec->db_version));

    return dict;
}

PyObject *
PyObject_FromYumRepoMd(lr_YumRepoMd repomd)
{
    PyObject *dict, *list;

    if (!repomd)
        Py_RETURN_NONE;

    if ((dict = PyDict_New()) == NULL)
        return NULL;

    PyDict_SetItemString(dict, "revision", PyStringOrNone_FromString(repomd->revision));

    list = PyList_New(0);
    for (int i=0; i < repomd->nort; i++) {
        char *tag = repomd->repo_tags[i];
        if (tag)
            PyList_Append(list, PyStringOrNone_FromString(tag));
    }
    PyDict_SetItemString(dict, "repo_tags", list);

    list = PyList_New(0);
    for (int i=0; i < repomd->nodt; i++) {
        char *cpeid = repomd->distro_tags[i]->cpeid;
        char *value = repomd->distro_tags[i]->value;
        if (value) {
            PyList_Append(list, Py_BuildValue("(NN)",
                                    PyStringOrNone_FromString(cpeid),
                                    PyStringOrNone_FromString(value)));
        }
    }
    PyDict_SetItemString(dict, "distro_tags", list);

    list = PyList_New(0);
    for (int i=0; i < repomd->noct; i++) {
        char *tag = repomd->content_tags[i];
        if (tag)
            PyList_Append(list, PyStringOrNone_FromString(tag));
    }
    PyDict_SetItemString(dict, "content_tags", list);

    for (int x=0; x < repomd->nor; x++)
        PyDict_SetItemString(dict,
                repomd->records[x]->type,
                PyObject_FromRepoMdRecord(repomd->records[x]));

    return dict;
}

PyObject *
PyObject_FromMetalink(lr_Metalink metalink)
{
    PyObject *dict, *sub_list;

    if (!metalink)
        Py_RETURN_NONE;

    if ((dict = PyDict_New()) == NULL)
        return NULL;

    PyDict_SetItemString(dict, "filename", PyStringOrNone_FromString(metalink->filename));
    PyDict_SetItemString(dict, "timestamp", PyLong_FromLong(metalink->timestamp));
    PyDict_SetItemString(dict, "size", PyLong_FromLong(metalink->size));

    // Hashes
    if ((sub_list = PyList_New(0)) == NULL) {
        PyDict_Clear(dict);
        return NULL;
    }
    PyDict_SetItemString(dict, "hashes", sub_list);

    for (int x = 0; x < metalink->noh; x++) {
        PyObject *tuple;
        if ((tuple = PyTuple_New(2)) == NULL) {
            PyDict_Clear(dict);
            return NULL;
        }
        PyTuple_SetItem(tuple, 0, PyStringOrNone_FromString(metalink->hashes[x]->type));
        PyTuple_SetItem(tuple, 1, PyStringOrNone_FromString(metalink->hashes[x]->value));
        PyList_Append(sub_list, tuple);
    }

    // Urls
    if ((sub_list = PyList_New(0)) == NULL) {
        PyDict_Clear(dict);
        return NULL;
    }
    PyDict_SetItemString(dict, "urls", sub_list);

    for (int x = 0; x < metalink->nou; x++) {
        PyObject *udict;
        if ((udict = PyDict_New()) == NULL) {
            PyDict_Clear(dict);
            return NULL;
        }
        PyDict_SetItemString(udict, "protocol",
                PyStringOrNone_FromString(metalink->urls[x]->protocol));
        PyDict_SetItemString(udict, "type",
                PyStringOrNone_FromString(metalink->urls[x]->type));
        PyDict_SetItemString(udict, "location",
                PyStringOrNone_FromString(metalink->urls[x]->location));
        PyDict_SetItemString(udict, "preference",
                PyLong_FromLong((long) metalink->urls[x]->preference));
        PyDict_SetItemString(udict, "url",
                PyStringOrNone_FromString(metalink->urls[x]->url));
        PyList_Append(sub_list, udict);
    }

    return dict;
}
