/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#ifndef __XMALLOC_H
#define __XMALLOC_H

#include <stdlib.h>

#define xmalloc(__size) \
       _xmalloc(__size, __FILE__, __LINE__)
#define xfree(__ptr) \
       _xfree(__ptr, __FILE__, __LINE__)
#define xcalloc(__nmemb, __size) \
       _xcalloc(__nmemb, __size, __FILE__, __LINE__)
#define xrealloc(__ptr, __size) \
       _xrealloc(__ptr, __size, __FILE__, __LINE__)

void *_xmalloc(size_t size, char *file, int line);
void _xfree(void *ptr, char *file, int line);
void *_xcalloc(size_t nmemb, size_t size, char *file, int line);
void *_xrealloc(void *ptr, size_t size, char *file, int line);

#endif /* __XMALLOC_H */
