/* librepo - A library providing (libcURL like) API to downloading repository
 *
 * Copyright (C) 2012 Colin Walters <walters@verbum.org>.
 * Copyright (C) 2014 Richard Hughes <richard@hughsie.com>
 * Copyright (C) 2014 Tomas Mlcoch
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

/* This file was taken from libhif (https://github.com/hughsie/libhif) */

#ifndef __LR_CLEANUP_H__
#define __LR_CLEANUP_H__

#include <glib.h>
#include <unistd.h>

G_BEGIN_DECLS

static inline void
lr_close(int fildes)
{
    if (fildes < 0)
        return;
    close(fildes);
}

#define LR_DEFINE_CLEANUP_FUNCTION(Type, name, func) \
  static inline void name (Type *v) \
  { \
    func (*v); \
  }

#define LR_DEFINE_CLEANUP_FUNCTION0(Type, name, func) \
  static inline void name (Type *v) \
  { \
    if (*v) \
      func (*v); \
  }

#define LR_DEFINE_CLEANUP_FUNCTIONt(Type, name, func) \
  static inline void name (Type *v) \
  { \
    if (*v) \
      func (*v, TRUE); \
  }

LR_DEFINE_CLEANUP_FUNCTION0(GArray*, lr_local_array_unref, g_array_unref)
LR_DEFINE_CLEANUP_FUNCTION0(GChecksum*, lr_local_checksum_free, g_checksum_free)
LR_DEFINE_CLEANUP_FUNCTION0(GDir*, lr_local_dir_close, g_dir_close)
LR_DEFINE_CLEANUP_FUNCTION0(GError*, lr_local_free_error, g_error_free)
LR_DEFINE_CLEANUP_FUNCTION0(GHashTable*, lr_local_hashtable_unref, g_hash_table_unref)
#if GLIB_CHECK_VERSION(2, 32, 0)
LR_DEFINE_CLEANUP_FUNCTION0(GKeyFile*, lr_local_keyfile_unref, g_key_file_unref)
#endif
LR_DEFINE_CLEANUP_FUNCTION0(GKeyFile*, lr_local_keyfile_free, g_key_file_free)
LR_DEFINE_CLEANUP_FUNCTION0(GPtrArray*, lr_local_ptrarray_unref, g_ptr_array_unref)
LR_DEFINE_CLEANUP_FUNCTION0(GTimer*, lr_local_destroy_timer, g_timer_destroy)
LR_DEFINE_CLEANUP_FUNCTION0(GDateTime*, lr_local_date_time_unref, g_date_time_unref)

LR_DEFINE_CLEANUP_FUNCTIONt(GString*, lr_local_free_string, g_string_free)

LR_DEFINE_CLEANUP_FUNCTION(char**, lr_local_strfreev, g_strfreev)
LR_DEFINE_CLEANUP_FUNCTION(GList*, lr_local_free_list, g_list_free)
LR_DEFINE_CLEANUP_FUNCTION(GSList*, lr_local_free_slist, g_slist_free)
LR_DEFINE_CLEANUP_FUNCTION(int, lr_local_fd_close, lr_close)

/*
 * g_free() could be used for any pointer type.
 * To avoid warnings, we must take void * in place of void **.
 */
static inline void
lr_local_free(void *v)
{
    g_free(*(void **)v);
}

#define _cleanup_dir_close_ __attribute__ ((cleanup(lr_local_dir_close)))
#define _cleanup_fd_close_ __attribute__ ((cleanup(lr_local_fd_close)))
#define _cleanup_timer_destroy_ __attribute__ ((cleanup(lr_local_destroy_timer)))
#define _cleanup_free_ __attribute__ ((cleanup(lr_local_free)))
#define _cleanup_checksum_free_ __attribute__ ((cleanup(lr_local_checksum_free)))
#define _cleanup_error_free_ __attribute__ ((cleanup(lr_local_free_error)))
#define _cleanup_list_free_ __attribute__ ((cleanup(lr_local_free_list)))
#define _cleanup_slist_free_ __attribute__ ((cleanup(lr_local_free_slist)))
#define _cleanup_string_free_ __attribute__ ((cleanup(lr_local_free_string)))
#define _cleanup_strv_free_ __attribute__ ((cleanup(lr_local_strfreev)))
#define _cleanup_array_unref_ __attribute__ ((cleanup(lr_local_array_unref)))
#define _cleanup_hashtable_unref_ __attribute__ ((cleanup(lr_local_hashtable_unref)))
#define _cleanup_keyfile_unref_ __attribute__ ((cleanup(lr_local_keyfile_unref)))
#define _cleanup_keyfile_free_ __attribute__ ((cleanup(lr_local_keyfile_free)))
#define _cleanup_ptrarray_unref_ __attribute__ ((cleanup(lr_local_ptrarray_unref)))
#define _cleanup_date_time_unref_ __attribute__ ((cleanup(lr_local_date_time_unref)))

G_END_DECLS

#endif
