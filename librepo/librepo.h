#ifndef LR_LIBREPO_H
#define LR_LIBREPO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <curl/curl.h>

typedef enum {
    LRE_OK,                         /*!< everything is ok */
    LRE_BAD_FUNCTION_ARGUMENT,      /*!< bad function argument */

    /* lr_setopt specific */
    LRE_BAD_OPTION_ARGUMENT,        /*!< bad argument of the option */
    LRE_UNKNOWN_OPTION,             /*!< library doesn't know the option */
    LRE_CURL_SETOPT,                /*!< cURL doesn't know the option.
                                         Too old curl version? */
    LRE_CURL_DUP,                   /*!< cannot duplicate curl handle */
    LRE_CURL,                       /*!< cURL error. Use the
                                         lr_last_curl_error to get CURLcode */
    LRE_CURLM,                      /*!< cULR multi handle error. Use the
                                         lr_last_mculr_error to get CURLMcode */
    LRE_BAD_STATUS,                 /*!< HTTP or FTP returned status code which
                                         do not represent success
                                         (file doesn't exists, etc.) */
    LRE_SELECT,                     /*!< error while call select() on set
                                         of sockets */
    LRE_IO,                         /*!< input output error */
    LRE_ML_BAD,                     /*!< bad metalink file (metalink doesn't
                                         contain needed file) */
    LRE_ML_XML,                     /*!< metalink XML parse error */
    LRE_BAD_CHECKSUM,               /*!< bad checksum */
    LRE_REPOMD_XML,                 /*!< repomd XML parse error */
    LRE_NOURL,                      /*!< no usable URL found */
    LRE_CANNOT_CREATE_TMP,          /*!< cannot create tmp directory */
    LRE_UNKNOWN_CHECKSUM,           /*!< unknown type of checksum is need to
                                         calculate to verify one or more file */
} lr_Rc; /*!< Return codes */

typedef enum {
    LR_CHECK_GPG          = (1<<0),
    LR_CHECK_CHECKSUM     = (1<<1),
} _lr_Checks;

typedef enum {
    LR_YUMREPO     = (1<<1),
    LR_SUSEREPO    = (1<<2),
    LR_DEBREPO     = (1<<3),
} lr_Repotype;

typedef enum {
    LR_YUM_FULL         = (1<<0),
    LR_YUM_REPOMD       = (1<<1),
    LR_YUM_XML_PRI      = (1<<2),
    LR_YUM_XML_FIL      = (1<<3),
    LR_YUM_XML_OTH      = (1<<4),
    LR_YUM_SQL_PRI      = (1<<5),
    LR_YUM_SQL_FIL      = (1<<6),
    LR_YUM_SQL_OTH      = (1<<7),
    LR_YUM_GROUPFILE    = (1<<8),
    LR_YUM_CGROUPFILE   = (1<<9),
    LR_YUM_UPDATEINFO   = (1<<10),
    LR_YUM_PRESTODELTA  = (1<<11),

    // Common combinations
    LR_YUM_BASE_XML     = LR_YUM_XML_PRI|LR_YUM_XML_FIL|LR_YUM_XML_OTH,
    LR_YUM_BASE_HAWKEY  = LR_YUM_XML_PRI|LR_YUM_XML_FIL|LR_YUM_PRESTODELTA,
} lr_YumRepoFlags;

typedef enum {
    LR_URL,         /*!< Base repo URL */
    LR_MIRRORLIST,  /*!< Mirrorlist or metalink url */
    LR_HTTPAUTH,    /*!< Enable all supported method of HTTP
                         authentification. */
    LR_USERPWD,     /*!< User and password for http authetification in format
                         "user:password" */
    LR_PROXY,       /*!< Address of proxy server eg. "proxy-host.com:8080" */
    LR_PROXYPORT,   /*!< Set port number for proxy separately */
    LR_PROXYSOCK,   /*!< Set type of proxy to SOCK (default is assumed
                         HTTP proxy) */
    LR_PROXYAUTH,   /*!< Enable all supported method for proxy
                         authentification */
    LR_PROXYUSERPWD,/*!< User and password for proxy */
    LR_PROGRESSCB,  /*!< Progress callback */
    LR_PROGRESSDATA,/*!< Progress callback user data */
    LR_RETRIES,     /*!< Number of maximum retries for each file */
    LR_MAXSPEED,    /*!< Maximum download speed in bytes per second */
    LR_DESTDIR,     /*!< Where to save downloaded files */

    LR_REPOTYPE,    /*!< Type of downloaded repo, currently only supported
                         is LR_YUMREPO. */

    /* Repo common options */
    LR_GPGCHECK,    /*!< Check GPG signature if available */
    LR_CHECKSUM,    /*!< Check files checksum if available */

    /* LR_YUMREPO specific options */
    LR_YUMDOWNLOADFLAGS, /*!< Download only specified files */

} lr_Option; /*!< Handle config options */

