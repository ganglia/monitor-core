/* $Id$ */
#include "interface.h"
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
pthread_mutex_t  mcast_socket_mutex      = PTHREAD_MUTEX_INITIALIZER;

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

/* In cleanup.c */
extern void *cleanup_thread(void *arg);

/* In debug_msg.c */
extern int debug_level;

struct gengetopt_args_info args_info;

extern gmond_config_t gmond_config;

uint32_t start_time;

void 
send_all_metric_data( void )
{
   uint32_t i;
   struct timeval now;
   int waiting_period = 600;  /* secs .. 10 mins */

   gettimeofday(&now, NULL);

   /* Did this gmond just restart in the last "waiting_period" secs? */
   if( (now.tv_sec - start_time) < waiting_period )
      {
         for (i=1; i< num_key_metrics; i++)
            {
               pthread_mutex_lock(&(metric[i].mutex));

               /* Will the next multicast be forced beyond the waiting period? */
               if( metric[i].mcast_threshold > (now.tv_sec + waiting_period) )
                  {

                     /* Make sure it multicasts in the 2 mins following the 
                        waiting period */
                     metric[i].mcast_threshold = now.tv_sec + waiting_period +
                       1+ (int)(120.0*rand()/(RAND_MAX+1.0)); 
                  }

               pthread_mutex_unlock(&(metric[i].mutex));
            }
      }
   else /* Gmond has been running at least "waiting_period" secs. */
      {
         for (i=1; i< num_key_metrics; i++)
            {
               pthread_mutex_lock(&(metric[i].mutex));
               metric[i].mcast_threshold = 0;
               pthread_mutex_unlock(&(metric[i].mutex));
            }
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

g_val_t
location_func(void)
{
   g_val_t val;

   strncpy(val.str, gmond_config.location, MAX_G_STRING_SIZE);
   debug_msg("my location is %s", val.str);
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
   struct intf_entry *entry;
   struct in_addr mcast_if_addr;

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

   /* Initialize mutexes */
   for(i = 0; i < num_key_metrics; i++)
      pthread_mutex_init(&(metric[i].mutex), NULL);

   if(! gmond_config.mcast_if_given )
      {
         entry = get_first_multicast_interface();
         if(!entry) {
            err_msg("Warning: Could not find a multicast-enabled interface, using anything we can find.\n");
            entry = get_first_interface();
            if (!entry)
               err_quit("We don't have any interfaces besides loopback, exiting.\n");
         }
         debug_msg("Using interface %s", entry->intf_name);
      }
   else
      {
         entry = get_interface ( gmond_config.mcast_if );
         if(!entry)
            err_quit("%s is not a valid multicast-enabled interface", gmond_config.mcast_if);
         debug_msg("Using multicast-enabled interface %s", gmond_config.mcast_if);
      }

   /* fd for incoming multicast messages */
   if(! gmond_config.deaf )
      {
         mcast_join_socket = g_mcast_in ( gmond_config.mcast_channel, gmond_config.mcast_port,
                                          (struct in_addr *)&(entry->intf_addr.addr_ip));
         if (! mcast_join_socket )
            {
               perror("g_mcast_in() failed");
               return -1;
            }

         debug_msg("mcast listening on %s %hu", gmond_config.mcast_channel, gmond_config.mcast_port);

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
         
         /* A thread to cleanup old metrics and hosts */
         pthread_create(&tid, &attr, cleanup_thread, (void *) NULL);
         debug_msg("cleanup thread has been started");
      }

   /* fd for outgoing multicast messages */
   if(! gmond_config.mute )
      {
         mcast_socket = g_mcast_out ( gmond_config.mcast_channel, gmond_config.mcast_port,  
                                      (struct in_addr *)&(entry->intf_addr.addr_ip), gmond_config.mcast_ttl);
         if ( !mcast_socket )
            {
               perror("gmond could not connect to multicast channel");
               return -1;
            }
         debug_msg("multicasting on channel %s %d", gmond_config.mcast_channel, gmond_config.mcast_port);

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
