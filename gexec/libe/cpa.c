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
#include <string.h>
#include "bytes.h"
#include "cpa.h"
#include "e_error.h"
#include "safe.h"
#include "xmalloc.h"

void cpa_create(char ***dst_array, int *dst_array_size,
                char **src_array, int src_array_size)
{
    int i, elem_len;

    *dst_array      = (char **)xmalloc((src_array_size + 1 )* sizeof(char *));
    *dst_array_size = src_array_size;

    for (i = 0; i < *dst_array_size; i++) {
        if (src_array[i] != NULL) {
            elem_len = strlen(src_array[i]) + NULLBYTE_SIZE;
            (*dst_array)[i] = (char *)xmalloc(elem_len);
            memcpy((*dst_array)[i], src_array[i], elem_len);
        }
        else
            (*dst_array)[i] = NULL;
    }
    (*dst_array)[i] = NULL;
}

int cpa_create_from_buf(char ***array, int *array_size, void *buf, int buf_len)
{
    void *p;
    int  i, elem_len, plen;

    *array      = NULL;
    *array_size = 0;

    p    = buf;
    plen = buf_len;
    while ((p - buf) < buf_len) {
        if (safe_memcpy(&elem_len, p, sizeof(elem_len), plen) != E_OK)
            goto cleanup;
        p    += sizeof(elem_len);
        plen -= sizeof(elem_len);
        p    += elem_len;
        plen -= elem_len;
        (*array_size)++;
    }
    (*array) = (char **)xmalloc(((*array_size) + 1) * sizeof(char *));

    p    = buf;
    plen = buf_len;
    for (i = 0; i < *array_size; i++) {
        if (safe_memcpy(&elem_len, p, sizeof(elem_len), plen) != E_OK)
            goto cleanup;
        p    += sizeof(elem_len);
        plen -= sizeof(elem_len);
        if (elem_len != 0) {
            (*array)[i] = (char *)xmalloc(elem_len);
            if (safe_memcpy((*array)[i], p, elem_len, plen) != E_OK)
                goto cleanup;
        }
        else
            (*array)[i] = NULL;
        p    += elem_len;
        plen -= elem_len;
    }
    (*array)[i] = NULL;
    e_assert(plen == 0);
    return E_OK;
    
 cleanup:
    if (*array != NULL) {
        for (i = 0; i < *array_size; i++)
            if ((*array)[i] != NULL)
                xfree((*array)[i]);
        xfree(*array);
    }
    *array_size = 0;
    return E_MEMLEN_ERROR;
}

void *cpa_malloc_buf(char **array, int array_size, int *buf_len)
{
    int  i, elem_len;
    void *buf, *p;

    *buf_len = 0;
    for (i = 0; i < array_size; i++)
        if (array[i] != NULL)
            *buf_len += sizeof(int) + strlen(array[i]) + NULLBYTE_SIZE;
        else
            *buf_len += sizeof(int);
    buf = xmalloc(*buf_len);

    p = buf;
    for (i = 0; i < array_size; i++) {
        if (array[i] != NULL) {
            elem_len = strlen(array[i]) + NULLBYTE_SIZE;
            memcpy(p, &elem_len, sizeof(elem_len));
            p += sizeof(elem_len);
            memcpy(p, array[i], elem_len);
            p += elem_len;
        }
        else {
            elem_len = 0;
            memcpy(p, &elem_len, sizeof(elem_len));
            p += sizeof(elem_len);
        }
    }
    return buf;
}

void cpa_destroy(char **array, int array_size)
{
    int i;

    for (i = 0; i < array_size; i++)
        xfree(array[i]);
    xfree(array);
}
