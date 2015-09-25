/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2015  Tomas Mlcoch
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
#include "librepo/cleanup.h"

#include "typeconversion.h"
#include "exception-py.h"
#include "result-py.h"


#define LR_LOGDOMAIN    "librepo"


// Global variables
GSList *logfiledata_list = NULL;
G_LOCK_DEFINE(logfiledata_list_lock);

typedef struct {
    long uid;
    gchar *fn;
    FILE *f;
    guint handler_id;
} LogFileData;

void
logfiledata_free(LogFileData *data)
{
    if (!data)
        return;
    g_log_remove_handler(LR_LOGDOMAIN, data->handler_id);
    fclose(data->f);
    g_free(data->fn);
    g_free(data);
}

void
remove_all_log_handlers(void)
{
    G_LOCK(logfiledata_list_lock);

    for (GSList *elem = logfiledata_list; elem; elem = g_slist_next(elem))
        logfiledata_free((LogFileData *) elem->data);
    g_slist_free(logfiledata_list);
    logfiledata_list = NULL;

    G_UNLOCK(logfiledata_list_lock);
}

void
logfile_func(G_GNUC_UNUSED const gchar *log_domain,
             G_GNUC_UNUSED GLogLevelFlags log_level,
             const gchar *msg,
             gpointer user_data)
{
    LogFileData *data = user_data;
    _cleanup_free_ gchar *time = NULL;
    _cleanup_date_time_unref_ GDateTime *datetime = NULL;

    datetime = g_date_time_new_now_local();
    time = g_date_time_format(datetime, "%H:%M:%S");
    fprintf(data->f, "%s %s\n", time, msg);

    fflush(data->f);
}

PyObject *
py_log_set_file(G_GNUC_UNUSED PyObject *self, PyObject *args)
{
    static long uid = 0;
    const char *fn = NULL;

    // Parse arguments
    if (!PyArg_ParseTuple(args, "s:py_log_set_file", &fn))
        return NULL;

    // Open the file
    FILE *f = fopen(fn, "a");
    if (!f) {
        PyErr_Format(PyExc_IOError, "Cannot open %s: %s", fn, g_strerror(errno));
        return NULL;
    }

    // Setup user data
    LogFileData *data = g_malloc0(sizeof(*data));
    data->fn = g_strdup(fn);
    data->f = f;

    // Set handler
    data->handler_id = g_log_set_handler(LR_LOGDOMAIN,
                                         G_LOG_LEVEL_DEBUG,
                                         logfile_func,
                                         data);

    // Save user data (in a thread safe way)
    G_LOCK(logfiledata_list_lock);

    // Get unique ID of the handler
    data->uid = ++uid;

    // Append the data to the global list
    logfiledata_list = g_slist_prepend(logfiledata_list, data);

    G_UNLOCK(logfiledata_list_lock);

    // Log librepo version and current time (including timezone)
    lr_log_librepo_summary();

    // Return unique id of the handler data
    return PyLong_FromLong(data->uid);
}

PyObject *
py_log_remove_handler(G_GNUC_UNUSED PyObject *self, PyObject *args)
{
    long uid = -1;

    // Parse arguments
    if (!PyArg_ParseTuple(args, "l:py_log_remove_handler", &uid))
        return NULL;

    // Lock the list
    G_LOCK(logfiledata_list_lock);

    // Search for the corresponding LogFileData
    LogFileData *data = NULL;
    for (GSList *elem = logfiledata_list; elem; elem = g_slist_next(elem)) {
        data = elem->data;
        if (data->uid == uid)
            break;
    }

    if (!data || data->uid != uid) {
        // No corresponding handler found
        G_UNLOCK(logfiledata_list_lock);
        PyErr_Format(LrErr_Exception,
                     "Log handler with id %ld doesn't exist", uid);
        return NULL;
    }

    // Remove the item from the list
    logfiledata_list = g_slist_remove(logfiledata_list, data);

    // Unlock the list
    G_UNLOCK(logfiledata_list_lock);

    // Remove the handler and free the data
    logfiledata_free(data);

    Py_RETURN_NONE;
}
