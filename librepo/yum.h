#ifndef LR_YUMREPO_H
#define LR_YUMREPO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "librepo.h"

/* Return codes of the module:
 *  LRE_OK      everything ok
 *  LRE_IO      input/output error
 *  LRE_URL     no usable URL (no base URL or mirrorlist URL specified)
 *  + Codes from metalink.h
 *  + Codes from mirrorlist.h
 *  + Codes from repomd.h
 *  + Codes from curl.h
 */

int lr_yum_perform(lr_Handle handle, void **repo_ptr);

#ifdef __cplusplus
}
#endif

#endif
