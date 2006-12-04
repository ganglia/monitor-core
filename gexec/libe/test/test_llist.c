#include <stdio.h>
#include <stdlib.h>
#include "llist.h"
#include "xmalloc.h"

#define NVALS  10

int val_compare(void *p1, void *p2)
{   
    int n1, n2;
    
    n1 = *((int *)p1);
    n2 = *((int *)p2);
    if (n1 < n2)
        return -1;
    else if (n1 > n2)
        return 1;
    else 
        return 0;
}

int main(int argc, char **argv)
{
    int         i, v, *vals;
    llist       *ll;
    llist_entry *lle;

    srand(0);
    vals = xmalloc(NVALS * sizeof(int));
    for (i = 0; i < NVALS; i++)
        vals[i] = rand();

    llist_create(&ll);
    for (i = 0; i < NVALS; i++)
        llist_insert_tail(ll, (void *)&vals[i]);
    llist_sort(ll, val_compare);
    for (lle = ll->head; lle != NULL; lle = lle->next) {
        v = *((int *)lle->val);
        printf("%010d\n", v);
    }
    llist_destroy(ll);

    xfree(vals);
    return 0;
}
