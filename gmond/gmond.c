/* $Id$ */
#include <pwd.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <signal.h>

#include "lib/ganglia_private.h"
#include "lib/dotconf.h"
#include "lib/hash.h"
#include "lib/barrier.h"
#include "lib/become_a_nobody.h"
#include "libunp/unp.h"
#include "lib/debug_msg.h"
#include "lib/tpool.h"

#include "conf.h"
#include "cmdline.h"
#include "metric.h"

/* The entire cluster this gmond knows about */
hash_t *cluster;

g3_thread_pool collect_send_pool = NULL;
int send_sockets[MAX_NUM_CHANNELS];

g3_thread_pool receive_pool = NULL;
int receive_sockets[MAX_NUM_CHANNELS];

int server_socket;
pthread_mutex_t  server_socket_mutex     = PTHREAD_MUTEX_INITIALIZER;

int compressed_socket;
pthread_mutex_t  compressed_socket_mutex = PTHREAD_MUTEX_INITIALIZER;

/* In monitor.c */
extern void monitor_thread(void *arg);
extern int send_value ( uint32_t key );

/* In listen.c */
extern void msg_listen_thread(void *arg);  

/* In server.c */
extern void *server_thread(void *arg);

/* In cleanup.c */
extern void *cleanup_thread(void *arg);

/* In debug_msg.c */
extern int debug_level;

struct gengetopt_args_info args_info;

extern gmond_config_t gmond_config;

uint32_t start_time;

/* This only works on IPv4 for right now */
static int
is_multicast (const char *ip)
{
  struct in_addr haddr;
  unsigned int addr;

  if(!inet_aton(ip, &haddr))
    return -1;  /* not a valid address */

  addr = htonl( haddr.s_addr );

  if ((addr & 0xF0000000) == 0xE0000000)
    return 1;

  return 0;
}

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
               /* Will the next multicast be forced beyond the waiting period? */
               if( metric[i].msg_threshold > (now.tv_sec + waiting_period) )
                  {

                     /* Make sure it multicasts in the 2 mins following the 
                        waiting period */
                     metric[i].msg_threshold = now.tv_sec + waiting_period +
                       1+ (int)(120.0*rand()/(RAND_MAX+1.0)); 
                  }
            }
      }
   else /* Gmond has been running at least "waiting_period" secs. */
      {
         for (i=1; i< num_key_metrics; i++)
            {
               metric[i].msg_threshold = 0;
            }
      }
}

/* Added temporarily to make gexec work until I build the service model */
g_val_t
gexec_func ( void )
{
   g_val_t val;

#if 0
   if( gmond_config.no_gexec || ( SUPPORT_GEXEC == 0 ) )
      snprintf(val.str, MAX_G_STRING_SIZE, "%s", "OFF");
   else
      snprintf(val.str, MAX_G_STRING_SIZE, "%s", "ON");
#endif

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
   struct timeval tv;
   int no_compression = 0;
   struct sockaddr *sa;
   socklen_t salen;
   const int on = 1;

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

   /* If given, use command line directives over config file ones. */
   if (args_info.debug_given) {
      gmond_config.debug_level = args_info.debug_arg;
   }
   if (args_info.location_given) {
      debug_msg("Cmd line: Setting location to %s\n", args_info.location_arg);
      free(gmond_config.location);
      gmond_config.location = strdup(args_info.location_arg);
   }

   /* in machine.c */
   initval = metric_init();
   if ( initval.int32 <0)
      {
         err_quit("metric_init() returned an error");
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

   /* Ignore any SIGPIPE signals */
   signal( SIGPIPE, SIG_IGN );    

   if(! gmond_config.deaf )
      {
        receive_pool = g3_thread_pool_create( gmond_config.num_receive_channels, 128, 1 );

        for(i=0; i< gmond_config.num_receive_channels; i++)
          {
            if(gmond_config.receive_channels[i] && is_multicast(gmond_config.receive_channels[i]))
              {
                /* Multicast channels.. we need to join a Multicast group */
                receive_sockets[i] = Udp_client( gmond_config.receive_channels[i], 
                                                 gmond_config.receive_ports[i],
                                                 (void **)&sa, &salen);
                Setsockopt( receive_sockets[i], SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
                Bind( receive_sockets[i], sa, salen );
                Mcast_join( receive_sockets[i], sa, salen, 
                       gmond_config.mcast_if_given ? gmond_config.mcast_if : NULL, 0);
              }
            else
              {
                /* Non-multicast channels */
                receive_sockets[i] = Udp_server( gmond_config.receive_channels[i], 
                                             gmond_config.receive_ports[i],
                                             &salen);
                Setsockopt( receive_sockets[i], SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
              }

             g3_run( receive_pool, msg_listen_thread, (void *)&receive_sockets[i]); 
           }
        
         debug_msg("Starting socket to listen on XML port %s", gmond_config.xml_port);
         server_socket = Tcp_listen(NULL, gmond_config.xml_port, &salen );

         debug_msg("Starting socket to listen on compressed XML port %s", gmond_config.compressed_xml_port);
         compressed_socket = Tcp_listen(NULL, gmond_config.compressed_xml_port, &salen);

         /* Spin off the threads for raw XML */
         for ( i=0 ; i < gmond_config.xml_threads; i++ )
            {
               pthread_create(&tid, &attr, server_thread, (void *)&no_compression);
            }
         debug_msg("listening (uncompressed xml) thread(s) have been started");

         /* Spin off the threads for compressed XML */
         for ( i=0 ; i < gmond_config.compressed_xml_threads; i++ )
            {
               pthread_create(&tid, &attr, server_thread, 
                             (void *)&(gmond_config.xml_compression_level));
            }
         debug_msg("listening (compressed xml) thread(s) have been started");
         
         /* A thread to cleanup old metrics and hosts */
         pthread_create(&tid, &attr, cleanup_thread, (void *) NULL);
         debug_msg("cleanup thread has been started");
      }

   if(! gmond_config.mute )
      {
         collect_send_pool = g3_thread_pool_create( 4, 128, 1);
         for(i = 0 ; i < gmond_config.num_send_channels; i++)
           {
             send_sockets[i] = Udp_connect( gmond_config.send_channels[i],
                                               gmond_config.send_ports[i]);
             debug_msg("sending messages on channel %s %s", 
		    gmond_config.send_channels[i], gmond_config.send_ports[i]);

             if(is_multicast( gmond_config.send_channels[i] ))
               {
                 /* Set the TTL for the multicast channel.  In the future, this will have
                    meaning in the context of plain UDP channels too. */
                 Mcast_set_ttl( send_sockets[i], gmond_config.mcast_ttl); 
               }
           }

        /* Start up the threads for monitoring and sending metrics */
        for( i = 1 ; i < num_key_metrics; i++ )
          {
            metric[i].key = i;
            g3_run( collect_send_pool, monitor_thread, &metric[i]);
          }
      }
   
   for(;;)
      {
         pause();
      }
   return 0;
}
