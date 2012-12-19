#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <rrd.h>
#include <gmetad.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/poll.h>

#include "rrd_helpers.h"

#define PATHSIZE 4096
extern gmetad_config_t gmetad_config;

pthread_mutex_t rrd_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline void
my_mkdir ( const char *dir )
{
   pthread_mutex_lock( &rrd_mutex );
   if ( mkdir ( dir, 0755 ) < 0 && errno != EEXIST)
      {
         pthread_mutex_unlock(&rrd_mutex);
         err_sys("Unable to mkdir(%s)",dir);
      }
   pthread_mutex_unlock( &rrd_mutex );
}

static int
RRD_update( char *rrd, const char *sum, const char *num, unsigned int process_time )
{
   char *argv[3];
   int   argc = 3;
   char val[128];

   /* If we are a host RRD, we "sum" over only one host. */
   if (num)
      sprintf(val, "%u:%s:%s", process_time, sum, num);
   else
      sprintf(val, "%u:%s", process_time, sum);

   argv[0] = "dummy";
   argv[1] = rrd;
   argv[2] = val; 

   pthread_mutex_lock( &rrd_mutex );
   optind=0; opterr=0;
   rrd_clear_error();
   rrd_update(argc, argv);
   if(rrd_test_error())
      {
         err_msg("RRD_update (%s): %s", rrd, rrd_get_error());
         pthread_mutex_unlock( &rrd_mutex );
         return 0;
      } 
   /* debug_msg("Updated rrd %s with value %s", rrd, val); */
   pthread_mutex_unlock( &rrd_mutex );
   return 0;
}


/* Warning: RRD_create will overwrite a RRdb if it already exists */
static int
RRD_create( char *rrd, int summary, unsigned int step, 
            unsigned int process_time, ganglia_slope_t slope)
{
   const char *data_source_type = "GAUGE";
   char *argv[128];
   int  argc=0;
   int heartbeat;
   char s[16], start[64];
   char sum[64];
   char num[64];
   int i;

   /* Our heartbeat is twice the step interval. */
   heartbeat = 8*step;

   switch( slope) {
   case GANGLIA_SLOPE_POSITIVE:
     data_source_type = "COUNTER";
     break;

   case GANGLIA_SLOPE_DERIVATIVE:
     data_source_type = "DERIVE";
     break;

   case GANGLIA_SLOPE_ZERO:
   case GANGLIA_SLOPE_NEGATIVE:
   case GANGLIA_SLOPE_BOTH:
   case GANGLIA_SLOPE_UNSPECIFIED:
     data_source_type = "GAUGE";
     break;
   }

   argv[argc++] = "dummy";
   argv[argc++] = rrd;
   argv[argc++] = "--step";
   sprintf(s, "%u", step);
   argv[argc++] = s;
   argv[argc++] = "--start";
   sprintf(start, "%u", process_time-1);
   argv[argc++] = start;
   sprintf(sum,"DS:sum:%s:%d:U:U",
           data_source_type,
           heartbeat);
   argv[argc++] = sum;
   if (summary) {
      sprintf(num,"DS:num:%s:%d:U:U", 
              data_source_type,
              heartbeat);
      argv[argc++] = num;
   }

   for(i = 0; i< gmetad_config.num_RRAs; i++)
     {
       argv[argc++] = gmetad_config.RRAs[i];
     }
#if 0
   /* Read in or defaulted in conf.c */
   argv[argc++] = "RRA:AVERAGE:0.5:1:240";
   argv[argc++] = "RRA:AVERAGE:0.5:24:240";
   argv[argc++] = "RRA:AVERAGE:0.5:168:240";
   argv[argc++] = "RRA:AVERAGE:0.5:672:240";
   argv[argc++] = "RRA:AVERAGE:0.5:5760:370";
#endif

   pthread_mutex_lock( &rrd_mutex );
   optind=0; opterr=0;
   rrd_clear_error();
   rrd_create(argc, argv);
   if(rrd_test_error())
      {
         err_msg("RRD_create: %s", rrd_get_error());
         pthread_mutex_unlock( &rrd_mutex );
         return 1;
      }
   debug_msg("Created rrd %s", rrd);
   pthread_mutex_unlock( &rrd_mutex );
   return 0;
}


