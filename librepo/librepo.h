#ifndef LR_LIBREPO_H
#define LR_LIBREPO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <curl/curl.h>

#include "types.h"

/* Return/Error codes */
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
    LRE_MUSTDUP,                    /*!< DONTDUP option used, but the URL is not
                                         a local address */
    LRE_SELECT,                     /*!< error while call select() on set
                                         of sockets */
    LRE_CANNOT_CREATE_DIR,          /*!< cannot create a directory in output
                                         dir (the directory already exists?) */
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
    LRE_UNKNOWN_ERROR,              /*!< unknown error - last element in
                                         error codes enum */
} lr_Rc; /*!< Return codes */

typedef enum {
    LR_URL,         /*!< Base repo URL */
    LR_MIRRORLIST,  /*!< Mirrorlist or metalink url */
    LR_DONTDUP,     /*!< Do not duplicate local metadata, just locate
                         the old one */
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
    LR_RETRIES,     /*!< Number of maximum retries for each file - TODO */
    LR_MAXSPEED,    /*!< Maximum download speed in bytes per second */
    LR_DESTDIR,     /*!< Where to save downloaded files */

    LR_REPOTYPE,    /*!< Type of downloaded repo, currently only supported
                         is LR_YUMREPO. */

    /* Repo common options */
    LR_GPGCHECK,    /*!< Check GPG signature if available - TODO */
    LR_CHECKSUM,    /*!< Check files checksum if available */

    /* LR_YUMREPO specific options */
    LR_YUMREPOFLAGS,/*!< Download only specified files */

} lr_Option; /*!< Handle config options */

void lr_global_init();
void lr_global_cleanup();

lr_Handle lr_handle_init();
void lr_handle_free(lr_Handle handle);

/* look at: url.c - Curl_setopt() */
int lr_setopt(lr_Handle handle, lr_Option option, ...);
int lr_perform(lr_Handle handle, void *repo_ptr);
int lr_last_curl_error(lr_Handle);
int lr_last_curlm_error(lr_Handle);

/* Yum repo */

lr_YumRepo lr_yum_repo_create();
void lr_yum_repo_free(lr_YumRepo repo);

#ifdef __cplusplus
}
#endif

#endif
