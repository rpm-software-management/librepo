/* SPDX-License-Identifier: LGPL-2.1-or-later */
#ifndef LR_METALINK_INTERNAL_H
#define LR_METALINK_INTERNAL_H

#include "metalink.h"

/* Filter parsed URLs before exposing them through a handle. */
G_GNUC_INTERNAL void
lr_metalink_filter(LrMetalink *metalink, GRegex *domain, GRegex *location);

#endif
