#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ck_rwlock.h>

#include "hash.h"
#include "ganglia.h"

datum_t *
datum_new ( void *data, size_t size )
{
   datum_t *datum;

   datum = malloc( sizeof(datum_t));
   if ( datum == NULL )
      return NULL;

   datum->data = malloc( size );
   if ( datum->data == NULL )
      {
         free(datum);
         return NULL;
      }

   datum->size = size;
   memcpy(datum->data, data, datum->size);
   return datum;
}

static datum_t *
datum_dup (datum_t *src)
{
   datum_t *dest;

   dest = malloc(sizeof(datum_t));
   if( dest == NULL )
      return NULL;

   dest->data = malloc(src->size);
   if ( dest->data == NULL )
      {
         free(dest);
         return NULL;
      } 

   dest->size = src->size;
   memcpy(dest->data, src->data, src->size);
   return dest;
}

void
datum_free (datum_t *datum)
{
   free(datum->data);
   free(datum);
}

hash_t *
hash_create (size_t size)
{
   hash_t *hash;

   debug_msg("hash_create size = %zd", size);

   hash = (hash_t *) malloc ( sizeof(hash_t) );
   if( hash == NULL )
      {
         debug_msg("hash malloc error in hash_create()");
         return NULL;
      } 

   /*
    * Rounds up size to nearest power of two. This allows us to avoid division
    * from modulus math when calculating buckets.
    */
   size--;
   size |= size >> 1;
   size |= size >> 2;
   size |= size >> 4;
   size |= size >> 8;
   size |= size >> 16;
   size++;
   hash->size = size;

   debug_msg("hash->size is %zd", hash->size);

   hash->node = calloc(hash->size, sizeof (*hash->node));
   if (hash->node == NULL)
      {
         debug_msg("hash->node malloc error. freeing hash.");
         free(hash);
         return NULL;
      }

   hash->lock = calloc(hash->size, sizeof (*hash->lock));
   if (hash->lock == NULL) 
      {
         debug_msg("hash->lock malloc error. freeing hash.");
         free(hash);
         return NULL;
      }
   /*
    * ck_rwlock_init just sets things to zero and does a memory fence. calloc 
    * already set zero, so add mfence only.
    */
   ck_pr_fence_memory();

   return hash;
}

void
hash_destroy (hash_t * hash)
{
   size_t i;
   node_t *bucket, *next;
   datum_t *val;

   for( i=0; i< hash->size; i++)
     {
        for(bucket = &hash->node[i]; bucket!= NULL; bucket = next)
           {
              next = bucket->next;
              if (bucket->key)
                {
                  val  = hash_delete( bucket->key, hash);
                  datum_free(val);
                }
           }
     }
        
   free( hash->node );
   free( hash );
}

int
hash_get_flags (hash_t *hash)
{
   return hash->flags;
}

void
hash_set_flags (hash_t *hash, int flags)
{
   hash->flags = flags;
}

static uint64_t
hash_key (const void *key, size_t len, uint64_t seed)
{
        const uint64_t m = 0xc6a4a7935bd1e995LLU;
        const int r = 47;
        uint64_t h = seed ^ (len * m);
        const uint64_t *data = (const uint64_t *)key;
        const uint64_t *end = data + (len/8);
        const unsigned char *data2;

        while(data != end) {
                uint64_t k = *data++;

                k *= m;
                k ^= k >> r;
                k *= m;

                h ^= k;
                h *= m;
        }

        data2 = (const unsigned char *)data;
        switch(len & 7) {
                case 7: h ^= ((uint64_t)(data2[6])) << 48;
                case 6: h ^= ((uint64_t)(data2[5])) << 40;
                case 5: h ^= ((uint64_t)(data2[4])) << 32;
                case 4: h ^= ((uint64_t)(data2[3])) << 24;
                case 3: h ^= ((uint64_t)(data2[2])) << 16;
                case 2: h ^= ((uint64_t)(data2[1])) << 8;
                case 1: h ^= ((uint64_t)(data2[0]));
                        h *= m;
        }

        h ^= h >> r;
        h *= m;
        h ^= h >> r;

        return (h);
}

inline size_t
hashval ( datum_t *key, hash_t *hash )
{
 
   return hash_key(key->data, key->size, 0) & (hash->size - 1);
}

int
hash_keycmp(hash_t *hash, datum_t *key1, datum_t *key2)
{
   return key1->size == key2->size && !strncmp(key1->data, key2->data, key1->size);
}

