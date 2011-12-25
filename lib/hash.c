#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

static size_t
hash_prime (size_t size)
{
   size_t i, num_primes;
   /* http://www.mathematical.com/primes1to100k.html */
   int primes[]={
      2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97,
      101,103,107,109,113,127,131,137,139,149,151,157,163,167,173,179,181,191,193,197,199,
      211,223,227,229,233,239,241,251,257,263,269,271,277,281,283,293,
      307,311,313,317,331,337,347,349,353,359,367,373,379,383,389,397,
      401,409,419,421,431,433,439,443,449,457,461,463,467,479,487,491,499,
      503,509,521,523,541,547,557,563,569,571,577,587,593,599,
      601,607,613,617,619,631,641,643,647,653,659,661,673,677,683,691,
      701,709,719,727,733,739,743,751,757,761,769,773,787,797,
      809,811,821,823,827,829,839,853,857,859,863,877,881,883,887,
      907,911,919,929,937,941,947,953,967,971,977,983,991,997,
      1009,1013,1019,1021,1031,1033,1039,1049,1051,1061,1063,1069,1087,1091,1093,1097,
      1103,1109,1117,1123,1129,1151,1153,1163,1171,1181,1187,1193,
      1201,1213,1217,1223,1229,1231,1237,1249,1259,1277,1279,1283,1289,1291,1297,
      1301,1303,1307,1319,1321,1327,1361,1367,1373,1381,1399,
      1409,1423,1427,1429,1433,1439,1447,1451,1453,1459,1471,1481,1483,1487,1489,1493,1499,
      1511,1523,1531,1543,1549,1553,1559,1567,1571,1579,1583,1597,
      1601,1607,1609,1613,1619,1621,1627,1637,1657,1663,1667,1669,1693,1697,1699,
      1709,1721,1723,1733,1741,1747,1753,1759,1777,1783,1787,1789,
      1801,1811,1823,1831,1847,1861,1867,1871,1873,1877,1879,1889,
      1901,1907,1193,1931,1933,1949,1951,1973,1979,1987,1993,1997,1999,
      2003,2011,2017,2027,2029,2039,2053,2063,2069,2081,2083,2087,2089,2099,
      2111,2113,2129,2131,2137,2141,2143,2153,2161,2179,
      2203,2207,2213,2221,2237,2239,2243,2251,2267,2269,2273,2281,2287,2293,2297,
      2309,2311,2333,2339,2341,2347,2351,2357,2371,2377,2381,2383,2389,2393,2399
      };

   num_primes = sizeof(primes)/sizeof(int);

   for(i=0; i < num_primes ; i++)
      {
         if( primes[i] > size )
            return primes[i];
      }

   return primes[num_primes-1];
}

hash_t *
hash_create (size_t size)
{
   size_t i;
   hash_t *hash;

   debug_msg("hash_create size = %d", size);

   hash = (hash_t *) malloc ( sizeof(hash_t) );
   if( hash == NULL )
      {
         debug_msg("hash malloc error in hash_create()");
         return NULL;
      } 

   hash->size = hash_prime(size);

   debug_msg("hash->size is %d", hash->size);

   hash->node = (node_t * *) malloc (sizeof (node_t *) * hash->size);
   if (hash->node == NULL)
      {
         debug_msg("hash->node malloc error. freeing hash.");
         free(hash);
         return NULL;
      }

   for (i = 0; i < hash->size; i++)
      {
         hash->node[i] = malloc( sizeof(node_t) );
         if ( hash->node[i] == NULL )
            break;
         /* Initialize */
         hash->node[i]->bucket = NULL;
         pthread_rdwr_init_np( &(hash->node[i]->rwlock) );
      }

   /* Was there an error initializing the hash nodes? */
   if ( i != hash->size )
      {
         debug_msg("hash->node[i] malloc error");
         /* Rewind */
         for (hash->size = i; hash->size >= 0; hash->size--)
            {
               free(hash->node[hash->size]);
            }
         free(hash);
         return NULL; 
      }
   return hash;
}

