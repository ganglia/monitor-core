/* $Id$ */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <signal.h>

#include "libmetrics.h"

#include "metric.h"

#include "interface.h"
#include "dotconf.h"

#include "lib/debug_msg.h"
#include "lib/error.h"
#include "lib/hash.h"
#include "lib/llist.h"
#include "lib/net.h"
#include "expat.h"
#include "lib/gmond_config.h"
#include "lib/hash.h"
#include "lib/barrier.h"
#include "lib/become_a_nobody.h"
#include "lib/net.h"
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
               /* Will the next multicast be forced beyond the waiting period? */
               if( metric[i].mcast_threshold > (now.tv_sec + waiting_period) )
                  {

                     /* Make sure it multicasts in the 2 mins following the 
                        waiting period */
                     metric[i].mcast_threshold = now.tv_sec + waiting_period +
                       1+ (int)(120.0*rand()/(RAND_MAX+1.0)); 
                  }
            }
      }
   else /* Gmond has been running at least "waiting_period" secs. */
      {
         for (i=1; i< num_key_metrics; i++)
            {
               metric[i].mcast_threshold = 0;
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

int 
main ( int argc, char *argv[] )
{
   int rval, i=0;
   g_val_t initval;
   pthread_t tid;
   pthread_attr_t attr;
   barrier *mcast_listen_barrier, *server_barrier;
   struct timeval tv;
   struct ifi_info *entry = NULL;

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

   /*
   print_gmond_config();
   */
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

   if(! gmond_config.mcast_if_given )
      {
         entry = get_first_multicast_interface();
         if(!entry) {
            err_msg("Warning: Could not find a multicast-enabled interface, using anything we can find.\n");
            entry = get_first_interface();
            if (!entry)
               err_quit("We don't have any interfaces besides loopback, exiting.\n");
         }
         debug_msg("Using interface %s", entry->ifi_name);
      }
   else
      {
         if( is_multicast( gmond_config.mcast_channel ))
           {
             entry = get_interface ( gmond_config.mcast_if );
             if(!entry)
               err_quit("%s is not a valid multicast-enabled interface", gmond_config.mcast_if);
             debug_msg("Using multicast-enabled interface %s", gmond_config.mcast_if);
           }
      }

   /* fd for incoming multicast messages */
   if(! gmond_config.deaf )
      {
	 struct in_addr *addr =  &(((struct sockaddr_in *)(entry->ifi_addr))->sin_addr);
	 if( is_multicast( gmond_config.mcast_channel ))
	     {
                mcast_join_socket = g_mcast_in ( gmond_config.mcast_channel, gmond_config.mcast_port,
                                          addr);
	     }
	 else
	     {
               const int on = 1;
               struct sockaddr_in localaddr;

               fprintf(stderr,"Running UDP server on port %d\n", gmond_config.mcast_port);

	       mcast_join_socket = malloc(sizeof(g_mcast_socket));
               if(!mcast_join_socket)
                 {
                   perror("unable to malloc data for UDP server socket\n");
                   return -1;
                 }
               (mcast_join_socket->ref_count)++;
               mcast_join_socket->sockfd = socket( AF_INET, SOCK_DGRAM, 0);
               if(mcast_join_socket->sockfd < 0)
                 {
                   perror("unable to create UDP server socket\n");
                   return -1;
                 }
               if(setsockopt(mcast_join_socket->sockfd, SOL_SOCKET, SO_REUSEADDR, (void*) &on, sizeof(on)) != 0)
                 {
                   perror("gmond could not setsockopt on UDP server socket");
                   return -1;
                 }
               localaddr.sin_family  = AF_INET;
               localaddr.sin_port    = htons(gmond_config.mcast_port);
               if( gmond_config.send_bind_given )
                 {
                   /* bind to a specific local address */
                   struct in_addr inaddr;
                   /* Try to read the name as if were dotted decimal */
                   if (inet_aton(gmond_config.send_bind, &inaddr) == 0)
                     {
                       fprintf(stderr,"send_bind address is not dotted decimal notation\n");
                       return -1;
                     }
                   memcpy(&localaddr.sin_addr, (char*) &inaddr, sizeof(struct in_addr));
                 }
               else
                 {
                   /* let the kernel decide which local address to bind to */
                   localaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
                 }
               
               /* bind to the local address */
               if (bind(mcast_join_socket->sockfd, (struct sockaddr *)&localaddr, sizeof(localaddr)) <0)
                 {
                   perror("unable to bind to local UDP server address\n");
                   return -1;
                 }

               fprintf(stderr,"UDP SERVER RUNNING\n");
	     }
	      
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
         if(barrier_init(&mcast_listen_barrier, gmond_config.mcast_threads))
            {
               perror("barrier_init() error");
               return -1;    
            }

         for ( i = 0 ; i < gmond_config.mcast_threads; i++ )
            {
               pthread_create(&tid, &attr, mcast_listen_thread, (void *)mcast_listen_barrier);
            }
         debug_msg("listening thread(s) have been started");

         /* threads to answer requests for XML */
         if(barrier_init(&server_barrier, (gmond_config.xml_threads)))
            {
               perror("barrier_init() error");
               return -1;
            }
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
	 if( is_multicast( gmond_config.mcast_channel ))
	   {
	     struct in_addr *addr =   &( ((struct sockaddr_in *)(entry->ifi_addr)) ->sin_addr);
             mcast_socket = g_mcast_out ( gmond_config.mcast_channel, gmond_config.mcast_port,  
                             addr, gmond_config.mcast_ttl);
	   }
	 else
	   {
	     struct sockaddr_in remoteaddr;
             struct in_addr inaddr;
	     const int on = 1;

	     mcast_socket = malloc(sizeof(g_mcast_socket));
	     if(!mcast_socket)
	       {
		 perror("gmond unable to malloc memory for UDP channel\n");
		 return -1;
	       }
	     memset( mcast_socket, 0, sizeof(g_mcast_socket));
	     (mcast_socket->ref_count)++;

	     mcast_socket->sockfd = socket( AF_INET, SOCK_DGRAM, 0);
	     if(mcast_socket->sockfd < 0)
	       {
		 perror("gmond could not create UDP socket");
		 return -1;
	       }

	     if(setsockopt(mcast_socket->sockfd, SOL_SOCKET, SO_REUSEADDR, (void*) &on, sizeof(on)) != 0)
	       {
		 perror("gmond could not setsockopt on UDP socket");
		 return -1;
	       }

	     /* create the sockaddr_in structure */
	     if (inet_aton(gmond_config.mcast_channel, &inaddr) == 0)
	       {
		 fprintf(stderr,"mcast_channel/send_channel is not in dotted decimal notation\n");
		 return -1;
	       }
	     remoteaddr.sin_family = AF_INET;
             remoteaddr.sin_port   = htons(gmond_config.mcast_port);
             memcpy(&remoteaddr.sin_addr, (char*) &inaddr, sizeof(struct in_addr));

	     /* connect the socket to the UDP address */
	     if(connect(mcast_socket->sockfd, (struct sockaddr *)&remoteaddr, sizeof(remoteaddr)) < 0)
	       {
		 fprintf(stderr,"unable to connect to UDP channel '%s:%d\n", gmond_config.mcast_channel,
			 gmond_config.mcast_port);
		 return -1;
	       }
	   }

         if ( !mcast_socket )
            {
               perror("gmond could not connect to multicast channel");
               return -1;
            }
         debug_msg("multicasting on channel %s %d", 
		    gmond_config.mcast_channel, gmond_config.mcast_port);

         pthread_create(&tid, &attr, monitor_thread, NULL);
         debug_msg("created monitor thread");
      }

   
   for(;;)
      {
         pause();
      }
   return 0;
}
