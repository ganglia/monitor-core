/*
 * cleanup.c - Enforces metric/host delete time. Helps keep
 *    memory usage trim and fit by deleting expired metrics from hash.
 *
 * ChangeLog:
 *
 * 25sept2002 - Federico Sacerdoti <fds@sdsc.edu>
 *    Original version.
 *
 * 16feb2003 - Federico Sacerdoti <fds@sdsc.edu>
 *    Made more efficient: O(n) from O(n^2) in
 *    worst case. Fixed bug in hash_foreach() that caused
 *    immortal metrics.
 * 
 * Sept2004 - Federico D. Sacerdoti <fds@sdsc.edu>
 *    Adapted for use in Gmetad.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include <apr_time.h>

#include "ganglia.h"
#include "gmetad.h"

#include "conf.h"
#include "cmdline.h"

/* Interval (seconds) between cleanup runs */
#define CLEANUP_INTERVAL 180

#ifdef AIX
extern void *h_errno_which(void);
#define h_errno   (*(int *)h_errno_which())
#else
extern int h_errno;
#endif


/* The root source. */
extern Source_t root;


struct cleanup_arg {
   struct timeval *tv;
   datum_t *key;
   datum_t *val;
   size_t hashval;
};


static int
cleanup_metric ( datum_t *key, datum_t *val, void *arg )
{
   struct cleanup_arg *cleanup = (struct cleanup_arg *) arg;
   struct timeval *tv = cleanup->tv;
   Metric_t * metric = (Metric_t *) val->data;
   unsigned int born;

   born = metric->t0.tv_sec;

   /* Never delete a metric if its DMAX=0 */
   if (metric->dmax && ((tv->tv_sec - born) > metric->dmax) ) 
      {
         cleanup->key = key;
         return 1;
      }
   return 0;
}


static int
cleanup_node ( datum_t *key, datum_t *val, void *arg )
{
   struct cleanup_arg *nodecleanup = (struct cleanup_arg *) arg;
   struct cleanup_arg cleanup;
   struct timeval *tv = (struct timeval *) nodecleanup->tv;
   Host_t * node = (Host_t *) val->data;
   datum_t *rv;
   unsigned int born;

   born = node->t0.tv_sec;

   /* Never delete a node if its DMAX=0 */
   if (node->dmax && ((tv->tv_sec - born) > node->dmax) ) {
      /* Host is older than dmax. Delete. */
      nodecleanup->key = key;
      nodecleanup->val = val;
      return 1;
   }

   cleanup.tv = tv;

   cleanup.key = 0;
   cleanup.hashval = 0;
   while (hash_walkfrom(node->metrics, cleanup.hashval, 
                           cleanup_metric, (void*)&cleanup)) {

      if (cleanup.key) {
         cleanup.hashval = hashval(cleanup.key, node->metrics);
         rv=hash_delete(cleanup.key, node->metrics);
         if (rv) datum_free(rv);
         cleanup.key=0;
      }
      else break;
   }

   /* We have not deleted this node */
   return 0;
}


static int
cleanup_source ( datum_t *key, datum_t *val, void *arg )
{
   struct cleanup_arg *nodecleanup = (struct cleanup_arg *) arg;
   struct cleanup_arg cleanup;
   struct timeval *tv = (struct timeval *) nodecleanup->tv;
   Source_t * source = (Source_t *) val->data;
   datum_t *rv;
   Host_t *node;
#if 0
   unsigned int born;
#endif
   hash_t *metric_summary;

   /* Currently we never delete a source. */
   
   /* Clean up metric summaries */
   cleanup.tv = tv;
   cleanup.key = 0;
   cleanup.hashval = 0;
   metric_summary = source->metric_summary; /* Just look at the current one */
   while (hash_walkfrom(metric_summary, cleanup.hashval, 
                           cleanup_metric, (void*) &cleanup)) {

      if (cleanup.key) {
         cleanup.hashval = hashval(cleanup.key, metric_summary);
         rv=hash_delete(cleanup.key, metric_summary);
         if (rv) datum_free(rv);
         cleanup.key=0;
      }
      else break;
   }
    
   /* Our "authority" is a list of children hash tables, NULL for 
    * a grid. */
   if (!source->authority)
       return 0;

   /* Clean up hosts */
   cleanup.tv = tv;
   cleanup.key = 0;
   cleanup.hashval = 0;
   while (hash_walkfrom(source->authority, cleanup.hashval, 
                           cleanup_node, (void*) &cleanup)) {

      if (cleanup.key) {
         debug_msg("Cleanup deleting host \"%s\"", 
                         (char*) cleanup.key->data);
         
         node = (Host_t *) cleanup.val->data;
         hash_destroy(node->metrics);
         
         cleanup.hashval = hashval(cleanup.key, source->authority);
         rv=hash_delete(cleanup.key, source->authority);
         /* Don't use 'node' pointer after this call. */
         if (rv) datum_free(rv);
         cleanup.key = 0;
      }
      else break;
   }

   /* We have not deleted this source */
   return 0;
}


void *
cleanup_thread(void *arg)
{
   struct timeval tv;
   struct cleanup_arg cleanup;
   datum_t *rv;
   Source_t *source;

   for (;;) {
      /* Cleanup every 3 minutes. */
      apr_sleep(apr_time_from_sec(CLEANUP_INTERVAL));

      debug_msg("Cleanup thread running...");

      gettimeofday(&tv, NULL);

      cleanup.tv = &tv;
      cleanup.key = 0;
      cleanup.val = 0;
      cleanup.hashval = 0;
      while (hash_walkfrom(root.authority, cleanup.hashval, 
                              cleanup_source, (void*)&cleanup)) {

         if (cleanup.key) {
            debug_msg("Cleanup deleting source \"%s\"", 
                            (char*) cleanup.key->data);
            source = (Source_t *) cleanup.val->data;
            hash_destroy(source->metric_summary);
            if (source->authority)
                hash_destroy(source->authority);

            cleanup.hashval = hashval(cleanup.key, root.authority);
            rv=hash_delete(cleanup.key, root.authority);
            /* Don't use 'node' pointer after this call. */
            if (rv) datum_free(rv);
            cleanup.key = 0;
         }
         else break;
      }

   } /* for (;;) */

}

