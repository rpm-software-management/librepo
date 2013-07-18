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

#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>

#include "types.h"
#include "setup.h"
#include "rcodes.h"
#include "librepo.h"
#include "handle_internal.h"
#include "result_internal.h"
#include "util.h"

void
lr_global_init()
{
#ifdef CURL_GLOBAL_ACK_EINTR
#define EINTR_SUPPORT " with CURL_GLOBAL_ACK_EINTR support"
    curl_global_init(CURL_GLOBAL_ALL|CURL_GLOBAL_ACK_EINTR);
#else
#define EINTR_SUPPORT ""
    curl_global_init(CURL_GLOBAL_ALL);
#endif

    g_debug("Librepo version: %d.%d.%d%s (%s)", LR_VERSION_MAJOR,
                                                LR_VERSION_MINOR,
                                                LR_VERSION_PATCH,
                                                EINTR_SUPPORT,
                                                curl_version());
}

void
lr_global_cleanup()
{
    curl_global_cleanup();
}
