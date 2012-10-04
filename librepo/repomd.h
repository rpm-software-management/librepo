#ifndef LR_REPOMD_H
#define LR_REPOMD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

/* Return codes of the module:
 *  LRE_OK          everything ok
 *  LRE_IO          input/output error
 *  LR_REPOMD_XML   xml parse error
 */

lr_YumRepoMd lr_yum_repomd_init();
void lr_yum_repomd_clear(lr_YumRepoMd repomd);
void lr_yum_repomd_free(lr_YumRepoMd repomd);
int lr_yum_repomd_parse_file(lr_YumRepoMd repomd, int fd);

#ifdef __cplusplus
}
#endif

#endif
