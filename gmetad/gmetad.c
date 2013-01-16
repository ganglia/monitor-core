#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <time.h>
#include <gmetad.h>
#include <cmdline.h>

#include <apr_time.h>

#include "daemon_init.h"
#include "update_pidfile.h"

#include "rrd_helpers.h"

#define METADATA_SLEEP_RANDOMIZE 5.0
#define METADATA_MINIMUM_SLEEP 1

/* Holds our data sources. */
hash_t *sources;

/* The root of our local grid. Replaces the old "xml" hash table. */
Source_t root;

g_tcp_socket *server_socket;
g_tcp_socket *interactive_socket;

pthread_mutex_t  server_socket_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t  server_interactive_mutex = PTHREAD_MUTEX_INITIALIZER;

extern void *data_thread ( void *arg );
extern void* server_thread(void *);
extern int parse_config_file ( char *config_file );
extern int number_of_datasources ( char *config_file );
extern struct type_tag* in_type_list (char *, unsigned int);

struct gengetopt_args_info args_info;

extern gmetad_config_t gmetad_config;
static int debug_level;

/* In cleanup.c */
extern void *cleanup_thread(void *arg);

static int
print_sources ( datum_t *key, datum_t *val, void *arg )
{
   int i;
   data_source_list_t *d = *((data_source_list_t **)(val->data));
   g_inet_addr *addr;

   fprintf(stderr,"Source: [%s, step %d] has %d sources\n",
      (char*) key->data, d->step, d->num_sources);
   for(i = 0; i < d->num_sources; i++)
      {
         addr = d->sources[i];
         fprintf(stderr, "\t%s\n", addr->name);
      }

   return 0;
}

static int
spin_off_the_data_threads( datum_t *key, datum_t *val, void *arg )
{
   data_source_list_t *d = *((data_source_list_t **)(val->data));
   pthread_t pid;
   pthread_attr_t attr;

   pthread_attr_init( &attr );
   pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

   pthread_create(&pid, &attr, data_thread, (void *)d);
   return 0;
}


/* The string fields in Metric_t are actually offsets into the value buffer field.
 * This function returns a regular char* pointer. Tricky, but efficient.
 */
char *
getfield(char* buf, short int index)
{
   if (index<0) return "unspecified";

   return (char*) buf+index;
}

/* A bit slower than doing things by hand, but much safer. Guards
 * against memory overflows.
 */
int
addstring(char *strings, int *edge, const char *s)
{
   int e = *edge;
   int end = e + strlen(s) + 1;

   /* I wish C had real exceptions. */
   if (e > GMETAD_FRAMESIZE || end > GMETAD_FRAMESIZE)
   {
      err_msg("Field is too big!!");
      return -1;
   }

   strcpy(strings + e, s);
   *edge = end;

   return e;
}


/* Zeroes out every metric value in a summary hash table. */
int
zero_out_summary(datum_t *key, datum_t *val, void *arg)
{
   Metric_t *metric;

   /* Note that we get the actual value bytes here, not a copy. */
   metric = (Metric_t*) val->data;
   memset(&metric->val, 0, sizeof(metric->val));
   metric->num = 0;

   return 0;
}


/* Sums the metric summaries from all data sources. */
static int
sum_metrics(datum_t *key, datum_t *val, void *arg)
{
   datum_t *hash_datum, *rdatum;
   Metric_t *rootmetric, *metric;
   char *type;
   struct type_tag *tt;
   int do_sum = 1;

   metric = (Metric_t *) val->data;
   type = getfield(metric->strings, metric->type);

   hash_datum = hash_lookup(key, root.metric_summary);
   if (!hash_datum)
      {
         hash_datum = datum_new((char*) metric, val->size);
         do_sum = 0;
      }
   rootmetric = (Metric_t*) hash_datum->data;

   if (do_sum)
      {
         tt = in_type_list(type, strlen(type));
         if (!tt) {
            datum_free(hash_datum);
            return 0;
	 }

         /* We sum everything in double to properly combine integer sources
            (3.0) with float sources (3.1).  This also avoids wraparound
            errors: for example memory KB exceeding 4TB. */
         switch (tt->type)
            {
               case INT:
               case UINT:
               case FLOAT:
                  rootmetric->val.d += metric->val.d;
                  break;
               default:
                  break;
            }
         rootmetric->num += metric->num;
      }

   rdatum = hash_insert(key, hash_datum, root.metric_summary);

   datum_free(hash_datum);

   if (!rdatum)
      return 1;
   else
      return 0;
}


