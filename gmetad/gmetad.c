/* $Id$ */
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
#include <ganglia/llist.h>

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
extern int write_data_to_rrd( const char *source, const char *host, const char *metric,
   const char *sum, const char *num, unsigned int step, unsigned int time_polled);

struct gengetopt_args_info args_info;

extern gmetad_config_t gmetad_config;
extern int debug_level;


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
         if (!tt) return 0;

         switch (tt->type)
            {
               case INT:
                  rootmetric->val.int32 += metric->val.int32;
                  break;
               case UINT:
                  rootmetric->val.uint32 += metric->val.uint32;
                  break;
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

   /* We skip dead sources. */
   if (source->ds->dead)
      return 0;

   /* Need to be sure the source has a complete sum for its metrics. */
   pthread_mutex_lock(source->sum_finished);

   /*err_msg("Doing root summary for source %s", source->ds->name);*/

   /* We know that all these metrics are numeric. */
   rc = hash_foreach(source->metric_summary, sum_metrics, arg);

   /* Update the top level root source */
   root.hosts_up += source->hosts_up;
   root.hosts_down += source->hosts_down;

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

   name = (char*) key->data;
   metric = (Metric_t*) val->data;
   type = getfield(metric->strings, metric->type);

   /* Summarize all numeric metrics */
   tt = in_type_list(type, strlen(type));
   if (!tt) return 0;

   switch (tt->type)
      {
         case INT:
            sprintf(sum, "%d", metric->val.int32);
            break;
         case UINT:
            sprintf(sum, "%u", metric->val.uint32);
            break;
         case FLOAT:
            sprintf(sum, "%.5f", metric->val.d);
            break;
         default:
            break;
      }

   sprintf(num, "%u", metric->num);

   /*err_msg("Writing Overall Summary for metric %s (%s)", name, sum);*/

   /* Save the data to a round robin database */
   rc = write_data_to_rrd( NULL, NULL, name, sum, num, 15, 0);
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
   int i, num_sources, sleep_time;
   uid_t gmetad_uid;
   char * gmetad_username;
   struct passwd *pw;
   char hostname[HOSTNAMESZ];
   gmetad_config_t *c = &gmetad_config;

   srand(52336789);

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

   /* Setup our default authority pointer if the conf file hasnt yet.
    * Done in the style of hash node strings. */
   if (!root.stringslen)
      {
         gethostname(hostname, HOSTNAMESZ);
         root.authority_ptr = 0;
         sprintf(root.strings, "http://%s/ganglia/", hostname);
         root.stringslen += strlen(root.strings) + 1;
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
         printf("Sources are ...\n");
         hash_foreach( sources, print_sources, NULL);
      }

   /* Debug level 1 is error output only, and no daemonizing. */
   if (!debug_level)
      {
         daemon_init (argv[0], 0);
      }

   server_socket = g_tcp_socket_server_new( c->xml_port );
   if (server_socket == NULL)
      {
         perror("tcp_listen() on xml_port failed");
         exit(1);
      }
   debug_msg("xml listening on port %d", c->xml_port);
   
   interactive_socket = g_tcp_socket_server_new( c->interactive_port );
   if (interactive_socket == NULL)
      {
         perror("tcp_listen() on interactive_port failed");
         exit(1);
      }
   debug_msg("interactive xml listening on port %d", c->interactive_port);

   pthread_attr_init( &attr );
   pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

   /* Spin off the non-interactive server threads. (Half as many as interactive). */
   for (i=0; i < c->server_threads/2; i++)
      pthread_create(&pid, &attr, server_thread, (void*) 0);

   /* Spin off the interactive server threads. */
   for (i=0; i < c->server_threads; i++)
      pthread_create(&pid, &attr, server_thread, (void*) 1);

   hash_foreach( sources, spin_off_the_data_threads, NULL );

    /* Meta data */
   for(;;)
      {
         /* Do at a random interval between 10 and 30 sec. */
         sleep_time = 10 + ((30-10)*1.0) * rand()/(RAND_MAX + 1.0);
         sleep(sleep_time);

         /* Flush the old values */
         hash_foreach(root.metric_summary, zero_out_summary, NULL);
         root.hosts_up = 0;
         root.hosts_down = 0;

         /* Sum the new values */
         hash_foreach(root.authority, do_root_summary, NULL );

         /* Save them to RRD */
         hash_foreach(root.metric_summary, write_root_summary, NULL);
      }

   return 0;
}

