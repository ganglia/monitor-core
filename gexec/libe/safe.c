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
