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
#ifndef __HASH_H
#define __HASH_H

/* Programmer manages storage for key, val */
typedef struct _hash_table_entry {
    void                        *key;
    int                         klen;
    void                        *val;
    int                         vlen;
    struct _hash_table_entry    *prev;
    struct _hash_table_entry    *next;
} hash_table_entry;

typedef struct _hash_table {
    int                 size;
    hash_table_entry    **chains;
} hash_table;

void hash_table_create(hash_table **t, int size);
void hash_table_destroy(hash_table *t);
int  hash_table_insert(hash_table *t, void *key, int klen, void *val, int vlen);
int  hash_table_delete(hash_table *t, void *key, int klen);
int  hash_table_search(hash_table *t, void *key, int klen, void **val, int *vlen);

#endif /* __HASH_H */ 
