/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "e_error.h"
#include "hash.h"
#include "xmalloc.h"

/* chain_add: add e to hash table chain */
static void chain_add(hash_table_entry **chain, hash_table_entry *e)
{
    if ((*chain) != NULL) {
        e->prev = NULL;
        e->next = (*chain);
        (*chain)->prev = e;
        (*chain) = e;
    }
    else {
        e->prev = NULL;
        e->next = NULL;
        (*chain) = e;
    }
}

/* chain_remove: remove e from hash table chain (0 if not in hash) */
static int chain_remove(hash_table_entry **chain, hash_table_entry *e)
{
    hash_table_entry *ei;

    for (ei = (*chain); ei != NULL; ei = ei->next) {
        if (ei == e) {
            if ((e == (*chain)) && (e->next == NULL)) { 
                (*chain) = NULL;                
            }
            else if ((e == (*chain)) && (e->next != NULL)) { 
                e->next->prev = NULL;
                (*chain) = e->next;
            }
            else if (e->next == NULL) {
                e->prev->next = NULL;
            }
            else {
                e->prev->next = e->next;
                e->next->prev = e->prev;
            }
            xfree(e);
            return 1;
        }
    }
    return 0;
}

/* 
 * chain_search: search for entry with key as its its key and return in e 
 * returns 1 if found, else 0
 */
static int chain_search(hash_table_entry *chain, void *key, int klen,
                        hash_table_entry **e)
{
    hash_table_entry *ei;

    for (ei = chain; ei != NULL; ei = ei->next) {
        if ((ei->klen == klen) && memcmp(ei->key, key, klen) == 0) {
            (*e) = ei;
            return 1;
        }
    }
    return 0;
}

/* hash: some hash function */
static int hash(hash_table *t, void *key, int klen)
{
    int i;
    int hash;

    if (klen == 0)
        return 0;

    hash = ((unsigned char *)key)[0];
    for (i = 1; i < klen; i++)
        hash = (hash * 32 + ((unsigned char *)key)[i]) % t->size;
    return hash;
}

/* hash_table_create: init & allocate memory for pointers to chains */ 
void hash_table_create(hash_table **t, int size)
{
    int i;

    (*t) = xmalloc(sizeof(hash_table));
    (*t)->size   = size;
    (*t)->chains = xmalloc(size * sizeof(hash_table_entry *));
    for (i = 0; i < size; i++)
        (*t)->chains[i] = NULL;
}

/* hash_table_destroy: clean out hash table, free pointers to chains */
void hash_table_destroy(hash_table *t)
{
    int                 i;
    hash_table_entry    *e, *temp;

    for (i = 0; i < t->size; i++)
        for (e = t->chains[i]; e != NULL; ) {
            temp = e->next;
            xfree(e);
            e = temp;
        }
    xfree(t->chains);
    xfree(t);
}

/* 
 * hash_table_insert: insert e into hash table t (no duplicates allowed)
 * NOTE: programmer must allocate storage for key, val, fill in klen 
 */
int hash_table_insert(hash_table *t, void *key, int klen, void *val, int vlen)
{
    int                 i, found;
    hash_table_entry    *e;

    i = hash(t, key, klen);
    found = chain_search(t->chains[i], key, klen, &e);
    if (found)
        return 0;

    e = (hash_table_entry *)xmalloc(sizeof(hash_table_entry));
    e->key  = key;
    e->klen = klen;
    e->val  = val;
    e->vlen = vlen;
    chain_add(&t->chains[i], e);
    return 1;
}

/*
 * hash_table_delete: delete e from hash table t
 * NOTE: programmer frees storage for key, val
 */
int hash_table_delete(hash_table *t, void *key, int klen)
{
    int                 i, found;
    hash_table_entry    *e;

    i = hash(t, key, klen);
    found = chain_search(t->chains[i], key, klen, &e);
    if (!found)
        return 0;
    return chain_remove(&t->chains[i], e);
}

/*
 * hash_table_search: search for entry with key as its key,
 * return in e.
 */
int hash_table_search(hash_table *t, void *key, int klen, void **val, int *vlen)
{
    int                 i, found;
    hash_table_entry    *e;

    i = hash(t, key, klen);
    found = chain_search(t->chains[i], key, klen, &e);
    if (found) {
        *val  = e->val;
        *vlen = e->vlen;
        return 0;
    }
    else {
        *val  = NULL;
        *vlen = 0;
        return -1;
    }
}
