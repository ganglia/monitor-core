/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <cmdline.h>
#include <dotconf.h>
#include <ganglia/error.h>
#include <ganglia/debug_msg.h>
#include <ganglia/hash.h>
#include <ganglia/llist.h>
#include <ganglia/net.h>
#include "gmetad_typedefs.h"

extern void *data_thread ( void *arg );

llist_entry *trusted_hosts = NULL;
hash_t *sources;
extern int debug_level;
int xml_port = 8651;
g_tcp_socket *   server_socket;
pthread_mutex_t  server_socket_mutex     = PTHREAD_MUTEX_INITIALIZER;
int server_threads = 2;
char *rrd_rootdir = "/var/lib/ganglia/rrds";

extern void* server_thread(void *);

hash_t *xml;

static DOTCONF_CB(cb_trusted_hosts)
{
   int i;
   llist_entry *le;

   for (i = 0; i < cmd->arg_count; i++)
      {
         le = (llist_entry *)malloc(sizeof(llist_entry));
         le->val = strdup ( cmd->data.list[i] );
         llist_add(&(trusted_hosts), le);
      }
   return NULL;
}

static DOTCONF_CB(cb_data_source)
{
   unsigned int i, len;
   data_source_list_t *dslist;
   g_inet_addr *ia;
   datum_t key;
   datum_t *find;
   datum_t val;
   int port;
 
   debug_msg("[%s] [%s] [%s]", cmd->data.list[0], cmd->data.list[1], cmd->data.list[2]);
 
   key.data = cmd->data.list[0];
   key.size = strlen ( key.data ) + 1;
 
   port = atoi( cmd->data.list[2] );
 
   find = hash_lookup( &key, sources ); 
   if (!find )
      {
         debug_msg("[%s] is a NEW data source tag", cmd->data.list[0]);

         dslist = (data_source_list_t *) malloc ( sizeof(data_source_list_t) );
         if(!dslist)
            {
               err_quit("Unable to malloc data source list");
            }

         dslist->num_sources = 0;
         dslist->sources = NULL;
         dslist->name = strdup( cmd->data.list[0] );
      }
   else
      {
         debug_msg("[%s] is an OLD data source tag", cmd->data.list[0]);
         dslist = *(data_source_list_t**)find->data;

         datum_free(find);
      }

   debug_msg("***| dslist number is %d addr %p", dslist->num_sources, dslist);

   /* Make space for the new entry */

   len = (dslist->num_sources+1) * sizeof( g_inet_addr * );

   debug_msg("Reallocating %d bytes of memory at %p", len, dslist->sources);

   dslist->sources = realloc( (void *)(dslist->sources), len );

   debug_msg("Realloc returned %p", dslist->sources);

   if(!dslist->sources)
      {
         err_quit("Unable to realloc source list");
      }

   dslist->sources[dslist->num_sources] = (g_inet_addr *) g_inetaddr_new ( cmd->data.list[1], port );
   if(! dslist->sources[dslist->num_sources])
      {
         err_quit("Unable to create inetaddr and save it");
      } 
   else
      {
         dslist->num_sources++;
      }

   val.data = &dslist;
   val.size = sizeof(dslist);

   debug_msg("Inserting data into the sources hash");
   find  = hash_insert( &key, &val, sources );
   if(!find)
      {
         err_quit("Unable to insert list pointer into source hash\n");
      }
   debug_msg("Data inserted for [%s] into sources hash", key.data);
   return NULL;
}

static DOTCONF_CB(cb_debug_level)
{
   debug_level = cmd->data.value;
   debug_msg("Setting debug level to %d", cmd->data.value);
   return NULL;
}

static DOTCONF_CB(cb_xml_port)
{
   debug_msg("Setting xml port to %d", cmd->data.value);
   xml_port = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_server_threads)
{
   debug_msg("Setting number of xml server threads to %d", cmd->data.value);
   server_threads = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_rrd_rootdir)
{
   debug_msg("Setting the RRD Rootdir to %s", cmd->data.str);
   free( rrd_rootdir );
   rrd_rootdir = strdup ( cmd->data.str );
}

struct gengetopt_args_info args_info;
static configoption_t gmetad_options[] =
   {
      {"data_source", ARG_LIST, cb_data_source, NULL, 0},
      {"trusted_hosts", ARG_LIST, cb_trusted_hosts, NULL, 0},
      {"debug_level",  ARG_INT,  cb_debug_level, NULL, 0},
      {"xml_port",  ARG_INT, cb_xml_port, NULL, 0},
      {"server_threads", ARG_INT, cb_server_threads, NULL, 0},
      {"rrd_rootdir", ARG_STR, cb_rrd_rootdir, NULL, 0},
      LAST_OPTION
   };

static FUNC_ERRORHANDLER(errorhandler)
{
   err_quit("gmetad config file error: %s\n", msg);
}

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
   configfile_t *configfile;
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

   configfile = dotconf_create( args_info.conf_arg, gmetad_options, 0, CASE_INSENSITIVE );    
   if (!configfile)
      {
         err_quit("Unable to open config file: %s\n", args_info.conf_arg);
      }

   configfile->errorhandler = (dotconf_errorhandler_t) errorhandler;

   if (dotconf_command_loop(configfile) == 0)
      {
         dotconf_cleanup(configfile);
         err_quit("dotconf_command_loop error");
      } 

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
