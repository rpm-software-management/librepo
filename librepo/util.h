#ifndef LR_UTIL_H
#define LR_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

void lr_out_of_memory();
void *lr_malloc(size_t len);
void *lr_malloc0(size_t len);
void *lr_realloc(void *ptr, size_t len);
void lr_free(void *mem);
char *lr_strdup(const char *str);
int lr_gettmpfile();

#ifdef __cplusplus
}
#endif

#endif
