/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <cmdline.h>
#include <ganglia/error.h>
#include <ganglia/debug_msg.h>
#include <ganglia/hash.h>
#include <ganglia/llist.h>
#include <ganglia/net.h>
#include <ganglia/barrier.h>
#include "gmetad.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

hash_t *xml;

hash_t *sources;

extern void *data_thread ( void *arg );
extern void* server_thread(void *);
extern int parse_config_file ( char *config_file );
extern int number_of_datasources ( char *config_file );

extern struct ganglia_metric metrics[];

llist_entry *trusted_hosts = NULL;
extern int debug_level;
int xml_port = 8651;
g_tcp_socket *   server_socket;
pthread_mutex_t  server_socket_mutex     = PTHREAD_MUTEX_INITIALIZER;
int server_threads = 2;
char *rrd_rootdir =  "/var/lib/ganglia/rrds";
char *setuid_username = "nobody";
int should_setuid = 1;
unsigned int source_index = 0;

struct gengetopt_args_info args_info;

long double  sum_of_sums[MAX_METRIC_HASH_VALUE]; 
unsigned int sum_of_nums[MAX_METRIC_HASH_VALUE];

int
print_sources ( datum_t *key, datum_t *val, void *arg )
{
   int i;
   data_source_list_t *d = *((data_source_list_t **)(val->data));
   g_inet_addr *addr;

   fprintf(stderr,"Source: [%s] has %d sources\n", key->data, d->num_sources);
   for(i = 0; i < d->num_sources; i++)
      {
         addr = d->sources[i];
         fprintf(stderr, "\t%s\n", addr->name);
      }
  
   return 0;
}

int
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

int
save_sums_n_nums( datum_t *key, datum_t *val, void *arg )
{
   data_source_list_t *ds =  *(data_source_list_t **)val->data;
   struct ganglia_metric *gm;
   register int i;
   int len;

   /* Ignore dead data sources */
   if( ds->dead )
      {
         debug_msg("[%s] is a dead source", ds->name);
         return 0;
      }
 
   for ( i = 0; i < MAX_METRIC_HASH_VALUE; i++ )
      {
         sum_of_sums[i] += ds->sum[i];
         sum_of_nums[i] += ds->num[i];
      }

   return 0;
}

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
   char sum[512];
   char num[512];  

   srand(52336789);

   if (cmdline_parser (argc, argv, &args_info) != 0)
      err_quit("command-line parser error");

   num_sources = number_of_datasources( args_info.conf_arg );
   if(!num_sources)
      {
         err_quit("%s doesn't have any data sources specified", args_info.conf_arg);
      }

   /* Get the real number of data sources later */
   sources = hash_create( num_sources + 10 );
   if (! sources )
      {
         err_quit("Unable to create sources hash\n");
      }

   xml = hash_create( num_sources + 10 );
   if (! xml)
      {
         err_quit("Unable to create XML hash\n");
      }

   parse_config_file ( args_info.conf_arg );

   /* The rrd_rootdir must be writable by the gmetad process */
   if( should_setuid )
      {
         if(! (pw = getpwnam(setuid_username)))
            {
               err_sys("Getpwnam error");
            }
         gmetad_uid = pw->pw_uid;
         gmetad_username = setuid_username;
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
   if( should_setuid )
      {
         become_a_nobody(setuid_username);
      }

   if( stat( rrd_rootdir, &struct_stat ) )
      {
          err_sys("Please make sure that %s exists", rrd_rootdir);
      }
          
   if ( struct_stat.st_uid != gmetad_uid )
      {
          err_quit("Please make sure that %s is owned by %s", rrd_rootdir, gmetad_username);
      }
        
   if (! (struct_stat.st_mode & S_IWUSR) )
      {
          err_quit("Please make sure %s has WRITE permission for %s", gmetad_username, rrd_rootdir);
      }

   if(debug_level)
      {
         printf("Sources are ...\n");
         hash_foreach( sources, print_sources, NULL);
      }
   else
      {
         daemon_init ( argv[0], 0);
      }

   server_socket = g_tcp_socket_server_new( xml_port );
   if (server_socket == NULL)
      {
         perror("tcp_listen() on xml_port failed");
         exit(1);
      }
   debug_msg("listening on port %d", xml_port);  

   pthread_attr_init( &attr );
   pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

   for(i=0; i< server_threads; i++)
      pthread_create(&pid, &attr, server_thread, NULL);   

   hash_foreach( sources, spin_off_the_data_threads, NULL );

   /* Meta data */
   for(;;)
      {
         sleep_time=10+(int) (20.0*rand()/(RAND_MAX+10.0));
         sleep(sleep_time);

         /* Flush the old values */
         memset( &sum_of_sums, 0, MAX_METRIC_HASH_VALUE * sizeof( long double ));
         memset( &sum_of_nums, 0, MAX_METRIC_HASH_VALUE * sizeof( unsigned int));

         /* Sum the new values */
         hash_foreach ( sources, save_sums_n_nums, NULL );

         /* Save them to RRD */
         for ( i = 0; i < MAX_METRIC_HASH_VALUE; i++ )
            {
               if( strlen(metrics[i].name) && sum_of_nums[i] )
                  {
                     sprintf(num, "%d",  sum_of_nums[i]);
                     sprintf(sum, "%Lf", sum_of_sums[i]);

                     /* Save the data to a round robin database */
                     if( write_data_to_rrd( NULL, NULL, (char *)metrics[i].name, sum, num, "15") )
                        {
                           err_msg("Unable to write meta data to RRDbs");
                        }                     
                  }
            }
      }
   return 0;
}
