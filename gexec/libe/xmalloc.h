/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
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
