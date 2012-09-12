#ifndef LR_METALINK_H
#define LR_METALINK_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LR_METALINK_RC_OK,      /*!< everything ok */
    LR_METALINK_RC_IO_ERR,  /*!< input/output error */
    LR_METALINK_RC_BADFILE, /*!< metalink doesn't contain repomd.xml */
    LR_METALINK_RC_MISSING_ATTR, /*!< some important attribute(s) is/are missing */
    LR_METALINK_RC_XML_ERR, /*!< non valid xml */
} lr_MetalinkRc;

struct _lr_MetalinkHash {
    char *type;     /*!< "md5", "sha1", "sha256", "sha512", ... */
    char *value;    /*!< file hash */
};
typedef struct _lr_MetalinkHash * lr_MetalinkHash;

struct _lr_MetalinkUrl {
    char *protocol;     /*!< "http", "ftp", "rsync", ... */
    char *type;         /*!< "http", "ftp", "rsync", ... */
    char *location;     /*!< ISO 3166-1 alpha-2 code ("US", "CZ", ..) */
    int preference;     /*!< integer number 1-100, higher is better */
    char *url;          /*!< URL to the target file */
};
typedef struct _lr_MetalinkUrl * lr_MetalinkUrl;

struct _lr_Metalink {
    char *filename;
    long timestamp;
    long size;
    lr_MetalinkHash *hashes;    /* could be NULL */
    lr_MetalinkUrl *urls;       /* could be NULL */

    int noh;    /*!< number of hashes */
    int nou;    /*!< number of urls */
    int loh;    /*!< lenght of hashes list */
    int lou;    /*!< lenght of urls list */
};
typedef struct _lr_Metalink * lr_Metalink;

lr_Metalink lr_metalink_create();
int lr_metalink_parse_file(lr_Metalink metalink, int fd);
void lr_metalink_free(lr_Metalink metalink);

#ifdef __cplusplus
}
#endif

#endif