datum_t *
hash_insert (datum_t *key, datum_t *val, hash_t *hash)
{
  size_t i;
  node_t *bucket;

  i = hashval(key, hash);

  ck_rwlock_write_lock(&hash->lock[i]);

  bucket = &hash->node[i];
  if (bucket->key == NULL)
     {
        /* This bucket hasn't been used yet */
        bucket->key  = datum_dup(key);
        if ( bucket->key == NULL )
           {
              free(bucket);
              bucket = NULL;
              ck_rwlock_write_unlock(&hash->lock[i]);
              return NULL;
           } 
        bucket->val = datum_dup(val);
        if ( bucket->val == NULL )
           {
              free(bucket);
              bucket = NULL;
              ck_rwlock_write_unlock(&hash->lock[i]);
              return NULL;
           }
        ck_rwlock_write_unlock(&hash->lock[i]);
        return bucket->val;
     }

  /* This node in the hash is already in use.  
     Collision or new data for existing key. */

  for (; bucket != NULL; bucket = bucket->next)
      {
         if(bucket->key && hash_keycmp(hash, bucket->key, key))
            {
               /* New data for an existing key */

               /* Make sure we have enough space */
               if ( bucket->val->size < val->size )
                  {
                     /* Make sure we have enough room */
                     if(! (bucket->val->data = realloc(bucket->val->data, val->size)) )
                        {
                           ck_rwlock_write_unlock(&hash->lock[i]);
                           return NULL;
                        }
                     bucket->val->size = val->size;
                  }

               memset( bucket->val->data, 0, val->size );
               memcpy( bucket->val->data, val->data, val->size );
               ck_rwlock_write_unlock(&hash->lock[i]);
               return bucket->val;
            }
      }
                    
  /* It's a Hash collision... link it in the collided bucket */
  bucket = calloc(1, sizeof(*bucket));
  if (bucket == NULL)
     {
        ck_rwlock_write_unlock(&hash->lock[i]);
        return NULL;
     }
  bucket->key = datum_dup (key);
  if ( bucket->key == NULL )
     {
        free(bucket);
        ck_rwlock_write_unlock(&hash->lock[i]);
        return NULL;
     }
  bucket->val = datum_dup (val);
  if ( bucket->val == NULL )
     {
        datum_free(bucket->key);
        free(bucket);
        ck_rwlock_write_unlock(&hash->lock[i]);
        return NULL;
     }  

  
  bucket->next = hash->node[i].next;
  hash->node[i].next = bucket;
  ck_rwlock_write_unlock(&hash->lock[i]);
  return bucket->val;
}

datum_t *
hash_lookup (datum_t *key, hash_t * hash)
{
  size_t i;
  datum_t *val;
  node_t *bucket;

  i = hashval(key, hash);

  ck_rwlock_read_lock(&hash->lock[i]);

  bucket = &hash->node[i];

  if ( bucket == NULL )
     {
        ck_rwlock_read_unlock(&hash->lock[i]);
        return NULL;
     }

  for (; bucket != NULL; bucket = bucket->next)
    {
      if (bucket->key && hash_keycmp(hash, key, bucket->key))
         {
            val =  datum_dup( bucket->val );
            ck_rwlock_read_unlock(&hash->lock[i]);
            return val;
         }
    }

  ck_rwlock_read_unlock(&hash->lock[i]);
  return NULL;
}

datum_t *
hash_delete (datum_t *key, hash_t * hash)
{
  size_t i;
  node_t *bucket, *last = NULL;

  i = hashval(key,hash);

  ck_rwlock_write_lock(&hash->lock[i]);

  bucket = &hash->node[i];
  if (bucket->key == NULL )
     {
        ck_rwlock_write_unlock(&hash->lock[i]);
        return NULL;
     }

  for (; bucket != NULL; last = bucket, bucket = bucket->next)
    {
      node_t tmp;
      if (bucket == &hash->node[i]) 
        {
          tmp.key = bucket->key;
          tmp.val = bucket->val;

          if (bucket->next)
            {
              bucket->key = bucket->next->key;
              bucket->val = bucket->next->val;
              bucket->next = bucket->next->next;
            }
          else
            {
              memset(bucket, 0, sizeof(*bucket));
            }

          datum_free(tmp.key);
          ck_rwlock_write_unlock(&hash->lock[i]);
        }
      else
        {
          last->next = bucket->next;
          datum_free(bucket->key);
          tmp.val = bucket->val;
          free(bucket);
          ck_rwlock_write_unlock(&hash->lock[i]);
        }

      return tmp.val;
    }

  ck_rwlock_write_unlock(&hash->lock[i]);
  return NULL;
}

/* Walk the hash table from hash index "from" until the end, or
 * until stopped by walk function. Similar to hash_foreach, used by cleanup.
 * Use of hint makes O(n^2) cleanup into O(n).
 */
int
hash_walkfrom (hash_t * hash, size_t from,
   int (*func)(datum_t *, datum_t *, void *), void *arg)
{
  int stop=0;
  size_t i;
  node_t *bucket;

  for (i = from; i < hash->size && !stop; i++)
    {
       ck_rwlock_read_lock(&hash->lock[i]);
       for (bucket = &hash->node[i]; bucket != NULL && bucket->key != NULL; bucket = bucket->next)
         {
           if (bucket->key == NULL) continue;
           stop = func(bucket->key, bucket->val, arg);
           if (stop) break;
         }
       ck_rwlock_read_unlock(&hash->lock[i]);
    }
  return stop;
}

int
hash_foreach (hash_t * hash, int (*func)(datum_t *, datum_t *, void *), void *arg)
{
  int stop=0;
  size_t i;
  node_t *bucket;

  for (i = 0; i < hash->size && !stop; i++)
    {
       ck_rwlock_read_lock(&hash->lock[i]);
       for (bucket = &hash->node[i]; bucket != NULL && bucket->key != NULL; bucket = bucket->next)
         {
           if (bucket->key == NULL) continue;
           stop = func(bucket->key, bucket->val, arg);
           if (stop) break;
         }
       ck_rwlock_read_unlock(&hash->lock[i]);
    }
  return stop;
}
