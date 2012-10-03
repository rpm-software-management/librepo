#ifndef LR_REPOUTIL_YUM_H
#define LR_REPOUTIL_YUM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Functions for repo manipulation */

/* 0 - Ok */
int lr_repoutil_yum_check_repo(const char *path);

int lr_repoutil_yum_parse_repomd(const char *path, lr_YumRepoMd repomd);

/*
int lr_repoutil_yum_update_repo(const char *path,
                                const char *url,
                                lr_update_cb cb);
*/
#ifdef __cplusplus
}
#endif

#endif
