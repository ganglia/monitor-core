/* $Id$ */
#include <ganglia.h>
#include <ganglia/hash.h>
#include <ganglia/barrier.h>
#include <ganglia/gmond_config.h>
#include "cmdline.h"
#include "key_metrics.h"
#include "metric_typedefs.h"
#include "node_data_t.h"
#include <string.h>

#ifdef AIX
extern void *h_errno_which(void);
#define h_errno   (*(int *)h_errno_which())
#else
extern int h_errno;
#endif

extern metric_t metric[];

extern g_mcast_socket * mcast_join_socket;
extern pthread_mutex_t mcast_join_socket_mutex;

/* The root hash table pointer. */
extern hash_t *cluster;


struct cleanup_arg {
   struct timeval *tv;
   datum_t *key;
   datum_t *val;
};


static int
cleanup_metric ( datum_t *key, datum_t *val, void *arg )
{
   struct cleanup_arg *cleanup = (struct cleanup_arg *) arg;
   struct timeval *tv = cleanup->tv;
   metric_data_t * metric = (metric_data_t *) val->data;
   unsigned int born;
   int rc;

   born = metric->timestamp.tv_sec;

   /* Never delete a metric if its DMAX=0 */
   if (metric->dmax && ((tv->tv_sec - born) > metric->dmax) ) {
      /* Need to delete this metric, stop hash_foreach() and put key in arg. */
      cleanup->key = key;
      return 1;
   }
   return 0;
}


static int
cleanup_node ( datum_t *key, datum_t *val, void *arg )
{
   struct cleanup_arg *cleanup = (struct cleanup_arg *) arg;
   struct timeval *tv = (struct timeval *) cleanup->tv;
   node_data_t * node = (node_data_t *) val->data;
   datum_t *rv;
   unsigned int born;
   int rc;

   born = node->timestamp.tv_sec;

   /* Never delete a node if its DMAX=0 */
   if (node->dmax && ((tv->tv_sec - born) > node->dmax) ) {
      /* Host is older than dmax. Forget about him. */
      cleanup->key = key;
      cleanup->val = val;
      return 1;
   }

   /* Re-use the cleanup_arg struct for checking our metrics */
   cleanup->key = 0;
   while ((rc=hash_foreach(node->hashp, cleanup_metric, (void*) cleanup))) {
      if (cleanup->key) {
         /* cleanup_metric() just told us to delete this metric. Hash_foreach() has
            not completed. */
         debug_msg("Cleanup deleting metric \"%d\"", (int) cleanup->key->data);
         rv=hash_delete(cleanup->key, node->hashp);
         if (rv) datum_free(rv);
         cleanup->key=0;
      }
      else break;
   }

   cleanup->key = 0;
   while ((rc=hash_foreach(node->user_hashp, cleanup_metric, (void*) cleanup))) {
      if (cleanup->key) {
         debug_msg("Cleanup deleting user metric \"%s\"", (char*) cleanup->key->data);
         rv=hash_delete(cleanup->key, node->user_hashp);
         if (rv) datum_free(rv);
         cleanup->key=0;
      }
      else  {
	  	 debug_msg("Cleanup: exiting hash_foreach with an error %d", rc);
         break;
	  }
   }

   return 0;
}


void *
cleanup_thread(void *arg)
{
   struct timeval tv;
   struct cleanup_arg cleanup;
   node_data_t * node;
   datum_t *rv;
   int rc;

   for (;;) {
      /* Cleanup every 5 minutes */
      sleep(300);

      gettimeofday(&tv, NULL);

      cleanup.tv = &tv;
      cleanup.key = 0;
      cleanup.val = 0;

      debug_msg("Cleanup thread running...");
      while ((rc=hash_foreach(cluster, cleanup_node, (void *) &cleanup))) {
         if (cleanup.key) {
            debug_msg("Cleanup deleting host \"%s\"", (char*) cleanup.key->data);
            node = (node_data_t *) cleanup.val->data;
            hash_destroy(node->hashp);
            hash_destroy(node->user_hashp);
            /* No need to WRITE_UNLOCK since there is only one cleanup thread? */
            rv=hash_delete(cleanup.key, cluster);
            /* Don't use 'node' pointer after this call. */
            if (rv) datum_free(rv);
            /* hash_foreach() has been stopped by cleanup_node() and needs to continue. */
            cleanup.key = 0;
         }
         else break;
      }
   } /* for (;;) */

}
