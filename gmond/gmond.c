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

#include "conf.h"
#include "cmdline.h"
#include "metric.h"

/* The entire cluster this gmond knows about */
hash_t *cluster;

int msg_out_socket;
pthread_mutex_t  msg_out_socket_mutex    = PTHREAD_MUTEX_INITIALIZER;

int msg_in_socket_active = 0; 
int msg_in_socket;
pthread_mutex_t  msg_in_socket_mutex     = PTHREAD_MUTEX_INITIALIZER;

int server_socket;
pthread_mutex_t  server_socket_mutex     = PTHREAD_MUTEX_INITIALIZER;

int compressed_socket;
pthread_mutex_t  compressed_socket_mutex = PTHREAD_MUTEX_INITIALIZER;

barrier *msg_listen_barrier, *server_barrier;

/* In monitor.c */
extern void *monitor_thread(void *arg);
extern int send_value ( uint32_t key );

/* In listen.c */
extern void *msg_listen_thread(void *arg);  

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
        /* We are not deaf .. we need to listen.
           If the msg channel is a multicast channel, we need to listen to that channel.
           If the msg channel is NOT multicast, we only listen if msg port is explicitly set
             (meaning are going to collect the UDP messages).
         */
        if(is_multicast( gmond_config.msg_channel ))
           {
             debug_msg("Msg channel is multicast");

             /* Since we are not deaf, we need to join the multicast channel */
             
             msg_in_socket = Udp_client( gmond_config.msg_channel, gmond_config.msg_port,
                                 (void **)&sa, &salen);
             Setsockopt( msg_in_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
             Bind( msg_in_socket, sa, salen );
             Mcast_join( msg_in_socket, sa, salen, 
                       gmond_config.msg_if_given ? gmond_config.msg_if : NULL, 0);
             msg_in_socket_active = 1;
           }
        else
           {
             debug_msg("Msg channel is NOT multicast");

             /* Since the msg channel is NOT multicast, we will only listen for incoming
                traffic if the msg port is explicitly set */
 
             if( gmond_config.msg_port_given )
               {
                 /* Clients will be sending us data, we need to listen on the UDP port */

                 msg_in_socket = Udp_server( gmond_config.msg_channel, gmond_config.msg_port,
                                    &salen);
                 Setsockopt( msg_in_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
                 msg_in_socket_active =1; 
               }
           }
        
         debug_msg("msg channel is %s %s", gmond_config.msg_channel, gmond_config.msg_port);

         debug_msg("Starting socket to listen on XML port %s", gmond_config.xml_port);
         server_socket = Tcp_listen(NULL, gmond_config.xml_port, &salen );

         debug_msg("Starting socket to listen on compressed XML port %s", gmond_config.compressed_xml_port);
         compressed_socket = Tcp_listen(NULL, gmond_config.compressed_xml_port, &salen);

         /* thread(s) to listen to the incoming traffic (if necessary) */
         if(msg_in_socket_active)
           {
             if(barrier_init(&msg_listen_barrier, gmond_config.msg_threads))
               {
                 perror("barrier_init() error");
                 return -1;    
               }

             for ( i = 0 ; i < gmond_config.msg_threads; i++ )
               {
                 pthread_create(&tid, &attr, msg_listen_thread, (void *)msg_listen_barrier);
               }
             debug_msg("listening thread(s) have been started");
           }


         /* threads to answer requests for XML */
         if(barrier_init(&server_barrier, gmond_config.xml_threads + 
                                          gmond_config.compressed_xml_threads))
            {
               perror("barrier_init() error");
               return -1;
            }

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

   /* fd for outgoing multicast messages */
   if(! gmond_config.mute )
      {
         /* We are not mute, we need our data out on the msg channel.
            It doesn't matter if the msg channel is multicast or not because
            both are just simple UDP socket connections */
 
         msg_out_socket = Udp_connect( gmond_config.msg_channel, gmond_config.msg_port );
         debug_msg("sending messages on channel %s %s", 
		    gmond_config.msg_channel, gmond_config.msg_port);

         if(is_multicast( gmond_config.msg_channel ))
           {
             /* Set the TTL for the multicast channel.  In the future, this will have
                meaning in the context of plain UDP channels too. */
             Mcast_set_ttl( msg_out_socket, gmond_config.msg_ttl); 
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
