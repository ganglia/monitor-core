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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bytes.h"
#include "e_error.h"
#include "hash.h"
#include "safe.h"
#include "tree.h"
#include "xmalloc.h"

static void compute_parents(int *parents, int nhosts, int fanout)
{
    int i, j, c;

    parents[0] = -1;
    for (i = 0; i < nhosts; i++) {
        for (c = 1; c <= fanout; c++) {
            j = i * fanout + c;
            if (j >= nhosts)
                return;
            parents[j] = i;
        }
    }
}

static int has_cycle(tree *t)
{
    int        i, klen, cycle = 0; 
    int        present = 1;
    void       *key;
    hash_table *hash;

    /* Checks for duplicates in [1,n-1] (ignores root) */
    hash_table_create(&hash, 191);
    for (i = 1; i < t->nhosts; i++) {
        key  = t->ips[i];
        klen = strlen(t->ips[i]) + NULLBYTE_SIZE;
        if (!hash_table_insert(hash, key, klen, &present, sizeof(int))) {
            cycle = 1;
            break;
        }
    }
    hash_table_destroy(hash);
     return (!cycle) ? E_OK : E_GRAPH_CYCLE_ERROR;
}


int tree_create(tree **t, int nhosts, char **ips, int fanout, int dup_ips)
{
    int i;

    (*t) = (tree *)xmalloc(sizeof(tree));
    (*t)->nhosts  = nhosts;
    (*t)->ips     = (char **)xmalloc(nhosts * sizeof(char *));
    (*t)->parents = (int *)xmalloc(nhosts * sizeof(int));
    (*t)->fanout  = fanout;
    for (i = 0; i < nhosts; i++) {
        (*t)->ips[i] = (char *)xmalloc(IP_STRLEN);
        memcpy((*t)->ips[i], ips[i], IP_STRLEN);
    }
    compute_parents((*t)->parents, (*t)->nhosts, (*t)->fanout);
    if (!dup_ips && has_cycle(*t))
        goto cleanup;
    return E_OK;

 cleanup:
    for (i = 0; i < nhosts; i++)
        xfree((*t)->ips[i]);
    xfree((*t)->parents);
    xfree((*t)->ips);
    xfree((*t));
    *t = NULL;
    return E_GRAPH_CYCLE_ERROR;
}

int tree_create_from_buf(tree **t, void *buf, int buf_len, int dup_ips)
{
    int  i, j, len, plen;
    void *p;

    (*t) = (tree *)xmalloc(sizeof(tree));
    p    = buf;
    plen = buf_len;

    /* Copy nhosts */
    len = sizeof((*t)->nhosts);
    if (safe_memcpy(&(*t)->nhosts, p, len, plen) != E_OK)
        goto cleanup0;
    p    += len;
    plen -= len;

    /* Copy ips */
    (*t)->ips = (char **)xmalloc((*t)->nhosts * sizeof(char *));
    for (i = 0; i < (*t)->nhosts; i++) {
        (*t)->ips[i] = (char *)xmalloc(IP_STRLEN);
        if (safe_memcpy((*t)->ips[i], p, IP_STRLEN, plen) != E_OK)
            goto cleanup1;
        p    += IP_STRLEN;
        plen -= IP_STRLEN;
    }

    /* Copy parents */
    len = (*t)->nhosts * sizeof(int);
    (*t)->parents = (int *)xmalloc(len);    
    if (safe_memcpy((*t)->parents, p, len, plen) != E_OK)
        goto cleanup2;
    p    += len;
    plen -= len;

    /* Copy fanout */
    len = sizeof((*t)->fanout);
    if (safe_memcpy(&(*t)->fanout, p, len, plen) != E_OK)
        goto cleanup2;
    p    += len;
    plen -= len;

    if (!dup_ips && has_cycle(*t))
        goto cleanup2;
    e_assert(plen == 0);
    return E_OK;

 cleanup2:
    xfree((*t)->parents);
 cleanup1:
    for (j = 0; j < i; j++)
        xfree((*t)->ips[j]);
    xfree((*t)->ips);
 cleanup0:
    xfree((*t));
    *t = NULL;
    return E_GRAPH_CYCLE_ERROR;
}

void *tree_malloc_buf(tree *t, int *buf_len)
{
    int  i;
    void *buf, *p;

    *buf_len = 
        sizeof(t->nhosts) + t->nhosts * IP_STRLEN + 
        t->nhosts * sizeof(int) + sizeof(t->fanout);

    p = buf = (void *)xmalloc(*buf_len);

    memcpy(p, &t->nhosts, sizeof(t->nhosts));
    p += sizeof(t->nhosts);

    for (i = 0; i < t->nhosts; i++) {
        memcpy(p, t->ips[i], IP_STRLEN);
        p += IP_STRLEN;
    }

    memcpy(p, t->parents, t->nhosts * sizeof(int));
    p += t->nhosts * sizeof(int);

    memcpy(p, &t->fanout, sizeof(t->fanout));
    p += sizeof(t->fanout);

    return buf;
}

void tree_destroy(tree *t)
{
    int i;

    xfree(t->parents);
    for (i = 0; i < t->nhosts; i++) 
        xfree(t->ips[i]);
    xfree(t->ips);
    xfree(t);
}

int tree_parent(tree *t, int index)
{
    return t->parents[index];
}

int tree_child(tree *t, int index, int child_num)
{
    int first, child;

    first = (index * t->fanout) + 1;
    first = (first < t->nhosts) ? first : -1;
    if (first == -1)
        return -1;

    child = first + child_num;
    return (child < t->nhosts) ? child : -1;
}

int tree_num_children(tree *t, int index)
{
    int first, last;
    
    first = (index * t->fanout) + 1;
    first = (first < t->nhosts) ? first : -1;
    if (first == -1)
        return 0;

    last = first + (t->fanout - 1);
    last = (last < t->nhosts) ? last : t->nhosts - 1;
    return (last - first + 1);
}
