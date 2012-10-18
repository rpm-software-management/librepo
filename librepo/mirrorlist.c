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

#define _POSIX_SOURCE

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "setup.h"
#include "rcodes.h"
#include "util.h"
#include "mirrorlist.h"

#define BUF_LEN 4096

lr_Mirrorlist
lr_mirrorlist_init()
{
    return lr_malloc0(sizeof(struct _lr_Mirrorlist));
}

void
lr_mirrorlist_free(lr_Mirrorlist mirrorlist)
{
    if (!mirrorlist)
        return;
    for (int x = 0; x < mirrorlist->nou; x++)
        lr_free(mirrorlist->urls[x]);
    lr_free(mirrorlist->urls);
    lr_free(mirrorlist);
}

void
lr_mirrorlist_append_url(lr_Mirrorlist m, char *url)
{
    if (m->nou+1 > m->lou) {
        m->lou += 5;
        m->urls = lr_realloc(m->urls, m->lou * sizeof(char *));
    }

    m->urls[m->nou] = url;
    m->nou++;
    return;
}

int
lr_mirrorlist_parse_file(lr_Mirrorlist mirrorlist, int fd)
{
    FILE *f;
    char buf[BUF_LEN], *p;

    assert(mirrorlist);
    DEBUGASSERT(fd >= 0);

    f = fdopen(dup(fd), "r");
    if (!f)
        return LRE_IO;

    while ((p = fgets(buf, BUF_LEN, f))) {
        int l;

        /* Skip leading white characters */
        while (*p == ' ' || *p == '\t')
            p++;

        if (!*p || *p == '#')
            continue;  /* End of string or comment */

        l = strlen(p);
        /* Remove trailing white characters */
        while (l > 0 && (p[l-1] == ' ' || p[l-1] == '\n' || p[l-1] == '\t'))
            l--;
        p[l] = '\0';

        if (!l)
            continue;

        /* Append URL */
        if (p[0] != '\0' && (strstr(p, "://") || p[0] == '/'))
            lr_mirrorlist_append_url(mirrorlist, lr_strdup(p));
    }

    fclose(f);

    return LRE_OK;
}
