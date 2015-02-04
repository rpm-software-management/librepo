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

#include "librepo/librepo.h"

#include "exception-py.h"
#include "handle-py.h"
#include "packagedownloader-py.h"
#include "packagetarget-py.h"
#include "result-py.h"
#include "yum-py.h"
#include "downloader-py.h"
#include "globalstate-py.h" // GIL Hack
#include "typeconversion.h"

volatile int global_logger = 0;
volatile PyThreadState **global_state = NULL;

PyObject *debug_cb = NULL;
PyObject *debug_cb_data = NULL;
gint      debug_handler_id = -1;

G_LOCK_DEFINE(gil_hack_lock);

void
py_debug_cb(G_GNUC_UNUSED const gchar *log_domain,
            G_GNUC_UNUSED GLogLevelFlags log_level,
            const gchar *message,
            G_GNUC_UNUSED gpointer user_data)
{
    PyObject *arglist, *data, *result, *py_message;

    if (!debug_cb)
        return;

    // XXX: GIL Hack
    if (global_state)
        EndAllowThreads((PyThreadState **) global_state);
    // XXX: End of GIL Hack

    py_message = PyStringOrNone_FromString(message);
    data = (debug_cb_data) ? debug_cb_data : Py_None;
    arglist = Py_BuildValue("(OO)", py_message, data);
    result = PyObject_CallObject(debug_cb, arglist);
    Py_DECREF(arglist);
    Py_XDECREF(result);
    Py_DECREF(py_message);

    // XXX: GIL Hack
    if (global_state)
        BeginAllowThreads((PyThreadState **) global_state);
    // XXX: End of GIL Hack
}

PyObject *
py_set_debug_log_handler(G_GNUC_UNUSED PyObject *self, PyObject *args)
{
    PyObject *cb, *cb_data = NULL;

    if (!PyArg_ParseTuple(args, "O|O:py_set_debug_log_handler", &cb, &cb_data))
        return NULL;

    if (cb == Py_None)
        cb = NULL;

    if (cb && !PyCallable_Check(cb)) {
        PyErr_SetString(PyExc_TypeError, "parameter must be callable");
       return NULL;
    }

    Py_XDECREF(debug_cb);
    Py_XDECREF(debug_cb_data);

    debug_cb      = cb;
    debug_cb_data = cb_data;

    Py_XINCREF(debug_cb);
    Py_XINCREF(debug_cb_data);

    if (debug_cb) {
        debug_handler_id = g_log_set_handler("librepo", G_LOG_LEVEL_DEBUG,
                                             py_debug_cb, NULL);
        global_logger = 1;
    } else if (debug_handler_id != -1) {
        g_log_remove_handler("librepo", debug_handler_id);
    }

    Py_RETURN_NONE;
}

static struct PyMethodDef librepo_methods[] = {
    { "yum_repomd_get_age",     (PyCFunction)py_yum_repomd_get_age,
      METH_VARARGS, NULL },
    { "set_debug_log_handler",  (PyCFunction)py_set_debug_log_handler,
      METH_VARARGS, NULL },
    { "download_packages",      (PyCFunction)py_download_packages,
      METH_VARARGS, NULL },
    { "download_url",           (PyCFunction)py_download_url,
      METH_VARARGS, NULL },
    { NULL }
};

void
exit_librepo(void)
{
    Py_XDECREF(debug_cb);
    Py_XDECREF(debug_cb_data);
    Py_XDECREF(LrErr_Exception);
}

struct module_state {
    PyObject *error;
};

#if PY_MAJOR_VERSION >= 3
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#else
#define GETSTATE(m) (&_state)
static struct module_state _state;
#endif

#if PY_MAJOR_VERSION >= 3

static int librepo_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(GETSTATE(m)->error);
        return 0;
}

static int librepo_clear(PyObject *m) {
    Py_CLEAR(GETSTATE(m)->error);
        return 0;
}

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "_librepo",
    "A library providing C and Python (libcURL like) API for downloading "
    "linux repository metadata and packages",
    sizeof(struct module_state),
    librepo_methods,
    NULL,
    librepo_traverse,
    librepo_clear,
    NULL
};

#define INITERROR return NULL

PyObject *
PyInit__librepo(void)

#else
#define INITERROR return

