/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
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
