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
#ifndef __TREE_H
#define __TREE_H

/* www.xxx.yyy.zzz (max IPV4 address + NULL byte) */
#define IP_STRLEN   16

typedef struct {
    int  nhosts;
    char **ips;
    int  *parents;
    int  fanout;
} tree;

int  tree_create(tree **t, int nhosts, char **ips, int fanout, int dup_ips);
int  tree_create_from_buf(tree **t, void *buf, int buf_len, int dup_ips);
void *tree_malloc_buf(tree *t, int *buf_len);
void tree_destroy(tree *t);

int  tree_parent(tree *t, int node);
int  tree_child(tree *t, int node, int child_num);
int  tree_num_children(tree *t, int node);

#define ROOT  0

#endif /* __TREE_H */

