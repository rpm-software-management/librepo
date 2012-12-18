#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "curltargetlist.h"

#define LR_CURLTARGETLIST_ALLOC_STEP    10

lr_CurlTarget
lr_curltarget_new()
{
    return lr_malloc0(sizeof(struct _lr_CurlTarget));
}

void
lr_curltarget_free(lr_CurlTarget target)
{
    if (!target) return;
    lr_free(target->path);
    lr_free(target->checksum);
    lr_free(target);
}

lr_CurlTargetList
lr_curltargetlist_new()
{
    return lr_malloc0(sizeof(struct _lr_CurlTargetList));
}

void
lr_curltargetlist_free(lr_CurlTargetList list)
{
    if (!list) return;
    for (int x = 0; x < list->used; x++)
        lr_curltarget_free(list->targets[x]);
    lr_free(list->targets);
    lr_free(list);
}

void
lr_curltargetlist_append(lr_CurlTargetList list, lr_CurlTarget target)
{
    assert(list);
    if (!target) return;

    if (list->size == list->used) {
        /* Realloc some extra space */
        list->size += LR_CURLTARGETLIST_ALLOC_STEP;
        list->targets = lr_realloc(list->targets,
                                   sizeof(lr_CurlTarget) * list->size);
    }

    list->targets[list->used] = target;
    list->used++;
}

int
lr_curltargetlist_len(lr_CurlTargetList list)
{
    assert(list);
    return list->used;
}

lr_CurlTarget
lr_curltargetlist_get(lr_CurlTargetList list, int index)
{
    assert(list);
    if (index >= list->used)
        return NULL;
    return list->targets[index];
}
