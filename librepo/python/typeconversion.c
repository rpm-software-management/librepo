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

    if ((dict = PyDict_New()) == NULL)
        return NULL;

    PyDict_SetItemString(dict, "repomd", PyStringOrNone_FromString(repo->repomd));
    PyDict_SetItemString(dict, "primary", PyStringOrNone_FromString(repo->primary));
    PyDict_SetItemString(dict, "filelists", PyStringOrNone_FromString(repo->filelists));
    PyDict_SetItemString(dict, "other", PyStringOrNone_FromString(repo->other));
    PyDict_SetItemString(dict, "primary_db", PyStringOrNone_FromString(repo->primary_db));
    PyDict_SetItemString(dict, "filelists_db", PyStringOrNone_FromString(repo->filelists_db));
    PyDict_SetItemString(dict, "other_db", PyStringOrNone_FromString(repo->other_db));
    PyDict_SetItemString(dict, "group", PyStringOrNone_FromString(repo->group));
    PyDict_SetItemString(dict, "group_gz", PyStringOrNone_FromString(repo->group_gz));
    PyDict_SetItemString(dict, "prestodelta", PyStringOrNone_FromString(repo->prestodelta));
    PyDict_SetItemString(dict, "deltainfo", PyStringOrNone_FromString(repo->deltainfo));
    PyDict_SetItemString(dict, "updateinfo", PyStringOrNone_FromString(repo->updateinfo));
    PyDict_SetItemString(dict, "origin", PyStringOrNone_FromString(repo->origin));
    PyDict_SetItemString(dict, "url", PyStringOrNone_FromString(repo->url));
    PyDict_SetItemString(dict, "destdir", PyStringOrNone_FromString(repo->destdir));

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

    PyDict_SetItemString(dict, "primary", PyObject_FromRepoMdRecord(repomd->primary));
    PyDict_SetItemString(dict, "filelists", PyObject_FromRepoMdRecord(repomd->filelists));
    PyDict_SetItemString(dict, "other", PyObject_FromRepoMdRecord(repomd->other));
    PyDict_SetItemString(dict, "primary_db", PyObject_FromRepoMdRecord(repomd->primary_db));
    PyDict_SetItemString(dict, "filelists_db", PyObject_FromRepoMdRecord(repomd->filelists_db));
    PyDict_SetItemString(dict, "other_db", PyObject_FromRepoMdRecord(repomd->other_db));
    PyDict_SetItemString(dict, "group", PyObject_FromRepoMdRecord(repomd->group));
    PyDict_SetItemString(dict, "group_gz", PyObject_FromRepoMdRecord(repomd->group_gz));
    PyDict_SetItemString(dict, "prestodelta", PyObject_FromRepoMdRecord(repomd->prestodelta));
    PyDict_SetItemString(dict, "deltainfo", PyObject_FromRepoMdRecord(repomd->deltainfo));
    PyDict_SetItemString(dict, "updateinfo", PyObject_FromRepoMdRecord(repomd->updateinfo));
    PyDict_SetItemString(dict, "origin", PyObject_FromRepoMdRecord(repomd->origin));

    return dict;
}
