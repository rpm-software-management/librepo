#ifndef LR_MIRRORLIST_H
#define LR_MIRRORLIST_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LR_MIRRORLIST_RC_OK,      /*!< everything ok */
    LR_MIRRORLIST_RC_IO_ERR,  /*!< input/output error */
} lr_MirrorlistRc;

struct _lr_Mirrorlist {
    char **urls;    /* could be NULL */
    int nou;        /*!< number of urls */
    int lou;        /*!< lenght of urls list */
};
typedef struct _lr_Mirrorlist * lr_Mirrorlist;

lr_Mirrorlist lr_mirrorlist_create();
int lr_mirrorlist_parse_file(lr_Mirrorlist mirrorlist, int fd);
void lr_mirrorlist_free(lr_Mirrorlist mirrorlist);

#ifdef __cplusplus
}
#endif

#endif
