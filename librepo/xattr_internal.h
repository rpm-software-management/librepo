/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2021 Red Hat, Inc.
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

#ifndef __LR_XATTR_INTERNAL_H__
#define __LR_XATTR_INTERNAL_H__

#define XATTR_CHKSUM_PREFIX     "user.Librepo.checksum."

#if __APPLE__

#define FGETXATTR(FD, NAME, VALUE, SIZE)   \
            fgetxattr((FD), (NAME), (VALUE), (SIZE), 0, 0)
#define FLISTXATTR(FD, LIST, SIZE)   \
            flistxattr((FD), (LIST), (SIZE), 0)
#define FREMOVEXATTR(FD, NAME)   \
            fremovexattr((FD), (NAME), 0)
#define FSETXATTR(FD, NAME, VALUE, SIZE, FLAGS)   \
            fsetxattr((FD), (NAME), (VALUE), (SIZE), (FLAGS), 0)
#define GETXATTR(PATH, NAME, VALUE, SIZE)   \
            getxattr((PATH), (NAME), (VALUE), (SIZE), 0, 0)

#else

#define FGETXATTR(FD, NAME, VALUE, SIZE)   \
            fgetxattr((FD), (NAME), (VALUE), (SIZE))
#define FLISTXATTR(FD, LIST, SIZE)   \
            flistxattr((FD), (LIST), (SIZE))
#define FREMOVEXATTR(FD, NAME)   \
            fremovexattr((FD), (NAME))
#define FSETXATTR(FD, NAME, VALUE, SIZE, FLAGS)   \
            fsetxattr((FD), (NAME), (VALUE), (SIZE), (FLAGS))
#define GETXATTR(PATH, NAME, VALUE, SIZE)   \
            getxattr((PATH), (NAME), (VALUE), (SIZE))

#endif

#endif