/* Sums the metric summaries from all data sources. */
static int
do_root_summary( datum_t *key, datum_t *val, void *arg )
{
   Source_t *source = (Source_t*) val->data;
   int rc;
   llist_entry *le;

   /* We skip dead sources. */
   if (source->ds->dead)
      return 0;

   /* We skip metrics not to be summarized. */
   if (llist_search(&(gmetad_config.unsummarized_metrics), (void *)key->data, llist_strncmp, &le) == 0)
      return 0;

   /* Need to be sure the source has a complete sum for its metrics. */
   pthread_mutex_lock(source->sum_finished);

   /* We know that all these metrics are numeric. */
   rc = hash_foreach(source->metric_summary, sum_metrics, arg);

   /* Update the top level root source */
   root.hosts_up += source->hosts_up;
   root.hosts_down += source->hosts_down;

   /* summary completed for source */
   pthread_mutex_unlock(source->sum_finished);

   return rc;
}


static int
write_root_summary(datum_t *key, datum_t *val, void *arg)
{
   char *name, *type;
   char sum[256];
   char num[256];
   Metric_t *metric;
   int rc;
   struct type_tag *tt;
   llist_entry *le;

   name = (char*) key->data;
   metric = (Metric_t*) val->data;
   type = getfield(metric->strings, metric->type);

   /* Summarize all numeric metrics */
   tt = in_type_list(type, strlen(type));
   /* Don't write a summary for an unknown or STRING type */
   if (!tt || (tt->type == STRING))
       return 0;

   /* Don't write a summary for metrics not to be summarized */
   if (llist_search(&(gmetad_config.unsummarized_metrics), (void *)key->data, llist_strncmp, &le) == 0)
       return 0;

   /* We log all our sums in double which does not suffer from
      wraparound errors: for example memory KB exceeding 4TB. -twitham */
   sprintf(sum, "%.5f", metric->val.d);

   sprintf(num, "%u", metric->num);

   /* err_msg("Writing Overall Summary for metric %s (%s)", name, sum); */

   /* Save the data to a rrd file unless write_rrds == off */
	 if (gmetad_config.write_rrds == 0)
	     return 0;

   debug_msg("Writing Root Summary data for metric %s", name);

   rc = write_data_to_rrd( NULL, NULL, name, sum, num, 15, 0, metric->slope);
   if (rc)
      {
         err_msg("Unable to write meta data for metric %s to RRD", name);
      }
   return 0;
}

#define HOSTNAMESZ 64

