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
#include <string.h>
#include "bitmask.h"
#include "e_error.h"
#include "xmalloc.h"

void bitmask_reset(int *mask, int mask_nbits)
{
    int i, nchunks;

    e_assert((mask_nbits % BITMASK_CHUNK) == 0);
    nchunks = mask_nbits / BITMASK_CHUNK;
    for (i = 0; i < nchunks; i++)
        mask[i] = 0x00000000;
}

void bitmask_set(int *mask, int mask_nbits, int bit)
{
    int i, shift, nchunks;

    if (bit < 0 || bit >= mask_nbits)
        return;
    e_assert((mask_nbits % BITMASK_CHUNK) == 0);
    nchunks = mask_nbits / BITMASK_CHUNK;
    i     = bit / BITMASK_CHUNK;
    shift = bit % BITMASK_CHUNK;
    mask[i] |= (1 << shift);
}

void bitmask_clr(int *mask, int mask_nbits, int bit)
{
    int i, shift, nchunks;

    if (bit < 0 || bit >= mask_nbits)
        return;
    e_assert((mask_nbits % BITMASK_CHUNK) == 0);
    nchunks = mask_nbits / BITMASK_CHUNK;
    i     = bit / BITMASK_CHUNK;
    shift = bit % BITMASK_CHUNK;
    mask[i] &= (~(1 << shift));
}

int bitmask_isset(int *mask, int mask_nbits, int bit)
{
    int i, shift, nchunks;

    if (bit >= mask_nbits)
        return 0;
    e_assert((mask_nbits % BITMASK_CHUNK) == 0);
    nchunks = mask_nbits / BITMASK_CHUNK;
    i     = bit / BITMASK_CHUNK;
    shift = bit % BITMASK_CHUNK;
    return ((mask[i] & (1 << shift)) != 0);
}

int bitmask_ffs(int *mask, int mask_nbits)
{
    int i, nchunks, n;

    /* 
     * If string.h's ffs() isn't fast enough, we can always break ints
     * down into bytes and use a static lookup table which maps bytes
     * to first bit set (probably what ffs uses anyway).
     */
    e_assert((mask_nbits % BITMASK_CHUNK) == 0);
    nchunks = mask_nbits / BITMASK_CHUNK;
    for (i = 0; i < nchunks; i++) {
        n = ffs(mask[i]);
        if (n != (int)NULL)
            return ((i * BITMASK_CHUNK) + (n - 1));
    }
    return -1;
}

void bitmask_and(int *dst_mask, int *src_mask, int mask_nbits)
{
    int i, nchunks;

    e_assert((mask_nbits % BITMASK_CHUNK) == 0);
    nchunks = mask_nbits / BITMASK_CHUNK;    
    for (i = 0; i < nchunks; i++)
        dst_mask[i] &= src_mask[i];
}

void bitmask_or(int *dst_mask, int *src_mask, int mask_nbits)
{
    int i, nchunks;

    e_assert((mask_nbits % BITMASK_CHUNK) == 0);
    nchunks = mask_nbits / BITMASK_CHUNK;    
    for (i = 0; i < nchunks; i++)
        dst_mask[i] |= src_mask[i];
}

void bitmask_shift(int *mask, int mask_nbits, int shift)
{
    int bit, newbit, nchunks;
    int *mask_copy;

    e_assert((mask_nbits % BITMASK_CHUNK) == 0);
    nchunks = mask_nbits / BITMASK_CHUNK;
    mask_copy = (int *)xmalloc(nchunks * BITMASK_CHUNK_SIZE);
    memcpy(mask_copy, mask, nchunks * BITMASK_CHUNK_SIZE);
    bitmask_reset(mask, mask_nbits);
    while ((bit = bitmask_ffs(mask_copy, mask_nbits)) != -1) {
        newbit = bit + shift;
        bitmask_set(mask, mask_nbits, newbit);
        bitmask_clr(mask_copy, mask_nbits, bit);
    }
    xfree(mask_copy);
}

void bitmask_copy(int *dst_mask, int *src_mask, int mask_nbits)
{
    int nchunks;

    e_assert((mask_nbits % BITMASK_CHUNK) == 0);
    nchunks = mask_nbits / BITMASK_CHUNK;
    memcpy(dst_mask, src_mask, nchunks * BITMASK_CHUNK_SIZE);
}

void bitmask_print(int *mask, int mask_nbits)
{
    int i, nchunks;

    e_assert((mask_nbits % BITMASK_CHUNK) == 0);
    nchunks = mask_nbits / BITMASK_CHUNK;
    for (i = nchunks - 1; i >= 0; i--) {
        printf("%08x", mask[i]);
        if ((nchunks % 10) == 0)
            printf("\n");
    }
    printf("\n");
}
