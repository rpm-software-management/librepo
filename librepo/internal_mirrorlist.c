#include <assert.h>
#include <stdlib.h>

#include "util.h"
#include "internal_mirrorlist.h"

lr_InternalMirrorlist
lr_internalmirrorlist_from_mirrorlist(lr_Mirrorlist ml)
{
    lr_InternalMirrorlist iml;

    if (!ml || ml->nou == 0)
        return NULL;

    iml = lr_malloc0(sizeof(struct _lr_InternalMirrorlist));
    iml->nom = ml->nou;
    iml->mirrors = lr_malloc(sizeof(lr_InternalMirror *) * iml->nom);

    for (int x=0; x < iml->nom; x++) {
        lr_InternalMirror im;
        im = lr_malloc(sizeof(struct _lr_InternalMirror));
        im->url = lr_strdup(ml->urls[x]);
        im->preference = 100;
        im->fails = 0;
        iml->mirrors[x] = im;
    }

    return iml;
}

lr_InternalMirrorlist
lr_internalmirrorlist_from_metalink(lr_Metalink ml)
{
    lr_InternalMirrorlist iml;

    if (!ml || ml->nou == 0)
        return NULL;

    iml = lr_malloc0(sizeof(struct _lr_InternalMirrorlist));
    iml->nom = ml->nou;
    iml->mirrors = lr_malloc(sizeof(lr_InternalMirror *) * iml->nom);

    for (int x=0; x < iml->nom; x++) {
        lr_InternalMirror im;
        im = lr_malloc(sizeof(struct _lr_InternalMirror));
        im->url = lr_strdup(ml->urls[x]->url);
        im->preference = ml->urls[x]->preference;
        im->fails = 0;
        iml->mirrors[x] = im;
    }

    return iml;
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
