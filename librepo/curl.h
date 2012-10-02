#ifndef LR_CURL_H
#define LR_CURL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "librepo.h"

/* Return codes of the module:
 *  LRE_OK          everything ok
 *  LRE_IO          input/output error
 *  LRE_NOURL       no usable URL
 *  LRE_CURL_DUP    cannot duplicate curl handle
 *  LRE_CURL        curl err
 *  LRE_MCURL       curl multi handle error
 *  LRE_BAD_STATUS  HTTP or FTP returned status code which
 *                  do not represent success
 */

struct _lr_CurlTarget {
    char *url;
    int fd;
};
typedef struct _lr_CurlTarget * lr_CurlTarget;

lr_CurlTarget lr_target_create();
void lr_target_free(lr_CurlTarget target);

int lr_curl_single_download(lr_Handle handle,
                            const char *url,
                            int fd,
                            char *mandatory_suffix);

int lr_curl_multi_download(lr_Handle handle, lr_CurlTarget targets[], int not);

#ifdef __cplusplus
}
#endif

#endif
