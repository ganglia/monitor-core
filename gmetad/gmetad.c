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
#include "gmetad_typedefs.h"

hash_t *xml;
hash_t *sources;

extern void *data_thread ( void *arg );
extern void* server_thread(void *);
extern int parse_config_file ( char *config_file );

llist_entry *trusted_hosts = NULL;
extern int debug_level;
int xml_port = 8651;
g_tcp_socket *   server_socket;
pthread_mutex_t  server_socket_mutex     = PTHREAD_MUTEX_INITIALIZER;
int server_threads = 2;
char *rrd_rootdir = "/var/lib/ganglia/rrds";

struct gengetopt_args_info args_info;

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
main ( int argc, char *argv[] )
{
   pthread_t pid;
   pthread_attr_t attr;
   int i;

   srand(1234567);

   if (cmdline_parser (argc, argv, &args_info) != 0)
      err_quit("command-line parser error");

   /* Get the real number of data sources later */
   sources = hash_create( 64 );
   if (! sources )
      {
         err_quit("Unable to create sources hash\n");
      }

   xml = hash_create( 64 );
   if (! xml)
      { 
         err_quit("Unable to create XML hash\n");
      }

   parse_config_file ( args_info.conf_arg );

   if(debug_level)
      {
         printf("Sources are ...\n");
         hash_foreach( sources, print_sources, NULL);
      }

   hash_foreach( sources, spin_off_the_data_threads, NULL );

   server_socket = g_tcp_socket_server_new( xml_port );
   if (! server_socket )
      {
         perror("tcp_listen() on xml_port failed");
         exit(1);
      }
   debug_msg("listening on port %d", xml_port);  

   pthread_attr_init( &attr );
   pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

   for(i=0; i< server_threads; i++)
      pthread_create(&pid, &attr, server_thread, NULL);   

   /* Put in RRD processing here */
   for(;;)
      pause();

   return 0;
}
