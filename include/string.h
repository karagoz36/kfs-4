/* string.h: there is no libc, so the few functions the kernel needs live here. */

#ifndef STRING_H
#define STRING_H

#include "types.h"

size_t  k_strlen(const char *s);
int     k_strcmp(const char *a, const char *b);
void   *k_memcpy(void *dst, const void *src, size_t n);

#endif
