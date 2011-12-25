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
#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include "llist.h"

/*
 * ==========================================================
 *
 * Exported functions
 *
 * ==========================================================
 */

/* llist_add: add e to list llist */
void llist_add(llist_entry **llist, llist_entry *e)
{
    if ((*llist) != NULL) {
        e->prev = NULL;
        e->next = (*llist);
        (*llist)->prev = e;
        (*llist) = e;
    }
    else {
        e->prev = NULL;
        e->next = NULL;
        (*llist) = e;
    }
}

/* llist_remove: remove e from list llist */
int llist_remove(llist_entry **llist, llist_entry *e)
{
    llist_entry *ei;

    for (ei = (*llist); ei != NULL; ei = ei->next) {
        if (ei == e) {
            if ((e == (*llist)) && (e->next == NULL)) { 
                (*llist) = NULL;                
            }
            else if ((e == (*llist)) && (e->next != NULL)) { 
                e->next->prev = NULL;
                (*llist) = e->next;
            }
            else if (e->next == NULL) {
                e->prev->next = NULL;
            }
            else {
                e->prev->next = e->next;
                e->next->prev = e->prev;
            }
            return 0;
        }
    }
    return -1;
}

/* 
 * llist_search: search for entry with val that matches
 * according to compare_function in list llist. Return
 * match in e.
 */
int llist_search(llist_entry **llist, void *val, 
                 int (*compare_function)(const char *, const char *), 
                 llist_entry **e)
{
    llist_entry *ei;

    for (ei = (*llist); ei != NULL; ei = ei->next)
        if (compare_function(ei->val, val) == 0) {
            (*e) = ei;
            return 0;
        }
    return -1;
}

int
llist_print(llist_entry **llist)
{
   llist_entry *ei;

   for(ei = (*llist); ei != NULL; ei = ei->next)
      {
         printf("%s\n", (char *)ei->val);
      }
   return 0;
}
   

int
llist_sort(llist_entry *llist, int (*compare_function)(llist_entry *, llist_entry *))
{
    llist_entry     *lle1, *lle2;
    void            *tmp_val;

    for (lle1 = llist; lle1 != NULL; lle1 = lle1->next) {
        for (lle2 = lle1->next; lle2 != NULL; lle2 = lle2->next) {
            if (compare_function(lle1, lle2) == 1) {
                tmp_val = lle1->val;
                lle1->val = lle2->val;
                lle2->val = tmp_val;
            }
        }
    }
    return 0;
}