void
init_librepo(void)
#endif
{
#if PY_MAJOR_VERSION >= 3
    PyObject *m = PyModule_Create(&moduledef);
#else
    PyObject *m = Py_InitModule("_librepo", librepo_methods);
#endif

    if (!m)
        INITERROR;

    struct module_state *st = GETSTATE(m);

    // Exceptions
    if (!init_exceptions()) {
        Py_DECREF(m);
        INITERROR;
    }

    st->error = LrErr_Exception;

    PyModule_AddObject(m, "LibrepoException", LrErr_Exception);

    // Objects

    // _librepo.Handle
    if (PyType_Ready(&Handle_Type) < 0)
        INITERROR;
    Py_INCREF(&Handle_Type);
    PyModule_AddObject(m, "Handle", (PyObject *)&Handle_Type);

    // _librepo.Result
    if (PyType_Ready(&Result_Type) < 0)
        INITERROR;
    Py_INCREF(&Result_Type);
    PyModule_AddObject(m, "Result", (PyObject *)&Result_Type);

    // _librepo.PackageTarget
    if (PyType_Ready(&PackageTarget_Type) < 0)
        INITERROR;
    Py_INCREF(&PackageTarget_Type);
    PyModule_AddObject(m, "PackageTarget", (PyObject *)&PackageTarget_Type);

    // Init module
    Py_AtExit(exit_librepo);

    // Module constants

    // Version
    PyModule_AddIntConstant(m, "LR_VERSION_MAJOR", LR_VERSION_MAJOR);
    PyModule_AddIntConstant(m, "LR_VERSION_MINOR", LR_VERSION_MINOR);
    PyModule_AddIntConstant(m, "LR_VERSION_PATCH", LR_VERSION_PATCH);

    // Handle options
    PyModule_AddIntConstant(m, "LRO_UPDATE", LRO_UPDATE);
    PyModule_AddIntConstant(m, "LRO_URLS", LRO_URLS);
    PyModule_AddIntConstant(m, "LRO_MIRRORLIST", LRO_MIRRORLIST);
    PyModule_AddIntConstant(m, "LRO_MIRRORLISTURL", LRO_MIRRORLISTURL);
    PyModule_AddIntConstant(m, "LRO_METALINKURL", LRO_METALINKURL);
    PyModule_AddIntConstant(m, "LRO_LOCAL", LRO_LOCAL);
    PyModule_AddIntConstant(m, "LRO_HTTPAUTH", LRO_HTTPAUTH);
    PyModule_AddIntConstant(m, "LRO_USERPWD", LRO_USERPWD);
    PyModule_AddIntConstant(m, "LRO_PROXY", LRO_PROXY);
    PyModule_AddIntConstant(m, "LRO_PROXYPORT", LRO_PROXYPORT);
    PyModule_AddIntConstant(m, "LRO_PROXYTYPE", LRO_PROXYTYPE);
    PyModule_AddIntConstant(m, "LRO_PROXYAUTH", LRO_PROXYAUTH);
    PyModule_AddIntConstant(m, "LRO_PROXYUSERPWD", LRO_PROXYUSERPWD);
    PyModule_AddIntConstant(m, "LRO_PROGRESSCB", LRO_PROGRESSCB);
    PyModule_AddIntConstant(m, "LRO_PROGRESSDATA", LRO_PROGRESSDATA);
    PyModule_AddIntConstant(m, "LRO_MAXSPEED", LRO_MAXSPEED);
    PyModule_AddIntConstant(m, "LRO_DESTDIR", LRO_DESTDIR);
    PyModule_AddIntConstant(m, "LRO_REPOTYPE", LRO_REPOTYPE);
    PyModule_AddIntConstant(m, "LRO_CONNECTTIMEOUT", LRO_CONNECTTIMEOUT);
    PyModule_AddIntConstant(m, "LRO_IGNOREMISSING", LRO_IGNOREMISSING);
    PyModule_AddIntConstant(m, "LRO_INTERRUPTIBLE", LRO_INTERRUPTIBLE);
    PyModule_AddIntConstant(m, "LRO_USERAGENT", LRO_USERAGENT);
    PyModule_AddIntConstant(m, "LRO_FETCHMIRRORS", LRO_FETCHMIRRORS);
    PyModule_AddIntConstant(m, "LRO_MAXMIRRORTRIES", LRO_MAXMIRRORTRIES);
    PyModule_AddIntConstant(m, "LRO_MAXPARALLELDOWNLOADS", LRO_MAXPARALLELDOWNLOADS);
    PyModule_AddIntConstant(m, "LRO_MAXDOWNLOADSPERMIRROR", LRO_MAXDOWNLOADSPERMIRROR);
    PyModule_AddIntConstant(m, "LRO_VARSUB", LRO_VARSUB);
    PyModule_AddIntConstant(m, "LRO_FASTESTMIRROR", LRO_FASTESTMIRROR);
    PyModule_AddIntConstant(m, "LRO_FASTESTMIRRORCACHE", LRO_FASTESTMIRRORCACHE);
    PyModule_AddIntConstant(m, "LRO_FASTESTMIRRORMAXAGE", LRO_FASTESTMIRRORMAXAGE);
    PyModule_AddIntConstant(m, "LRO_FASTESTMIRRORCB", LRO_FASTESTMIRRORCB);
    PyModule_AddIntConstant(m, "LRO_FASTESTMIRRORDATA", LRO_FASTESTMIRRORDATA);
    PyModule_AddIntConstant(m, "LRO_LOWSPEEDTIME", LRO_LOWSPEEDTIME);
    PyModule_AddIntConstant(m, "LRO_LOWSPEEDLIMIT", LRO_LOWSPEEDLIMIT);
    PyModule_AddIntConstant(m, "LRO_GPGCHECK", LRO_GPGCHECK);
    PyModule_AddIntConstant(m, "LRO_CHECKSUM", LRO_CHECKSUM);
    PyModule_AddIntConstant(m, "LRO_YUMDLIST", LRO_YUMDLIST);
    PyModule_AddIntConstant(m, "LRO_YUMBLIST", LRO_YUMBLIST);
    PyModule_AddIntConstant(m, "LRO_HMFCB", LRO_HMFCB);
    PyModule_AddIntConstant(m, "LRO_SSLVERIFYPEER", LRO_SSLVERIFYPEER);
    PyModule_AddIntConstant(m, "LRO_SSLVERIFYHOST", LRO_SSLVERIFYHOST);
    PyModule_AddIntConstant(m, "LRO_IPRESOLVE", LRO_IPRESOLVE);
    PyModule_AddIntConstant(m, "LRO_ALLOWEDMIRRORFAILURES", LRO_ALLOWEDMIRRORFAILURES);
    PyModule_AddIntConstant(m, "LRO_ADAPTIVEMIRRORSORTING", LRO_ADAPTIVEMIRRORSORTING);
    PyModule_AddIntConstant(m, "LRO_GNUPGHOMEDIR", LRO_GNUPGHOMEDIR);
    PyModule_AddIntConstant(m, "LRO_FASTESTMIRRORTIMEOUT", LRO_FASTESTMIRRORTIMEOUT);
    PyModule_AddIntConstant(m, "LRO_HTTPHEADER", LRO_HTTPHEADER);
    PyModule_AddIntConstant(m, "LRO_SENTINEL", LRO_SENTINEL);

    // Handle info options
    PyModule_AddIntConstant(m, "LRI_UPDATE", LRI_UPDATE);
    PyModule_AddIntConstant(m, "LRI_URLS", LRI_URLS);
    PyModule_AddIntConstant(m, "LRI_MIRRORLIST", LRI_MIRRORLIST);
    PyModule_AddIntConstant(m, "LRI_MIRRORLISTURL", LRI_MIRRORLISTURL);
    PyModule_AddIntConstant(m, "LRI_METALINKURL", LRI_METALINKURL);
    PyModule_AddIntConstant(m, "LRI_LOCAL", LRI_LOCAL);
    PyModule_AddIntConstant(m, "LRI_PROGRESSCB", LRI_PROGRESSCB);
    PyModule_AddIntConstant(m, "LRI_PROGRESSDATA", LRI_PROGRESSDATA);
    PyModule_AddIntConstant(m, "LRI_DESTDIR", LRI_DESTDIR);
    PyModule_AddIntConstant(m, "LRI_REPOTYPE", LRI_REPOTYPE);
    PyModule_AddIntConstant(m, "LRI_USERAGENT", LRI_USERAGENT);
    PyModule_AddIntConstant(m, "LRI_YUMDLIST", LRI_YUMDLIST);
    PyModule_AddIntConstant(m, "LRI_YUMBLIST", LRI_YUMBLIST);
    PyModule_AddIntConstant(m, "LRI_FETCHMIRRORS", LRI_FETCHMIRRORS);
    PyModule_AddIntConstant(m, "LRI_MAXMIRRORTRIES", LRI_MAXMIRRORTRIES);
    PyModule_AddIntConstant(m, "LRI_VARSUB", LRI_VARSUB);
    PyModule_AddIntConstant(m, "LRI_MIRRORS", LRI_MIRRORS);
    PyModule_AddIntConstant(m, "LRI_METALINK", LRI_METALINK);
    PyModule_AddIntConstant(m, "LRI_FASTESTMIRROR", LRI_FASTESTMIRROR);
    PyModule_AddIntConstant(m, "LRI_FASTESTMIRRORCACHE", LRI_FASTESTMIRRORCACHE);
    PyModule_AddIntConstant(m, "LRI_FASTESTMIRRORMAXAGE", LRI_FASTESTMIRRORMAXAGE);
    PyModule_AddIntConstant(m, "LRI_HMFCB", LRI_HMFCB);
    PyModule_AddIntConstant(m, "LRI_SSLVERIFYPEER", LRI_SSLVERIFYPEER);
    PyModule_AddIntConstant(m, "LRI_SSLVERIFYHOST", LRI_SSLVERIFYHOST);
    PyModule_AddIntConstant(m, "LRI_IPRESOLVE", LRI_IPRESOLVE);
    PyModule_AddIntConstant(m, "LRI_ALLOWEDMIRRORFAILURES", LRI_ALLOWEDMIRRORFAILURES);
    PyModule_AddIntConstant(m, "LRI_ADAPTIVEMIRRORSORTING", LRI_ADAPTIVEMIRRORSORTING);
    PyModule_AddIntConstant(m, "LRI_GNUPGHOMEDIR", LRI_GNUPGHOMEDIR);
    PyModule_AddIntConstant(m, "LRI_FASTESTMIRRORTIMEOUT", LRI_FASTESTMIRRORTIMEOUT);
    PyModule_AddIntConstant(m, "LRI_HTTPHEADER", LRI_HTTPHEADER);
    PyModule_AddIntConstant(m, "LRI_SENTINEL", LRI_SENTINEL);

    // Check options
    PyModule_AddIntConstant(m, "LR_CHECK_GPG", LR_CHECK_GPG);
    PyModule_AddIntConstant(m, "LR_CHECK_CHECKSUM", LR_CHECK_CHECKSUM);

    // Repo type
    PyModule_AddIntConstant(m, "LR_YUMREPO", LR_YUMREPO);
    PyModule_AddIntConstant(m, "LR_SUSEREPO", LR_SUSEREPO);
    PyModule_AddIntConstant(m, "LR_DEBREPO", LR_DEBREPO);

    // Proxy type
    PyModule_AddIntConstant(m, "LR_PROXY_HTTP", LR_PROXY_HTTP);
    PyModule_AddIntConstant(m, "LR_PROXY_HTTP_1_0", LR_PROXY_HTTP_1_0);
    PyModule_AddIntConstant(m, "LR_PROXY_SOCKS4", LR_PROXY_SOCKS4);
    PyModule_AddIntConstant(m, "LR_PROXY_SOCKS5", LR_PROXY_SOCKS5);
    PyModule_AddIntConstant(m, "LR_PROXY_SOCKS4A", LR_PROXY_SOCKS4A);
    PyModule_AddIntConstant(m, "LR_PROXY_SOCKS5_HOSTNAME", LR_PROXY_SOCKS5_HOSTNAME);

    // IpResolve type
    PyModule_AddIntConstant(m, "LR_IPRESOLVE_WHATEVER", LR_IPRESOLVE_WHATEVER);
    PyModule_AddIntConstant(m, "LR_IPRESOLVE_V4", LR_IPRESOLVE_V4);
    PyModule_AddIntConstant(m, "LR_IPRESOLVE_V6", LR_IPRESOLVE_V6);

    // Return codes
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
    PyModule_AddIntConstant(m, "LRE_TEMPORARYERR", LRE_TEMPORARYERR);
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
    PyModule_AddIntConstant(m, "LRE_GPGNOTSUPPORTED", LRE_GPGNOTSUPPORTED);
    PyModule_AddIntConstant(m, "LRE_GPGERROR", LRE_GPGERROR);
    PyModule_AddIntConstant(m, "LRE_BADGPG", LRE_BADGPG);
    PyModule_AddIntConstant(m, "LRE_INCOMPLETEREPO", LRE_INCOMPLETEREPO);
    PyModule_AddIntConstant(m, "LRE_INTERRUPTED", LRE_INTERRUPTED);
    PyModule_AddIntConstant(m, "LRE_SIGACTION", LRE_SIGACTION);
    PyModule_AddIntConstant(m, "LRE_ALREADYDOWNLOADED", LRE_ALREADYDOWNLOADED);
    PyModule_AddIntConstant(m, "LRE_UNFINISHED", LRE_UNFINISHED);
    PyModule_AddIntConstant(m, "LRE_SELECT", LRE_SELECT);
    PyModule_AddIntConstant(m, "LRE_OPENSSL", LRE_OPENSSL);
    PyModule_AddIntConstant(m, "LRE_MEMORY", LRE_MEMORY);
    PyModule_AddIntConstant(m, "LRE_XMLPARSER", LRE_XMLPARSER);
    PyModule_AddIntConstant(m, "LRE_CBINTERRUPTED", LRE_CBINTERRUPTED);
    PyModule_AddIntConstant(m, "LRE_UNKNOWNERROR", LRE_UNKNOWNERROR);

    // Result option
    PyModule_AddIntConstant(m, "LR_LRR_YUM_REPO", LRR_YUM_REPO);
    PyModule_AddIntConstant(m, "LR_LRR_YUM_REPOMD", LRR_YUM_REPOMD);
    PyModule_AddIntConstant(m, "LR_LRR_YUM_TIMESTAMP", LRR_YUM_TIMESTAMP);
    PyModule_AddIntConstant(m, "LR_LRR_SENTINEL", LRR_SENTINEL);

    // Checksums
    PyModule_AddIntConstant(m, "LR_CHECKSUM_UNKNOWN", LR_CHECKSUM_UNKNOWN);
    PyModule_AddIntConstant(m, "LR_CHECKSUM_MD5", LR_CHECKSUM_MD5);
    PyModule_AddIntConstant(m, "LR_CHECKSUM_SHA1", LR_CHECKSUM_SHA1);
    PyModule_AddIntConstant(m, "LR_CHECKSUM_SHA224", LR_CHECKSUM_SHA224);
    PyModule_AddIntConstant(m, "LR_CHECKSUM_SHA256", LR_CHECKSUM_SHA256);
    PyModule_AddIntConstant(m, "LR_CHECKSUM_SHA384", LR_CHECKSUM_SHA384);
    PyModule_AddIntConstant(m, "LR_CHECKSUM_SHA512", LR_CHECKSUM_SHA512);

    // Transfer statuses
    PyModule_AddIntConstant(m, "LR_TRANSFER_SUCCESSFUL", LR_TRANSFER_SUCCESSFUL);
    PyModule_AddIntConstant(m, "LR_TRANSFER_ALREADYEXISTS", LR_TRANSFER_ALREDYEXISTS);
    PyModule_AddIntConstant(m, "LR_TRANSFER_ERROR", LR_TRANSFER_ERROR);

    // Fastest mirror stages
    PyModule_AddIntConstant(m, "LR_FMSTAGE_INIT", LR_FMSTAGE_INIT);
    PyModule_AddIntConstant(m, "LR_FMSTAGE_CACHELOADING", LR_FMSTAGE_CACHELOADING);
    PyModule_AddIntConstant(m, "LR_FMSTAGE_CACHELOADINGSTATUS", LR_FMSTAGE_CACHELOADINGSTATUS);
    PyModule_AddIntConstant(m, "LR_FMSTAGE_DETECTION", LR_FMSTAGE_DETECTION);
    PyModule_AddIntConstant(m, "LR_FMSTAGE_FINISHING", LR_FMSTAGE_FINISHING);
    PyModule_AddIntConstant(m, "LR_FMSTAGE_STATUS", LR_FMSTAGE_STATUS);

    // Callbacks return values
    PyModule_AddIntConstant(m, "LR_CB_OK", LR_CB_OK);
    PyModule_AddIntConstant(m, "LR_CB_ABORT", LR_CB_ABORT);
    PyModule_AddIntConstant(m, "LR_CB_ERROR", LR_CB_ERROR);

#if PY_MAJOR_VERSION >= 3
    return m;
#endif
}
