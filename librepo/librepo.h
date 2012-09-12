#ifndef LR_LIBREPO_H
#define LR_LIBREPO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <curl/curl.h>

typedef enum {
    LR_OK,                      /*!< everything is ok */
    LR_BAD_FUNCTION_ARGUMENT,   /*!< bad function argument */

    /* lr_setopt specific */
    LR_BAD_OPTION_ARGUMENT,     /*!< bad argument of the option*/
    LR_UNKNOWN_OPTION,          /*!< library doesn't know the option */
    LR_CURL_SETOPT_ERROR,       /*!< cURL doesn't know the option.
                                     Too old curl version? */
    LR_CURL_ERROR,              /*!< other cURL error. Use the
                                     lr_last_curl_error to get CURLcode */

    /* lr_perform specific */
    LR_NOURL_SPECIFIED,         /*!< no base url or mirrorlist url specified */
    LR_CANNOT_CREATE_TMP,       /*!< cannot create tmp directory */

    /* lr_perform - metalink specific */
    LR_IO_CANNOT_READ,          /*!< cannot read a file descriptor */
    LR_METALINK_PARSE_ERROR,
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

struct _lr_Handle {
    CURL            *curl_handle;   /*!< CURL handle */
    char            *baseurl;       /*!< Base URL of repo */
    char            *mirrorlist;    /*!< Mirrorlist or URL */
    int             retries;        /*!< Number of maximum retries */
    char            *destdir;       /*!< Destination directory */
    lr_Repotype     repotype;       /*!< Type of repository */
    _lr_Checks      checks;         /*!< Which check sould be applied */
    CURLcode        last_curl_error;/*!< Last curl error code */
    lr_YumRepoFlags yumflags;       /*!< Flags for downloading of yum repo */
};
typedef struct _lr_Handle *lr_Handle;

typedef curl_off_t lr_off_t;

typedef int (*lr_progress_cb)(void *clientp,
                              double dltotal,
                              double dlnow,
                              double ultotal,
                              double ulnow);

lr_Handle lr_init_handle();

void lr_free_handle(lr_Handle handle);

/* look at: url.c - Curl_setopt() */
lr_Rc lr_setopt(lr_Handle handle, lr_Option option, ...);

lr_Rc lr_perform(lr_Handle handle);

CURLcode lr_last_curl_error(lr_Handle);

/** TODO:
 * - Pri stahovani se budou kontrolovat checksumy, pri praci s lokalnim repem
 *   ale taky.
 */

/* Yum repo */

struct _lr_RepoMdRecod {
    char *location_href;
    char *checksum;
    char *checksum_type;
    char *checksum_open;
    char *checksum_open_type;
    unsigned long timestamp;
    unsigned long size;
    unsigned long size_open;
    int db_version;
};
typedef struct _lr_RepoMdRecord *lr_RepoMdRecord;

struct _lr_RepoMd {
    char *revision;
    char **repo_tags;
    char **distro_tags;
    char **content_tags;

    lr_RepoMdRecord pri_xml;
    lr_RepoMdRecord fil_xml;
    lr_RepoMdRecord oth_xml;
    lr_RepoMdRecord pri_sql;
    lr_RepoMdRecord fil_sql;
    lr_RepoMdRecord oth_sql;
    lr_RepoMdRecord groupfile;
    lr_RepoMdRecord cgroupfile;
    lr_RepoMdRecord updateinfo;
};
typedef struct _lr_RepoMd *lr_RepoMd;

struct _lr_YumRepo {
    lr_RepoMd repomd_obj;

    char *repomd;
    char *pri_xml;
    char *fil_xml;
    char *oth_xml;
    char *pri_sql;
    char *fil_sql;
    char *oth_sql;
    char *groupfile;
    char *cgroupfile;
    char *updateinfo;

    char *url;          /*!< URL from where repo was downloaded */
    char *destdir;      /*!< Local path to the repo */
};
typedef struct _lr_YumRepo *lr_YumRepo;

#ifdef __cplusplus
}
#endif

#endif
