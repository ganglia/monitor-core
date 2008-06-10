/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
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
