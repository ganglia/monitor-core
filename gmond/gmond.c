/* $Id$ */
#include "gangliaconf.h"
#include "dotconf.h"
#include <ganglia/gmond_config.h>
#include <ganglia/hash.h>
#include <ganglia/barrier.h>
#include <ganglia/become_a_nobody.h>
#include <ganglia/net.h>
#include "metric.h"
#include <pwd.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include "cmdline.h"

/* The entire cluster this gmond knows about */
hash_t *cluster;

g_mcast_socket * mcast_socket;
pthread_mutex_t  mcast_socket_mutex  = PTHREAD_MUTEX_INITIALIZER;

g_mcast_socket * mcast_join_socket;
pthread_mutex_t  mcast_join_socket_mutex = PTHREAD_MUTEX_INITIALIZER;

g_tcp_socket *   server_socket;
pthread_mutex_t  server_socket_mutex     = PTHREAD_MUTEX_INITIALIZER;

/* In dmonitor.c */
extern void *monitor_thread(void *arg);
extern int mcast_value ( uint32_t key );

/* In dlisten.c */
extern void *mcast_listen_thread(void *arg);  

/* In dserver.c */
extern void *server_thread(void *arg);

/* In debug_msg.c */
extern int debug_level;

struct gengetopt_args_info args_info;

extern gmond_config_t gmond_config;

uint32_t start_time;

void 
send_all_metric_data( void )
{
   uint32_t i;
 
   for (i=1; i< num_key_metrics; i++)
      {
         metric[i].mcast_threshold = 0;
      }
}              

/* Added temporarily to make gexec work until I build the service model */
g_val_t
gexec_func ( void )
{
   g_val_t val;

   if( gmond_config.no_gexec || ( SUPPORT_GEXEC == 0 ) )
      snprintf(val.str, MAX_G_STRING_SIZE, "%s", "OFF");
   else
      snprintf(val.str, MAX_G_STRING_SIZE, "%s", "ON");

   return val;
}

g_val_t
heartbeat_func( void )
{
   g_val_t val;

   val.uint32 = start_time;
   debug_msg("my start_time is %d\n", val.uint32);
   return val;
}

int 
main ( int argc, char *argv[] )
{
   int rval, i=0;
   g_val_t initval;
   pthread_t tid;
   pthread_attr_t attr;
   barrier *mcast_listen_barrier, *server_barrier;
   struct timeval tv;
   llist_entry *interfaces = NULL;
   llist_entry *last_interface = NULL;

   gettimeofday(&tv, NULL);
   start_time = (uint32_t) tv.tv_sec;

   if (cmdline_parser (argc, argv, &args_info) != 0)
      exit(1) ;

   rval = get_gmond_config(args_info.conf_arg);
   if ( rval == 0 )
      {
         debug_msg("no config file found.. going with defaults");
      } 
   else if( rval == 1)
      {
         debug_msg("config file %s processed with no errors", args_info.conf_arg);
      }
   else
      {
         err_quit("failed to process %s. Exiting.", args_info.conf_arg);
      }

   if(!gmond_config.no_setuid)
      become_a_nobody(gmond_config.setuid);

   debug_level = gmond_config.debug_level;
   if (! debug_level )
      {
         daemon_init ( argv[0], 0);
      }

   debug_msg("pthread_attr_init");
   pthread_attr_init( &attr );
   pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

   debug_msg("creating cluster hash for %d nodes", gmond_config.num_nodes);
   cluster = hash_create(gmond_config.num_nodes);
   debug_msg("gmond initialized cluster hash");

   srand(1);

   interfaces = g_inetaddr_list_interfaces();
   /* Go to the end */
   for(;interfaces != NULL; interfaces = interfaces->next)
      { 
         last_interface = interfaces; 
      }
   /* Move backwards */
   debug_msg("INTERFACES Discovered");
   for(i=0, interfaces = last_interface; interfaces != NULL; interfaces = interfaces->prev, i++)
      {
         debug_msg("\t%d %s", i, inet_ntoa( G_SOCKADDR_IN(((g_inet_addr *)(interfaces->val))->sa).sin_addr ));
      }

   /* fd for incoming multicast messages */
   if(! gmond_config.deaf )
      {
         g_inet_addr * addr;

         addr = (g_inet_addr *) g_inetaddr_new( gmond_config.mcast_channel, gmond_config.mcast_port );
         mcast_join_socket = g_mcast_socket_new( addr );
         if (! mcast_join_socket )
            {
               g_inetaddr_delete(addr);
               perror("gmond could not join the multicast channel");
               return -1;
            }
         debug_msg("mcast listening on %s %hu", gmond_config.mcast_channel, gmond_config.mcast_port); 

         /* Make sure we have loopback on */
         if ( g_mcast_socket_set_loopback( mcast_join_socket, 1) != 0 )
            {
               g_inetaddr_delete(addr);
               perror("gmond could not set loopback");
               return -1;
            }

         if ( g_mcast_socket_join_group( mcast_join_socket, addr ) != 0 )
            {
               g_inetaddr_delete(addr);
               perror("gmond could not join group");
               return -1;
            }

         server_socket = g_tcp_socket_server_new( gmond_config.xml_port );
         if (! server_socket )
            {
               perror("tcp_listen() on xml_port failed");
               return -1;
            }      
         debug_msg("XML listening on port %d", gmond_config.xml_port);

         /* thread(s) to listen to the multicast traffic */
         barrier_init(&mcast_listen_barrier, gmond_config.mcast_threads);
         for ( i = 0 ; i < gmond_config.mcast_threads; i++ )
            {
               pthread_create(&tid, &attr, mcast_listen_thread, (void *)mcast_listen_barrier);
            }
         debug_msg("listening thread(s) have been started");

         /* threads to answer requests for XML */
         barrier_init(&server_barrier, (gmond_config.xml_threads));
         for ( i=0 ; i < gmond_config.xml_threads; i++ )
            {
               pthread_create(&tid, &attr, server_thread, (void *)server_barrier);
            }
         debug_msg("listening thread(s) have been started");
      }

   /* fd for outgoing multicast messages */
   if(! gmond_config.mute )
      {
         g_inet_addr *addr;
 
         addr = g_inetaddr_new ( gmond_config.mcast_channel, gmond_config.mcast_port );
         mcast_socket = g_mcast_socket_new( addr );
         g_inetaddr_delete( addr );
         if ( !mcast_socket )
            {
               perror("gmond could not connect to multicast channel");
               return -1;
            }
         debug_msg("multicasting on channel %s %d", gmond_config.mcast_channel, gmond_config.mcast_port);

         if ( g_mcast_socket_set_ttl(mcast_socket, gmond_config.mcast_ttl ) < 0)
            {
               perror("gmond could not set the ttl");
               return -1;
            } 

         rval = g_mcast_socket_connect ( mcast_socket );
         if ( rval <0)
            {
               perror("mcast_connect() connect() error");
               return -1;
            }

         /* in machine.c */
         initval = metric_init();
         if ( initval.int32 <0)
            {
               err_quit("monitor_init() returned an error");
            }

         pthread_create(&tid, &attr, monitor_thread, NULL);
         debug_msg("created monitor thread");
      }

   
   for(;;)
      {
         pause();
      }
   return 0;
}
