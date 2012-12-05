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

#include "librepo/librepo.h"

#include "exception-py.h"
#include "handle-py.h"
#include "result-py.h"

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

    /* Exceptions */
    if (!init_exceptions())
        return;
    PyModule_AddObject(m, "Exception", LrErr_Exception);

    /* Objects */
    /* _librepo.Handle */
    if (PyType_Ready(&Handle_Type) < 0)
        return;
    Py_INCREF(&Handle_Type);
    PyModule_AddObject(m, "Handle", (PyObject *)&Handle_Type);
    /* _librepo.Result */
    if (PyType_Ready(&Result_Type) < 0)
        return;
    Py_INCREF(&Result_Type);
    PyModule_AddObject(m, "Result", (PyObject *)&Result_Type);

    /* Module constants */

    /* Version */
    PyModule_AddIntConstant(m, "VERSION_MAJOR", LR_VERSION_MAJOR);
    PyModule_AddIntConstant(m, "VERSION_MINOR", LR_VERSION_MINOR);
    PyModule_AddIntConstant(m, "VERSION_PATCH", LR_VERSION_PATCH);

    /* Handle options */
    PyModule_AddIntConstant(m, "LRO_UPDATE", LRO_UPDATE);
    PyModule_AddIntConstant(m, "LRO_URL", LRO_URL);
    PyModule_AddIntConstant(m, "LRO_MIRRORLIST", LRO_MIRRORLIST);
    PyModule_AddIntConstant(m, "LRO_LOCAL", LRO_LOCAL);
    PyModule_AddIntConstant(m, "LRO_HTTPAUTH", LRO_HTTPAUTH);
    PyModule_AddIntConstant(m, "LRO_USERPWD", LRO_USERPWD);
    PyModule_AddIntConstant(m, "LRO_PROXY", LRO_PROXY);
    PyModule_AddIntConstant(m, "LRO_PROXYPORT", LRO_PROXYPORT);
    PyModule_AddIntConstant(m, "LRO_PROXYSOCK", LRO_PROXYSOCK);
    PyModule_AddIntConstant(m, "LRO_PROXYAUTH", LRO_PROXYAUTH);
    PyModule_AddIntConstant(m, "LRO_PROXYUSERPWD", LRO_PROXYUSERPWD);
    PyModule_AddIntConstant(m, "LRO_PROGRESSCB", LRO_PROGRESSCB);
    PyModule_AddIntConstant(m, "LRO_PROGRESSDATA", LRO_PROGRESSDATA);
    PyModule_AddIntConstant(m, "LRO_RETRIES", LRO_RETRIES);
    PyModule_AddIntConstant(m, "LRO_MAXSPEED", LRO_MAXSPEED);
    PyModule_AddIntConstant(m, "LRO_DESTDIR", LRO_DESTDIR);
    PyModule_AddIntConstant(m, "LRO_REPOTYPE", LRO_REPOTYPE);
    PyModule_AddIntConstant(m, "LRO_GPGCHECK", LRO_GPGCHECK);
    PyModule_AddIntConstant(m, "LRO_CHECKSUM", LRO_CHECKSUM);
    PyModule_AddIntConstant(m, "LRO_YUMREPOFLAGS", LRO_YUMREPOFLAGS);
    PyModule_AddIntConstant(m, "LRO_SENTINEL", LRO_SENTINEL);

    /* Check options */
    PyModule_AddIntConstant(m, "LR_CHECK_GPG", LR_CHECK_GPG);
    PyModule_AddIntConstant(m, "LR_CHECK_CHECKSUM", LR_CHECK_CHECKSUM);

    /* Repo type */
    PyModule_AddIntConstant(m, "LR_YUMREPO", LR_YUMREPO);
    PyModule_AddIntConstant(m, "LR_SUSEREPO", LR_SUSEREPO);
    PyModule_AddIntConstant(m, "LR_DEBREPO", LR_DEBREPO);

    /* Yum repo flags */
    PyModule_AddIntConstant(m, "LR_YUM_REPOMDONLY", LR_YUM_REPOMDONLY);
    PyModule_AddIntConstant(m, "LR_YUM_PRI", LR_YUM_PRI);
    PyModule_AddIntConstant(m, "LR_YUM_FIL", LR_YUM_FIL);
    PyModule_AddIntConstant(m, "LR_YUM_OTH", LR_YUM_OTH);
    PyModule_AddIntConstant(m, "LR_YUM_PRIDB", LR_YUM_PRIDB);
    PyModule_AddIntConstant(m, "LR_YUM_FILDB", LR_YUM_FILDB);
    PyModule_AddIntConstant(m, "LR_YUM_OTHDB", LR_YUM_OTHDB);
    PyModule_AddIntConstant(m, "LR_YUM_GROUP", LR_YUM_GROUP);
    PyModule_AddIntConstant(m, "LR_YUM_GROUPGZ", LR_YUM_GROUPGZ);
    PyModule_AddIntConstant(m, "LR_YUM_PRESTODELTA", LR_YUM_PRESTODELTA);
    PyModule_AddIntConstant(m, "LR_YUM_DELTAINFO", LR_YUM_DELTAINFO);
    PyModule_AddIntConstant(m, "LR_YUM_UPDATEINFO", LR_YUM_UPDATEINFO);
    PyModule_AddIntConstant(m, "LR_YUM_ORIGIN", LR_YUM_ORIGIN);
    PyModule_AddIntConstant(m, "LR_YUM_BASEXML", LR_YUM_BASEXML);
    PyModule_AddIntConstant(m, "LR_YUM_BASEDB", LR_YUM_BASEDB);
    PyModule_AddIntConstant(m, "LR_YUM_BASEHAWKEY", LR_YUM_BASEHAWKEY);
    PyModule_AddIntConstant(m, "LR_YUM_FULL", LR_YUM_FULL);

    /* Return codes */
    PyModule_AddIntConstant(m, "LRE_OK", LRE_OK);
    PyModule_AddIntConstant(m, "LRE_BADFUNCARG", LRE_BADFUNCARG);
    PyModule_AddIntConstant(m, "LRE_BADOPTARG", LRE_BADOPTARG);
    PyModule_AddIntConstant(m, "LRE_UNKNOWNOPT", LRE_UNKNOWNOPT);
    PyModule_AddIntConstant(m, "LRE_CURLSETOPT", LRE_CURLSETOPT);
    PyModule_AddIntConstant(m, "LRE_ALREADYUSEDRESULT", LRE_ALREADYUSEDRESULT);
    PyModule_AddIntConstant(m, "LRE_INCOMPLETERESULT", LRE_INCOMPLETERESULT);
    PyModule_AddIntConstant(m, "LRE_CURLDUP", LRE_CURLDUP);
    PyModule_AddIntConstant(m, "LRE_CURL", LRE_CURL);
    PyModule_AddIntConstant(m, "LRE_CURLM", LRE_CURLM);
    PyModule_AddIntConstant(m, "LRE_BADSTATUS", LRE_BADSTATUS);
    PyModule_AddIntConstant(m, "LRE_NOTLOCAL", LRE_NOTLOCAL);
    PyModule_AddIntConstant(m, "LRE_CANNOTCREATEDIR", LRE_CANNOTCREATEDIR);
    PyModule_AddIntConstant(m, "LRE_IO", LRE_IO);
    PyModule_AddIntConstant(m, "LRE_MLBAD", LRE_MLBAD);
    PyModule_AddIntConstant(m, "LRE_MLXML", LRE_MLXML);
    PyModule_AddIntConstant(m, "LRE_BADCHECKSUM", LRE_BADCHECKSUM);
    PyModule_AddIntConstant(m, "LRE_REPOMDXML", LRE_REPOMDXML);
    PyModule_AddIntConstant(m, "LRE_NOURL", LRE_NOURL);
    PyModule_AddIntConstant(m, "LRE_CANNOTCREATETMP", LRE_CANNOTCREATETMP);
    PyModule_AddIntConstant(m, "LRE_UNKNOWNCHECKSUM", LRE_UNKNOWNCHECKSUM);
    PyModule_AddIntConstant(m, "LRE_BADURL", LRE_BADURL);
    PyModule_AddIntConstant(m, "LRE_UNKNOWNERROR", LRE_UNKNOWNERROR);

    /* Result option */
    PyModule_AddIntConstant(m, "LRR_YUM_REPO", LRR_YUM_REPO);
    PyModule_AddIntConstant(m, "LRR_YUM_REPOMD", LRR_YUM_REPOMD);
    PyModule_AddIntConstant(m, "LRR_SENTINEL", LRR_SENTINEL);

    /* Checksums */
    PyModule_AddIntConstant(m, "CHECKSUM_UNKNOWN", LR_CHECKSUM_UNKNOWN);
    PyModule_AddIntConstant(m, "CHECKSUM_MD2", LR_CHECKSUM_MD2);
    PyModule_AddIntConstant(m, "CHECKSUM_MD5", LR_CHECKSUM_MD5);
    PyModule_AddIntConstant(m, "CHECKSUM_SHA", LR_CHECKSUM_SHA);
    PyModule_AddIntConstant(m, "CHECKSUM_SHA1", LR_CHECKSUM_SHA1);
    PyModule_AddIntConstant(m, "CHECKSUM_SHA224", LR_CHECKSUM_SHA224);
    PyModule_AddIntConstant(m, "CHECKSUM_SHA256", LR_CHECKSUM_SHA256);
    PyModule_AddIntConstant(m, "CHECKSUM_SHA384", LR_CHECKSUM_SHA384);
    PyModule_AddIntConstant(m, "CHECKSUM_SHA512", LR_CHECKSUM_SHA512);
}
