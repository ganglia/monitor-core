/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
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
