/*
 * "Copyright (c) 1999 by Brent N. Chun and The Regents of the University 
 * of California.  All rights reserved."
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */
#ifndef LLIST_H
#define LLIST_H 1

/* programmer manages setting/storage for val */
typedef struct _llist_entry {
    struct _llist_entry     *prev;  /* Previous entry on list   */
    struct _llist_entry     *next;  /* Next entry on list       */
    void                    *val;   /* Entry value              */
} llist_entry;

void llist_add(llist_entry **llist, llist_entry *e);
int llist_remove(llist_entry **llist, llist_entry *e);
int llist_search(llist_entry **llist, void *val, 
                 int (*compare_function)(const char *,const char *), 
                 llist_entry **e);
int llist_sort(llist_entry *llist, int (*compare_function)(llist_entry *, llist_entry *));
int llist_print(llist_entry **llist);
#endif /* LLIST_H */ 