int
main ( int argc, char *argv[] )
{
   struct stat struct_stat;
   pthread_t pid;
   pthread_attr_t attr;
   int i, num_sources;
   uid_t gmetad_uid;
   mode_t rrd_umask;
   char * gmetad_username;
   struct passwd *pw;
   char hostname[HOSTNAMESZ];
   gmetad_config_t *c = &gmetad_config;
   apr_interval_time_t sleep_time;
   apr_time_t last_metadata;
   double random_sleep_factor;
   unsigned int rand_seed;

   /* Ignore SIGPIPE */
   signal( SIGPIPE, SIG_IGN );

   if (cmdline_parser(argc, argv, &args_info) != 0)
      err_quit("command-line parser error");

   num_sources = number_of_datasources( args_info.conf_arg );
   if(!num_sources)
      {
         err_quit("%s doesn't have any data sources specified", args_info.conf_arg);
      }

   memset(&root, 0, sizeof(root));
   root.id = ROOT_NODE;

   /* Get the real number of data sources later */
   sources = hash_create( num_sources + 10 );
   if (! sources )
      {
         err_quit("Unable to create sources hash\n");
      }

   root.authority = hash_create( num_sources + 10 );
   if (!root.authority)
      {
         err_quit("Unable to create root authority (our grids and clusters) hash\n");
      }

   root.metric_summary = hash_create (DEFAULT_METRICSIZE);
   if (!root.metric_summary)
      {
         err_quit("Unable to create root summary hash");
      }

   parse_config_file ( args_info.conf_arg );
    /* If given, use command line directives over config file ones. */
   if (args_info.debug_given)
      {
         c->debug_level = args_info.debug_arg;
      }
   debug_level = c->debug_level;
   set_debug_msg_level(debug_level);

   /* Setup our default authority pointer if the conf file hasnt yet.
    * Done in the style of hash node strings. */
   if (!root.stringslen)
      {
         gethostname(hostname, HOSTNAMESZ);
         root.authority_ptr = 0;
         sprintf(root.strings, "http://%s/ganglia/", hostname);
         root.stringslen += strlen(root.strings) + 1;
      }

   rand_seed = apr_time_now() * (int)pthread_self();
   for(i = 0; i < root.stringslen; rand_seed = rand_seed * root.strings[i++]);

   /* Debug level 1 is error output only, and no daemonizing. */
   if (!debug_level)
      {
         rrd_umask = c->umask;
         daemon_init (argv[0], 0, rrd_umask);
      }

   if (args_info.pid_file_given)
     {
       update_pidfile (args_info.pid_file_arg);
     }

   /* The rrd_rootdir must be writable by the gmetad process */
   if( c->should_setuid )
      {
         if(! (pw = getpwnam(c->setuid_username)))
            {
               err_sys("Getpwnam error");
            }
         gmetad_uid = pw->pw_uid;
         gmetad_username = c->setuid_username;
      }
   else
      {
         gmetad_uid = getuid();
         if(! (pw = getpwuid(gmetad_uid)))
            {
               err_sys("Getpwnam error");
            } 
         gmetad_username = strdup(pw->pw_name);
      }

   debug_msg("Going to run as user %s", gmetad_username);
   if( c->should_setuid )
      {
         become_a_nobody(c->setuid_username);
      }

   if( stat( c->rrd_rootdir, &struct_stat ) )
      {
          err_sys("Please make sure that %s exists", c->rrd_rootdir);
      }
   if ( struct_stat.st_uid != gmetad_uid )
      {
          err_quit("Please make sure that %s is owned by %s", c->rrd_rootdir, gmetad_username);
      }
   if (! (struct_stat.st_mode & S_IWUSR) )
      {
          err_quit("Please make sure %s has WRITE permission for %s", gmetad_username, c->rrd_rootdir);
      }

   if(debug_level)
      {
         fprintf(stderr,"Sources are ...\n");
         hash_foreach( sources, print_sources, NULL);
      }

   server_socket = g_tcp_socket_server_new( c->xml_port );
   if (server_socket == NULL)
      {
         err_quit("tcp_listen() on xml_port failed");
      }
   debug_msg("xml listening on port %d", c->xml_port);
   
   interactive_socket = g_tcp_socket_server_new( c->interactive_port );
   if (interactive_socket == NULL)
      {
         err_quit("tcp_listen() on interactive_port failed");
      }
   debug_msg("interactive xml listening on port %d", c->interactive_port);

   /* initialize summary mutex */
   root.sum_finished = (pthread_mutex_t *) 
                          malloc(sizeof(pthread_mutex_t));
   pthread_mutex_init(root.sum_finished, NULL);

   pthread_attr_init( &attr );
   pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

   /* Spin off the non-interactive server threads. (Half as many as interactive). */
   for (i=0; i < c->server_threads/2; i++)
      pthread_create(&pid, &attr, server_thread, (void*) 0);

   /* Spin off the interactive server threads. */
   for (i=0; i < c->server_threads; i++)
      pthread_create(&pid, &attr, server_thread, (void*) 1);

   hash_foreach( sources, spin_off_the_data_threads, NULL );

   /* A thread to cleanup old metrics and hosts */
   pthread_create(&pid, &attr, cleanup_thread, (void *) NULL);
   debug_msg("cleanup thread has been started");

    /* Meta data */
   last_metadata = 0;
   for(;;)
      {
         /* Do at a random interval, between 
                 (shortest_step/2) +/- METADATA_SLEEP_RANDOMIZE percent */
         random_sleep_factor = (1 + (METADATA_SLEEP_RANDOMIZE / 50.0) * ((rand_r(&rand_seed) - RAND_MAX/2)/(float)RAND_MAX));
         sleep_time = random_sleep_factor * apr_time_from_sec(c->shortest_step) / 2;
         /* Make sure the sleep time is at least 1 second */
         if(apr_time_sec(apr_time_now() + sleep_time) < (METADATA_MINIMUM_SLEEP + apr_time_sec(apr_time_now())))
            sleep_time += apr_time_from_sec(METADATA_MINIMUM_SLEEP);
         apr_sleep(sleep_time);

         /* Need to be sure root is locked while doing summary */
         pthread_mutex_lock(root.sum_finished);

         /* Flush the old values */
         hash_foreach(root.metric_summary, zero_out_summary, NULL);
         root.hosts_up = 0;
         root.hosts_down = 0;

         /* Sum the new values */
         hash_foreach(root.authority, do_root_summary, NULL );

         /* summary completed */
         pthread_mutex_unlock(root.sum_finished);

         /* Save them to RRD */
         hash_foreach(root.metric_summary, write_root_summary, NULL);

         /* Remember our last run */
         last_metadata = apr_time_now();
      }

   return 0;
}
