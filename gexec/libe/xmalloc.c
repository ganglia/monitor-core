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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "e_error.h"

void *_xmalloc(size_t size, char *file, int line)
{
    void *ptr;

    if (size <= 0) {
        fatal_error_long("xmalloc error\n", file, line);
        exit(1);
    } 
    ptr = malloc(size);
    if (ptr == NULL) {
        fatal_error_long("xmalloc error\n", file, line);
        exit(1);
    }
    return ptr;
}

void _xfree(void *ptr, char *file, int line)
{
    if (ptr == NULL) {
        fatal_error_long("xfree error\n", file, line);
        exit(1);
    }
    free(ptr);
}

void *_xcalloc(size_t nmemb, size_t size, char *file, int line)
{
    void *ptr;

    if (nmemb <= 0 || size <= 0) {
        fatal_error_long("xcalloc error\n", file, line);
        exit(1);
    }
    ptr = calloc(nmemb, size);
    if (ptr == NULL) {
        fatal_error_long("xcalloc error\n", file, line);
        exit(1);
    }
    return ptr;
}

void *_xrealloc(void *ptr, size_t size, char *file, int line)
{
    void *newptr;

    if (ptr == NULL) {
        fatal_error_long("xrealloc error\n", file, line);
        exit(1);
    }
    newptr = realloc(ptr, size);
    if (newptr == NULL) {
        fatal_error_long("xrealloc error\n", file, line);
        exit(1);
    }
    return newptr;
}
