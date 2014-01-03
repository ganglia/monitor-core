#ifndef HASH__H
#define HASH__H 1

#include <stddef.h>				  /* For size_t     */
#include <ck_rwlock.h>

#define HASH_FLAG_IGNORE_CASE 1

typedef struct 
{
   void        *data;
   unsigned int size;
} 
datum_t;

typedef struct node
{
   datum_t *key;
   datum_t *val;
   struct node *next;
   char __pad[8];
}
node_t;

typedef struct
{
   ck_rwlock_t *lock;
   size_t size;
   node_t *node;
   int flags;
}
hash_t;

hash_t  *hash_create (size_t size);
void     hash_destroy(hash_t *hash);

int      hash_get_flags(hash_t *hash);
void     hash_set_flags(hash_t *hash, int flags);

datum_t *hash_insert (datum_t *key, datum_t *val, hash_t *hash);
datum_t *hash_delete (datum_t *key, hash_t *hash);

datum_t *hash_lookup (datum_t *key, hash_t *hash);
int hash_foreach (hash_t *hash, int (*func)(datum_t *key, datum_t *val, void *), void *arg);
int hash_walkfrom (hash_t *hash, size_t from, int (*func)(datum_t *key, datum_t *val, void *), void *arg);

datum_t *datum_new  ( void *data, size_t size );
void     datum_free ( datum_t *datum );

size_t hashval ( datum_t *key, hash_t *hash );

#endif /* HASH__H */
