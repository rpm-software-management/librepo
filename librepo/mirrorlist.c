#define _POSIX_SOURCE

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "util.h"
#include "mirrorlist.h"

#define BUF_LEN 4096

lr_Mirrorlist
lr_mirrorlist_create()
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
append_url(lr_Mirrorlist m, char *url)
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

    f = fdopen(dup(fd), "r");
    if (!f)
        return LR_MIRRORLIST_RC_IO_ERR;

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
        append_url(mirrorlist, lr_strdup(p));
    }

    fclose(f);

    return LR_MIRRORLIST_RC_OK;
}
