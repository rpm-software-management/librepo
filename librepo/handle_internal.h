#ifndef LR_HANDLE_INTERNAL_H
#define LR_HANDLE_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

struct _lr_Handle {
    CURL            *curl_handle;   /*!< CURL handle */
    int             update;         /*!< Just update existing repo */
    char            *baseurl;       /*!< Base URL of repo */
    char            *mirrorlist;    /*!< Mirrorlist or URL */
    int             local;          /*!< Do not duplicate local data */
    char            *used_mirror;   /*!< Finally used mirror (if any) */
    int             retries;        /*!< Number of maximum retries */
    char            *destdir;       /*!< Destination directory */
    lr_Repotype     repotype;       /*!< Type of repository */
    lr_Checks       checks;         /*!< Which check sould be applied */
    long            status_code;    /*!< Last HTTP or FTP status code */
    CURLcode        last_curl_error;/*!< Last curl error code */
    CURLMcode       last_curlm_error;/*!< Last curl multi handle error code */
    lr_YumRepoFlags yumflags;       /*!< Flags for downloading of yum repo */
    lr_ProgressCb   user_cb;        /*!< User progress callback */
    void            *user_data;     /*!< User data for callback */
};

struct _lr_Result {
    char            *destdir;
    lr_YumRepoMd    yum_repomd;     /* pointer to struct representingrepomd.xml */
    lr_YumRepo      yum_repo;       /* pointer to struct with info about yum repo */
};

#ifdef __cplusplus
}
#endif

#endif