typedef int (*lr_progress_cb)(void *clientp,
                              double total_to_download,
                              double now_downloaded);

struct _lr_Handle {
    CURL            *curl_handle;   /*!< CURL handle */
    char            *baseurl;       /*!< Base URL of repo */
    char            *mirrorlist;    /*!< Mirrorlist or URL */
    char            *used_mirror;   /*!< Finally used mirror (if any) */
    int             retries;        /*!< Number of maximum retries */
    char            *destdir;       /*!< Destination directory */
    lr_Repotype     repotype;       /*!< Type of repository */
    _lr_Checks      checks;         /*!< Which check sould be applied */
    long            status_code;    /*!< Last HTTP or FTP status code */
    CURLcode        last_curl_error;/*!< Last curl error code */
    CURLMcode       last_curlm_error;/*!< Last curl multi handle error code */
    lr_YumRepoFlags yumflags;       /*!< Flags for downloading of yum repo */
    lr_progress_cb  user_cb;        /*!< User progress callback */
    void            *user_data;     /*!< User data for callback */
};
typedef struct _lr_Handle *lr_Handle;

typedef curl_off_t lr_off_t;

lr_Handle lr_init_handle();

void lr_free_handle(lr_Handle handle);

/* look at: url.c - Curl_setopt() */
lr_Rc lr_setopt(lr_Handle handle, lr_Option option, ...);

lr_Rc lr_perform(lr_Handle handle, void **repo_ptr);

CURLcode lr_last_curl_error(lr_Handle);
CURLMcode lr_last_curlm_error(lr_Handle);

/** TODO:
 * - Pri stahovani se budou kontrolovat checksumy, pri praci s lokalnim repem
 *   ale taky.
 */

/* Yum repo */

struct _lr_YumDistroTag {
    char *cpeid;
    char *value;
};
typedef struct _lr_YumDistroTag *lr_YumDistroTag;

struct _lr_YumRepoMdRecord {
    char *location_href;
    char *checksum;
    char *checksum_type;
    char *checksum_open;
    char *checksum_open_type;
    long timestamp;
    long size;
    long size_open;
    int db_version;
};
typedef struct _lr_YumRepoMdRecord *lr_YumRepoMdRecord;

#define NUMBER_OF_YUM_REPOMD_RECORDS    10  /*!< number of repomd records
                                                 in _lr_YumRepoMd structure */

struct _lr_YumRepoMd {
    char *revision;
    char **repo_tags;
    lr_YumDistroTag *distro_tags;
    char **content_tags;

    int nort; /* number of repo tags */
    int nodt; /* number of distro tags */
    int noct; /* number of content tags */

    lr_YumRepoMdRecord pri_xml;
    lr_YumRepoMdRecord fil_xml;
    lr_YumRepoMdRecord oth_xml;
    lr_YumRepoMdRecord pri_sql;
    lr_YumRepoMdRecord fil_sql;
    lr_YumRepoMdRecord oth_sql;
    lr_YumRepoMdRecord groupfile;
    lr_YumRepoMdRecord cgroupfile;
    lr_YumRepoMdRecord deltainfo;
    lr_YumRepoMdRecord updateinfo;
};
typedef struct _lr_YumRepoMd *lr_YumRepoMd;

struct _lr_YumRepo {
    lr_YumRepoMd repomd_obj;

    char *repomd;
    char *pri_xml;
    char *fil_xml;
    char *oth_xml;
    char *pri_sql;
    char *fil_sql;
    char *oth_sql;
    char *groupfile;
    char *cgroupfile;
    char *deltainfo;
    char *updateinfo;

    char *url;          /*!< URL from where repo was downloaded */
    char *destdir;      /*!< Local path to the repo */
};
typedef struct _lr_YumRepo *lr_YumRepo;

void lr_yum_repo_free(lr_YumRepo repo);

#ifdef __cplusplus
}
#endif

#endif
