#ifndef LR_REPOMD_H
#define LR_REPOMD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "librepo.h"

typedef enum {
    LR_YUM_REPOMD_RC_OK,      /*!< everything ok */
    LR_YUM_REPOMD_RC_IO_ERR,  /*!< input/output error */
    LR_YUM_REPOMD_RC_XML_ERR, /*!< non valid xml */
} lr_RepoMdRc;

lr_YumRepoMd lr_yum_repomd_create();
void lr_yum_repomd_free(lr_YumRepoMd repomd);
int lr_yum_repomd_parse_file(lr_YumRepoMd repomd, int fd);

#ifdef __cplusplus
}
#endif

#endif
