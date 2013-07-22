#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "lrmirrorlist.h"

static lr_LrMirror *
lr_lrmirror_new(const char *url, lr_UrlVars *urlvars)
{
    lr_LrMirror *mirror;

    mirror = lr_malloc0(sizeof(*mirror));
    mirror->url = lr_url_substitute(url, urlvars);
    return mirror;
}

static void
lr_lrmirror_free(void *data)
{
    lr_LrMirror *mirror = data;
    lr_free(mirror->url);
    lr_free(mirror);
}

void
lr_lrmirrorlist_free(lr_LrMirrorlist *list)
{
    if (!list)
        return;

    g_slist_free_full(list, lr_lrmirror_free);
}

lr_LrMirrorlist *
lr_lrmirrorlist_append_url(lr_LrMirrorlist *list,
                           const char *url,
                           lr_UrlVars *urlvars)
{
    if (!url || !strlen(url))
        return list;

    lr_LrMirror *mirror = lr_lrmirror_new(url, urlvars);
    mirror->preference = 100;

    return g_slist_append(list, mirror);
}

lr_LrMirrorlist *
lr_lrmirrorlist_append_mirrorlist(lr_LrMirrorlist *list,
                                  lr_Mirrorlist *mirrorlist,
                                  lr_UrlVars *urlvars)
{
    if (!mirrorlist || !mirrorlist->urls)
        return list;

    for (GSList *elem = mirrorlist->urls; elem; elem = g_slist_next(elem)) {
        char *url = elem->data;

        if (!url || !strlen(url))
            continue;

        lr_LrMirror *mirror = lr_lrmirror_new(url, urlvars);
        mirror->preference = 100;
        list = g_slist_append(list, mirror);
    }

    return list;
}

lr_LrMirrorlist *
lr_lrmirrorlist_append_metalink(lr_LrMirrorlist *list,
                                lr_Metalink *metalink,
                                const char *suffix,
                                lr_UrlVars *urlvars)
{
    size_t suffix_len = 0;

    if (!metalink || !metalink->urls)
        return list;

    if (suffix)
        suffix_len = strlen(suffix);

    for (GSList *elem = metalink->urls; elem; elem = g_slist_next(elem)) {
        lr_MetalinkUrl *metalinkurl = elem->data;
        assert(metalinkurl);
        char *url = metalinkurl->url;

        if (!url)
            continue;  // No url present

        size_t url_len = strlen(url);

        if (!url_len)
            continue;  // No url present

        char *url_copy = NULL;

        if (suffix_len) {
            /* Remove suffix if necessary */
            if (url_len >= suffix_len
                && !strcmp(url+(url_len-suffix_len), suffix))
                url_copy = g_strndup(url, url_len-suffix_len);
        }

        if (!url_copy)
            url_copy = lr_strdup(url);

        lr_LrMirror *mirror = lr_lrmirror_new(url_copy, urlvars);
        mirror->preference = metalinkurl->preference;
        lr_free(url_copy);
        list = g_slist_append(list, mirror);
    }

    return list;
}

lr_LrMirrorlist *
lr_lrmirrorlist_append_lrmirrorlist(lr_LrMirrorlist *list,
                                    lr_LrMirrorlist *other)
{
    if (!other)
        return list;

    for (lr_LrMirrorlist *elem = other; elem; elem = g_slist_next(elem)) {
        lr_LrMirror *oth = elem->data;
        lr_LrMirror *mirror = lr_lrmirror_new(oth->url, NULL);
        mirror->preference = oth->preference;
        mirror->fails      = oth->fails;
        list = g_slist_append(list, mirror);
    }

    return list;
}

lr_LrMirror *
lr_lrmirrorlist_nth(lr_LrMirrorlist *list,
                    unsigned int nth)
{
    return g_slist_nth_data(list, nth);
}

char *
lr_lrmirrorlist_nth_url(lr_LrMirrorlist *list,
                        unsigned int nth)
{
    lr_LrMirror *mirror = g_slist_nth_data(list, nth);
    return (mirror) ? mirror->url : NULL;
}
