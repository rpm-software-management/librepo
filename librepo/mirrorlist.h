#ifndef LR_MIRRORLIST_H
#define LR_MIRRORLIST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Return codes of the module:
 *  LR_RC_OK    verything ok
 *  LR_RC_IO    input/output error
 */

struct _lr_Mirrorlist {
    char **urls;    /*!< could be NULL */
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
