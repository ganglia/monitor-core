/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "e_error.h"

/* Ensures that src has at least len bytes left */
int safe_memcpy(void *dst, void *src, int len, int src_len)
{
    int new_src_len;

    new_src_len = src_len - len;
    if (new_src_len < 0)
        return E_MEMLEN_ERROR;
    memcpy(dst, src, len);
    return E_OK;
}