/* A summary RRD has a "num" and a "sum" DS (datasource) whereas the
   host rrds only have "sum" (since num is always 1) */
static int
push_data_to_rrd( char *rrd, const char *sum, const char *num,
                  unsigned int step, unsigned int process_time,
                  ganglia_slope_t slope)
{
   int rval;
   int summary;
   struct stat st;

   /*  if process_time is undefined, we set it to the current time */
   if (!process_time)
      process_time = time(0);

   if (num)
      summary=1;
   else
      summary=0;

   if( stat(rrd, &st) )
      {
         rval = RRD_create( rrd, summary, step, process_time, slope);
         if( rval )
            return rval;
      }
   return RRD_update( rrd, sum, num, process_time );
}

/* Assumes num argument will be NULL for a host RRD. */
int
write_data_to_rrd ( const char *source, const char *host, const char *metric, 
                    const char *sum, const char *num, unsigned int step,
                    unsigned int process_time, ganglia_slope_t slope)
{
   char rrd[ PATHSIZE + 1 ];
   char *summary_dir = "__SummaryInfo__";
   int i;

   /* Build the path to our desired RRD file. Assume the rootdir exists. */
   strncpy(rrd, gmetad_config.rrd_rootdir, PATHSIZE);

   if (source) {
      strncat(rrd, "/", PATHSIZE-strlen(rrd));
      strncat(rrd, source, PATHSIZE-strlen(rrd));
      my_mkdir( rrd );
   }

   if (host) {
      strncat(rrd, "/", PATHSIZE-strlen(rrd));
      i = strlen(rrd);
      strncat(rrd, host, PATHSIZE-strlen(rrd));
      if(gmetad_config.case_sensitive_hostnames == 0) {
         /* Convert the hostname to lowercase */
         for( ; rrd[i] != 0; i++)
            rrd[i] = tolower(rrd[i]);
      }
      my_mkdir( rrd );
   }
   else {
      strncat(rrd, "/", PATHSIZE-strlen(rrd));
      strncat(rrd, summary_dir, PATHSIZE-strlen(rrd));
      my_mkdir( rrd );
   }

   strncat(rrd, "/", PATHSIZE-strlen(rrd));
   strncat(rrd, metric, PATHSIZE-strlen(rrd));
   strncat(rrd, ".rrd", PATHSIZE-strlen(rrd));

   return push_data_to_rrd( rrd, sum, num, step, process_time, slope);
}

void init_sockaddr (struct sockaddr_in *name, const char *hostname, uint16_t port)
{
  struct hostent *hostinfo;

  name->sin_family = AF_INET;
  name->sin_port = htons (port);
  hostinfo = gethostbyname (hostname);
  if (hostinfo == NULL)
    {
      fprintf (stderr, "Unknown host %s.\n", hostname);
      exit (EXIT_FAILURE);
    }
  name->sin_addr = *(struct in_addr *) hostinfo->h_addr;
}

