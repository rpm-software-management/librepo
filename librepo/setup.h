#ifndef LR_SETUP_H
#define LR_SETUP_H

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG

#ifdef DEBUG
#define DEBUGF(x) x
#else
#define DEBUGF(x) do {} while(0)  /* Just to force write ';' after DEBUGF() */
#endif

/* DEBUGASSERT is only for debuging.
 * For assertion which shoud be always valid assert() is used directly.
 */
#ifdef DEBUG
#include <assert.h>
#define DEBUGASSERT(x) assert(x)
#else
#define DEBUGASSERT(x) do {} while(0)
#endif

#ifdef __cplusplus
}
#endif

#endif
