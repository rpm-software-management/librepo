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

#ifndef __LR_GLOBALSTATE_PY_H__
#define __LR_GLOBALSTATE_PY_H__

#include "exception-py.h"

/* XXX: GIL Hack for debug logging callback
 *
 * The librepo logging callback is global for whole module.
 * This callback is called from different parts of librepo module.
 * This callback calls a python code.
 * When C module wants to call a python code, it HAS TO have a GIL.
 * This is a problem since the Librepo python bindings
 * releases the GIL before calling librepo blocking functions like:
 * - lr_handle_perform = librepo.Handle.perform()
 * - lr_download_package = librepo.Handle.download()
 * - lr_downlodad_packages = librepo.download_packages()
 * When the callback is called within any of this function,
 * it leads to the state in which a python code is called without
 * holding a GIL which causes a crash.
 *
 * So when a debug log handler is used the Librepo starts to
 * be thread-unsafe!
 *
 * This GIL hack is designed to detect this and raise an exception.
 * Exception with a comprehensible description is much more better
 * then interpreter crash.
 */

extern volatile int global_logger;  // Python callback for the logger is set
extern volatile PyThreadState **global_state;  // Thread state

#define GIL_HACK_ERROR          0   // Other python thread with running librepo
                                    // already exists. Raise an error.
#define GIL_HACK_MUST_CLEAR     1
#define GIL_HACK_DO_NOT_CLEAR   2

G_LOCK_EXTERN(gil_hack_lock);

static int
gil_logger_hack_begin(PyThreadState **state)
{
    int ret = GIL_HACK_DO_NOT_CLEAR;

    G_LOCK(gil_hack_lock);
    if (global_logger) {
        if (global_state) {
            // Raise exception
            PyErr_SetString(LrErr_Exception,
                            "Librepo is not threadsafe when python debug "
                            "logger is used! Other thread using librepo "
                            "was detected.");
            ret = GIL_HACK_ERROR;
        } else {
            global_state = (volatile PyThreadState **) state;
            ret = GIL_HACK_MUST_CLEAR;
        }
    }
    G_UNLOCK(gil_hack_lock);

    return ret;
}

static gboolean
gil_logger_hack_end(int hack_begin_rc)
{
    int ret;

    G_LOCK(gil_hack_lock);
    switch (hack_begin_rc) {
        case GIL_HACK_MUST_CLEAR:
            global_state = NULL;
            ret = TRUE;
            break;
        case GIL_HACK_DO_NOT_CLEAR:
            ret = TRUE;
            break;
        case GIL_HACK_ERROR:
        default:
            PyErr_SetString(LrErr_Exception,
                            "GIL logger hack failed");
            ret = FALSE;
    }
    G_UNLOCK(gil_hack_lock);

    return ret;
}

#endif
