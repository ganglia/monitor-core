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
#ifndef __BITMASK_H
#define __BITMASK_H

#define BITMASK_CHUNK_SIZE  (sizeof(int))

/* Bitmasks are multiple of 32 bits */
#define BITMASK_CHUNK       (BITMASK_CHUNK_SIZE * 8)

void bitmask_reset(int *mask, int mask_nbits);
void bitmask_set(int *mask, int mask_nbits, int bit);
void bitmask_clr(int *mask, int mask_nbits, int bit);
int  bitmask_isset(int *mask, int mask_nbits, int bit);
int  bitmask_ffs(int *mask, int mask_nbits);
void bitmask_and(int *dst_mask, int *src_mask, int mask_nbits);
void bitmask_or(int *dst_mask, int *src_mask, int mask_nbits);
void bitmask_shift(int *mask, int mask_nbits, int shift);
void bitmask_copy(int *dst_mask, int *src_mask, int mask_nbits);
void bitmask_print(int *mask, int mask_nbits);

#endif /* __BITMASK_H */
