#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "internal_mirrorlist.h"

lr_InternalMirrorlist
lr_internalmirrorlist_new()
{
    return lr_malloc0(sizeof(struct _lr_InternalMirrorlist));
}

void
lr_internalmirrorlist_free(lr_InternalMirrorlist ml)
{
    if (!ml)
        return;

    for (int x=0; x < ml->nom; x++) {
        lr_free(ml->mirrors[x]->url);
        lr_free(ml->mirrors[x]);
    }
    lr_free(ml->mirrors);
    lr_free(ml);
}

void
lr_internalmirrorlist_append_url(lr_InternalMirrorlist iml, const char *url)
{
    lr_InternalMirror im;

    if (!iml || !url)
        return;

    im = lr_malloc(sizeof(struct _lr_InternalMirror));
    im->url = lr_strdup(url);
    im->preference = 100;
    im->fails = 0;

    iml->nom++;
    iml->mirrors = lr_realloc(iml->mirrors, sizeof(lr_InternalMirror *) * iml->nom);
    iml->mirrors[iml->nom-1] = im;
}

void
lr_internalmirrorlist_append_mirrorlist(lr_InternalMirrorlist iml, lr_Mirrorlist ml)
{
    int nom_old;
    if (!iml || !ml || ml->nou == 0)
        return;

    nom_old = iml->nom;
    iml->nom += ml->nou;
    iml->mirrors = lr_realloc(iml->mirrors, sizeof(lr_InternalMirror *) * iml->nom);

    for (int x=nom_old; x < iml->nom; x++) {
        int im_id = x - nom_old;
        lr_InternalMirror im;
        im = lr_malloc(sizeof(struct _lr_InternalMirror));
        im->url = lr_strdup(ml->urls[im_id]);
        im->preference = 100;
        im->fails = 0;
        iml->mirrors[x] = im;
    }
}

void
lr_internalmirrorlist_append_metalink(lr_InternalMirrorlist iml,
                                      lr_Metalink ml,
                                      const char *suffix)
{
    int nom_old;
    size_t suffix_len = 0;

    if (!iml || !ml || ml->nou == 0)
        return;

    if (suffix)
        suffix_len = strlen(suffix);

    nom_old = iml->nom;
    iml->nom += ml->nou;
    iml->mirrors = lr_realloc(iml->mirrors, sizeof(lr_InternalMirror *) * iml->nom);

    for (int x=nom_old; x < iml->nom; x++) {
        int im_id = x - nom_old;
        char *url = ml->urls[im_id]->url;
        lr_InternalMirror im;
        im = lr_malloc(sizeof(struct _lr_InternalMirror));
        im->url = lr_strdup(url);
        if (suffix_len) {
            /* Remove suffix if necessary */
            size_t url_len = strlen(url);
            if (url_len >= suffix_len && !strcmp(url+(url_len-suffix_len), suffix))
                im->url[url_len-suffix_len] = '\0';
        }
        im->preference = ml->urls[im_id]->preference;
        im->fails = 0;
        iml->mirrors[x] = im;
    }
}

lr_InternalMirror
lr_internalmirrorlist_get(lr_InternalMirrorlist iml, int i)
{
    if (!iml || i >= iml->nom)
        return NULL;
    return iml->mirrors[i];
}

char *
lr_internalmirrorlist_get_url(lr_InternalMirrorlist iml, int i)
{
    if (!iml || i >= iml->nom)
        return NULL;
    return iml->mirrors[i]->url;
}

int
lr_internalmirrorlist_len(lr_InternalMirrorlist iml)
{
    if (!iml)
        assert(0);
    return iml->nom;
}
