#ifndef __LLIST_H
#define __LLIST_H

/* Programmer manages storage for val */
typedef struct _llist_entry {
    struct _llist_entry *prev;
    struct _llist_entry *next;
    void   *val;
} llist_entry;

typedef struct _llist {
    int         size;
    llist_entry *head;
    llist_entry *tail;
} llist;

void llist_create(llist **l);
void llist_destroy(llist *l);
void llist_insert_head(llist *l, void *val);
void llist_insert_tail(llist *l, void *val);
int  llist_delete(llist *l, void *val);
void llist_sort(llist *l, int (*compare)(void *, void *));

#endif /* __LLIST_H */