void
hash_destroy (hash_t * hash)
{
   size_t i;
   bucket_t *bucket, *next;
   datum_t *val;

   for( i=0; i< hash->size; i++)
     {
        for(bucket = hash->node[i]->bucket; bucket!= NULL; bucket = next)
           {
              next = bucket->next;
              val  = hash_delete( bucket->key, hash);
              datum_free(val);
           }
        free(hash->node[i]);
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

size_t
hashval ( datum_t *key, hash_t *hash )
{
   int i;
   int hash_val;
 
   /* We should handle these errors better later */
   if ( hash == NULL || key == NULL || key->data == NULL || key->size <= 0 )
      return 0;

   if(hash->flags & HASH_FLAG_IGNORE_CASE)
   {
      hash_val = tolower(((unsigned char *)key->data)[0]);
      for ( i = 0; i < key->size ; i++ )
         hash_val = ( hash_val * 32 + tolower(((unsigned char *)key->data)[i])) % hash->size; 
   }
   else
   {
      hash_val = ((unsigned char *)key->data)[0];
      for ( i = 0; i < key->size ; i++ )
         hash_val = ( hash_val * 32 + ((unsigned char *)key->data)[i]) % hash->size;
   }

   return hash_val;
}

int
hash_keycmp(hash_t *hash, datum_t *key1, datum_t *key2)
{
   if(hash->flags & HASH_FLAG_IGNORE_CASE)
   {
      return strncasecmp(key1->data, key2->data, key1->size);
   }
   else
   {
      return strncmp(key1->data, key2->data, key1->size);
   }
}

datum_t *
hash_insert (datum_t *key, datum_t *val, hash_t *hash)
{
  size_t i;
  bucket_t *bucket;

  i = hashval(key, hash);

  WRITE_LOCK(hash, i);

  bucket = hash->node[i]->bucket;

  if ( bucket == NULL )
     {
        /* This bucket hasn't been used yet */

        bucket = (bucket_t *)malloc( sizeof(bucket_t) );
        if ( bucket == NULL )
           {
              WRITE_UNLOCK(hash, i);
              return NULL;
           }
 
        bucket->next = NULL;
        bucket->key  = datum_dup(key);
        if ( bucket->key == NULL )
           {
              free(bucket);
              bucket = NULL;
              WRITE_UNLOCK(hash, i);
              return NULL;
           } 
        bucket->val = datum_dup(val);
        if ( bucket->val == NULL )
           {
              free(bucket);
              bucket = NULL;
              WRITE_UNLOCK(hash, i);
              return NULL;
           }
        hash->node[i]->bucket = bucket;
        WRITE_UNLOCK(hash, i);
        return bucket->val;
     }

  /* This node in the hash is already in use.  
     Collision or new data for existing key. */

   for (bucket = hash->node[i]->bucket; bucket != NULL; bucket = bucket->next)
      {
         if( bucket->key->size != key->size )
            continue;

         if(! hash_keycmp(hash, bucket->key, key) )
            {
               /* New data for an existing key */

               /* Make sure we have enough space */
               if ( bucket->val->size != val->size )
                  {
                     /* Make sure we have enough room */
                     if(! (bucket->val->data = realloc(bucket->val->data, val->size)) )
                        {
                           WRITE_UNLOCK(hash, i);
                           return NULL;
                        }
                     bucket->val->size = val->size;
                  }

               memcpy( bucket->val->data, val->data, val->size );
               WRITE_UNLOCK(hash, i);
               return bucket->val;
            }
      }
                    
  /* It's a Hash collision... link it in the collided bucket */
  bucket = (bucket_t *) malloc (sizeof (bucket_t));
  if (bucket == NULL)
     {
        WRITE_UNLOCK(hash, i);
        return NULL;
     }
  bucket->key = datum_dup (key);
  if ( bucket->key == NULL )
     {
        free(bucket);
        WRITE_UNLOCK(hash, i);
        return NULL;
     }
  bucket->val = datum_dup (val);
  if ( bucket->val == NULL )
     {
        datum_free(bucket->key);
        free(bucket);
        WRITE_UNLOCK(hash, i);
        return NULL;
     }  

  bucket->next = hash->node[i]->bucket;
  hash->node[i]->bucket = bucket;
  WRITE_UNLOCK(hash, i);
  return bucket->val;
}

datum_t *
hash_lookup (datum_t *key, hash_t * hash)
{
  size_t i;
  datum_t *val;
  bucket_t *bucket;

  i = hashval(key, hash);

  READ_LOCK(hash, i);

  bucket = hash->node[i]->bucket;

  if ( bucket == NULL )
     {
        READ_UNLOCK(hash, i);
        return NULL;
     }

  for (; bucket != NULL; bucket = bucket->next)
    {
      if ( key->size != bucket->key->size )
         continue;
 
      if (! hash_keycmp( hash, key, bucket->key))
         {
            val =  datum_dup( bucket->val );
            READ_UNLOCK(hash, i);
            return val;
         }
    }

  READ_UNLOCK(hash, i);
  return NULL;
}

datum_t *
hash_delete (datum_t *key, hash_t * hash)
{
  size_t i;
  datum_t *val;
  bucket_t *bucket, *last = NULL;

  i = hashval(key,hash);

  WRITE_LOCK(hash,i);

  if ( hash->node[i]->bucket == NULL )
     {
        WRITE_UNLOCK(hash,i);
        return NULL;
     }

  for (last = NULL,  bucket = hash->node[i]->bucket;
       bucket != NULL; last = bucket, bucket = bucket->next)
    {
      if (bucket->key->size == key->size 
          && !hash_keycmp(hash, key, bucket->key))
        {
          if (last != NULL)
            {
              val = bucket->val;
              last->next = bucket->next;
              datum_free(bucket->key);
              free (bucket);
              WRITE_UNLOCK(hash,i);
              return val;
            }

          else
            {
              val = bucket->val;
              hash->node[i]->bucket = bucket->next;
              datum_free (bucket->key);
              free (bucket);
              WRITE_UNLOCK(hash,i);
              return val;
            }
        }
    }

  WRITE_UNLOCK(hash,i);
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
  bucket_t *bucket;

  for (i = from; i < hash->size && !stop; i++)
    {
       READ_LOCK(hash, i);
       for (bucket = hash->node[i]->bucket; bucket != NULL; bucket = bucket->next)
         {
           stop = func(bucket->key, bucket->val, arg);
           if (stop) break;
         }
       READ_UNLOCK(hash, i);
    }
   return stop;
}

int
hash_foreach (hash_t * hash, int (*func)(datum_t *, datum_t *, void *), void *arg)
{
  int stop=0;
  size_t i;
  bucket_t *bucket;

  for (i = 0; i < hash->size && !stop; i++)
    {
       READ_LOCK(hash, i);
       for (bucket = hash->node[i]->bucket; bucket != NULL; bucket = bucket->next)
         {
           stop = func(bucket->key, bucket->val, arg);
           if (stop) break;
         }
       READ_UNLOCK(hash, i);
    }
   return stop;
}
