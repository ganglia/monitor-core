/* $Id$ */
#include <pwd.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

ganglia_thread_pool collect_send_pool = NULL;
int send_sockets[1024];
int send_index;

ganglia_thread_pool receive_pool = NULL;
int receive_sockets[1024];
int receive_index;


int server_socket;
pthread_mutex_t  server_socket_mutex     = PTHREAD_MUTEX_INITIALIZER;

int compressed_socket;
pthread_mutex_t  compressed_socket_mutex = PTHREAD_MUTEX_INITIALIZER;

/* In monitor.c */
extern void monitor_thread(void *arg);

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

  if(!ip)
    return -1;

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
#if GEXEC_SUPPORTED == 1
  int rval;
  struct stat info;

  /* Later on .. we'll pass on errors and change the binary OFF, ON
   * status to be more complex.  For example, a host may both accept
   * gexec jobs and also send gexec jobs.
   */
  rval = stat(GAUTHD_PUBLIC_KEY, &info);
  if(rval <0)
    {
      snprintf(val.str, MAX_G_STRING_SIZE, "OFF");
    }
  else
    {
      snprintf(val.str, MAX_G_STRING_SIZE, "ON");
    }
#else
   snprintf(val.str, MAX_G_STRING_SIZE, "OFF");
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
   int rval, i=0, if_index, multicast;
   g_val_t initval;
   pthread_t tid;
   pthread_attr_t attr;
   struct timeval tv;
   int no_compression = 0;
   struct sockaddr *sa;
   socklen_t salen;
   const int on = 1;
   channel_t *channel;

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

   if(debug_level)
     gmond_print_conf(&gmond_config);

   if(! gmond_config.deaf )
      {
        /* We need to calculate how many channels we will be listening on */
        receive_pool = ganglia_thread_pool_create( gmond_config.num_receive_channels, 128, 1 );

	/* Set the receive socket index to zero */
	receive_index = 0;

        for(i=0; i <= gmond_config.current_channel; i++)
          {
            channel = gmond_config.channels[i];

	    /* We only process the channels with receive actions */
            if(!strstr(channel->action, "receive"))
	      continue;

	    /* Check if the address is multicast */
	    multicast = is_multicast(channel->address) == 1? 1: 0;

	    /* Set the interface index to zero */
	    if_index = 0;

	    do
	      {
	        if(multicast)
	          {
		    /* Multicast channels.. we need to join a Multicast group */
		    receive_sockets[receive_index] = Udp_client( channel->address, channel->port, (void **)&sa, &salen);
		    /* Make sure we have have loopback on (should be by default) */
		    Mcast_set_loop( receive_sockets[receive_index], 1);
		    Setsockopt( receive_sockets[receive_index], SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		    Bind( receive_sockets[receive_index], sa, salen );
		    Mcast_join( receive_sockets[receive_index], sa, salen, 
				channel->num_interfaces ? channel->interfaces[if_index] : NULL, 0);
		    debug_msg("Set up to receive messages on multicast channel %s: %s (%s interface)",
			      channel->address, channel->port, 
			      channel->num_interfaces ? channel->interfaces[if_index]: "kernel decides");
	          }
		else
		  {
		    /* Todo: need to allow explicit routes for unicast routes here */
		    receive_sockets[receive_index] = Udp_server( channel->address, channel->port, &salen);
		    Setsockopt( receive_sockets[receive_index], SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)); 
		  }

                ganglia_run( receive_pool, msg_listen_thread, (void *)&receive_sockets[receive_index]); 
		receive_index++;
		if(receive_index >= 1024)
		  {
		    fprintf(stderr,"The maximum number of receive sockets is 1024\n");
		    exit(1);
		  }

	      } while( ++if_index < channel->num_interfaces ); /* interfaces loop */

           } /* channels loop */
        
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
         collect_send_pool = ganglia_thread_pool_create( 4, 128, 1);

	 send_index = 0;

         for(i = 0 ; i <= gmond_config.current_channel; i++)
           {
	     channel = gmond_config.channels[i];

	     if(!strstr(channel->action, "send"))
	       continue;

	     if_index = 0;
	     do
	       {
                 send_sockets[send_index] = Udp_connect( channel->address, channel->port );
                 debug_msg("Sending UDP messages on channel %s %s", channel->address, channel->port );

                 if(is_multicast( channel->address ) == 1)
                   {
                     /* Set the TTL (and optionally interface) for the multicast channel.  */
                     Mcast_set_ttl( send_sockets[send_index], channel->ttl);
		     if(channel->num_interfaces)
		       {
		         Mcast_set_if( send_sockets[send_index], channel->interfaces[if_index], 0);
		       }
                   }
	         else
	           {
		     /* We will set the TTL / interface for UDP later when we have message forwarding */
	           }

		 send_index++;
		 if(send_index>=1024)
		   {
		     fprintf(stderr,"You have specified over 1024 send channels.\n");
		     exit(1);
		   }

	       } while( ++if_index < channel->num_interfaces );

           } /* channel loop close */

          /* Start up the threads for monitoring and sending metrics */
          for( i = 1 ; i < num_key_metrics; i++ )
            {
              metric[i].key = i;
              ganglia_run( collect_send_pool, monitor_thread, &metric[i]);
            }
        }

   for(;;)
      {
         pause();
      }

   return 0;
}