static int
push_data_to_carbon( char *graphite_msg)
{
  int port;
  int carbon_socket;
  struct sockaddr_in server;
  int carbon_timeout ;
  int nbytes;
  struct pollfd carbon_struct_poll;
  int poll_rval;
  int fl;

  if (gmetad_config.carbon_timeout)
      carbon_timeout=gmetad_config.carbon_timeout;
  else
      carbon_timeout = 500;

  if (gmetad_config.carbon_port)
     port=gmetad_config.carbon_port;
  else
     port=2003;


  debug_msg("Carbon Proxy:: sending \'%s\' to %s", graphite_msg, gmetad_config.carbon_server);

      /* Create a socket. */
  carbon_socket = socket (PF_INET, SOCK_STREAM, 0);
  if (carbon_socket < 0)
    {
      err_msg("socket (client): %s", strerror(errno));
      close (carbon_socket);
      return EXIT_FAILURE;
    }

  /* Set the socket to not block */
  fl = fcntl(carbon_socket,F_GETFL,0);
  fcntl(carbon_socket,F_SETFL,fl | O_NONBLOCK);

  /* Connect to the server. */
  init_sockaddr (&server, gmetad_config.carbon_server, port);
  connect (carbon_socket, (struct sockaddr *) &server, sizeof (server));

  /* Start Poll */
   carbon_struct_poll.fd=carbon_socket;
   carbon_struct_poll.events = POLLOUT;
   poll_rval = poll( &carbon_struct_poll, 1, carbon_timeout ); // default timeout .5s

  /* Send data to the server when the socket becomes ready */
  if( poll_rval < 0 ) {
    debug_msg("carbon proxy:: poll() error");
  } else if ( poll_rval == 0 ) {
    debug_msg("carbon proxy:: Timeout connecting to %s",gmetad_config.carbon_server);
  } else {
    if( carbon_struct_poll.revents & POLLOUT ) {
      /* Ready to send data to the server. */
      debug_msg("carbon proxy:: %s is ready to receive",gmetad_config.carbon_server);
      nbytes = write (carbon_socket, graphite_msg, strlen(graphite_msg) + 1);
      if (nbytes < 0) {
        err_msg("write: %s", strerror(errno));
        close(carbon_socket);
        return EXIT_FAILURE;
      }
    } else if ( carbon_struct_poll.revents & POLLHUP ) {
      debug_msg("carbon proxy:: Recvd an RST from %s during transmission",gmetad_config.carbon_server);
     close(carbon_socket);
     return EXIT_FAILURE;
    } else if ( carbon_struct_poll.revents & POLLERR ) {
      debug_msg("carbon proxy:: Recvd an POLLERR from %s during transmission",gmetad_config.carbon_server);
      close(carbon_socket);
      return EXIT_FAILURE;
    }
  }
  close (carbon_socket);
  return EXIT_SUCCESS;
}

int
write_data_to_carbon ( const char *source, const char *host, const char *metric, 
                    const char *sum, unsigned int process_time )
{

	char s_process_time[15];
   char graphite_msg[ PATHSIZE + 1 ];
   int i;

   /*  if process_time is undefined, we set it to the current time */
   if (!process_time)
      process_time = time(0);


	sprintf(s_process_time, "%u", process_time);

   /* Build the path */
   strncpy(graphite_msg, gmetad_config.graphite_prefix, PATHSIZE);




   if (source) {
		int sourcelen=strlen(source);		
		char sourcecp[sourcelen+1];

		/* find and replace space for _ in the hostname*/
		for(i=0; i<=sourcelen; i++){
	if ( source[i] == ' ') {
	  sourcecp[i]='_';
	}else{
	  sourcecp[i]=source[i];
	}
      }
		sourcecp[i+1]=0;
          
      strncat(graphite_msg, ".", PATHSIZE-strlen(graphite_msg));
      strncat(graphite_msg, sourcecp, PATHSIZE-strlen(graphite_msg));
   }


   if (host) {
		int hostlen=strlen(host);
		char hostcp[hostlen+1]; 

		/* find and replace . for _ in the hostname*/
		for(i=0; i<=hostlen; i++){
         if ( host[i] == '.') {
           hostcp[i]='_';
         }else{
           hostcp[i]=host[i];
         }
      }
		hostcp[i+1]=0;

      strncat(graphite_msg, ".", PATHSIZE-strlen(graphite_msg));

      i = strlen(graphite_msg);
      strncat(graphite_msg, hostcp, PATHSIZE-strlen(graphite_msg));

      if(gmetad_config.case_sensitive_hostnames == 0) {
         /* Convert the hostname to lowercase */
         for( ; graphite_msg[i] != 0; i++)
            graphite_msg[i] = tolower(graphite_msg[i]);
      }

   }

   strncat(graphite_msg, ".", PATHSIZE-strlen(graphite_msg));
   strncat(graphite_msg, metric, PATHSIZE-strlen(graphite_msg));
   strncat(graphite_msg, " ", PATHSIZE-strlen(graphite_msg));
   strncat(graphite_msg, sum, PATHSIZE-strlen(graphite_msg));
   strncat(graphite_msg, " ", PATHSIZE-strlen(graphite_msg));
   strncat(graphite_msg, s_process_time, PATHSIZE-strlen(graphite_msg));
   strncat(graphite_msg, "\n", PATHSIZE-strlen(graphite_msg));

	graphite_msg[strlen(graphite_msg)+1] = 0;

   return push_data_to_carbon( graphite_msg );
}
