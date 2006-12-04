#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "e_error.h"
#include "llist.h"
#include "xmalloc.h"

void llist_create(llist **l)
{
    (*l) = (llist *)xmalloc(sizeof(llist));
    (*l)->size = 0;
    (*l)->head = NULL;
    (*l)->tail = NULL;
}

void llist_destroy(llist *l)
{
    while (l->size != 0)
        llist_delete(l, l->head->val);
    xfree(l);
}

void llist_insert_head(llist *l, void *val)
{
    llist_entry *e;

    e = (llist_entry *)xmalloc(sizeof(llist_entry));
    e->val = val;
    if (l->size == 0) {
        e->prev = NULL;
        e->next = NULL;
        l->head = e;
        l->tail = e;
    }
    else {
        e->prev = NULL;
        e->next = l->head;
        l->head->prev = e;
        l->head = e;
    }
    l->size++;
}

void llist_insert_tail(llist *l, void *val)
{
    llist_entry *e;

    e = (llist_entry *)xmalloc(sizeof(llist_entry));
    e->val = val;
    if (l->size == 0) {
        e->prev = NULL;
        e->next = NULL;
        l->head = e;
        l->tail = e;
    }
    else {
        e->prev = l->tail;
        e->next = NULL;
        l->tail->next = e;
        l->tail = e;
    }
    l->size++;
}

/* Remove first entry on the list with value val */
int llist_delete(llist *l, void *val)
{
    llist_entry *ei;

    for (ei = l->head; ei != NULL; ei = ei->next) {
        if (ei->val == val) {
            l->size--;
            if (ei == l->head && ei->next == NULL) {
                l->head = NULL;
                l->tail = NULL;
            }
            else if (ei == l->head && ei->next != NULL) {
                ei->next->prev = NULL;
                l->head = ei->next;
                if (l->size == 1)
                    l->tail = l->head;
            }
            else if (ei == l->tail && ei->prev == NULL) {
                l->head = NULL;
                l->tail = NULL;
            }
            else if (ei == l->tail && ei->prev != NULL) {
                ei->prev->next = NULL;
                l->tail = ei->prev;
                if (l->size == 1)
                    l->head = l->tail;
            }
            else {
                ei->prev->next = ei->next;
                ei->next->prev = ei->prev;
            }
            xfree(ei);
            return 1;
        }
    }
    return 0;
}

void llist_sort(llist *l, int (*compare)(void *, void *))
{
    llist_entry *lle1, *lle2;
    void        *tmp_val;

    for (lle1 = l->head; lle1 != NULL; lle1 = lle1->next) {
        for (lle2 = lle1->next; lle2 != NULL; lle2 = lle2->next) {
            if (compare(lle1->val, lle2->val) == 1) {
                tmp_val   = lle1->val;
                lle1->val = lle2->val;
                lle2->val = tmp_val;
            }
        }
    }
}
