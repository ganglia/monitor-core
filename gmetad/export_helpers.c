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
#include <arpa/inet.h>
#include <apr_time.h>
#include <netdb.h>

#ifdef WITH_MEMCACHED
#include <libmemcached-1.0/memcached.h>
#include <libmemcachedutil-1.0/util.h>
#endif /* WITH_MEMCACHED */

#ifdef WITH_RIEMANN
#include "riemann.pb-c.h"

extern pthread_mutex_t  riemann_cb_mutex;

int riemann_circuit_breaker = RIEMANN_CB_CLOSED;
apr_time_t riemann_reset_timeout = 0;
int riemann_failures = 0;
#endif /* WITH_RIEMANN */

#include "export_helpers.h"
#include "gm_scoreboard.h"

#define MAX_PORT_LEN 6
#define PATHSIZE 4096
extern gmetad_config_t gmetad_config;

g_udp_socket *carbon_udp_socket;
pthread_mutex_t  carbon_mutex = PTHREAD_MUTEX_INITIALIZER;

g_udp_socket*
init_carbon_udp_socket (const char *hostname, uint16_t port)
{
   int sockfd;
   g_udp_socket* s;
   struct sockaddr_in *sa_in;
   struct hostent *hostinfo;

   sockfd = socket(AF_INET, SOCK_DGRAM, 0);
   if (sockfd < 0)
      {
         err_msg("create socket (client): %s", strerror(errno));
         return NULL;
      }

   s = malloc( sizeof( g_udp_socket ) );
   memset( s, 0, sizeof( g_udp_socket ));
   s->sockfd = sockfd;
   s->ref_count = 1;

   /* Set up address and port for connection */
   sa_in = (struct sockaddr_in*) &s->sa;
   sa_in->sin_family = AF_INET;
   sa_in->sin_port = htons (port);
   hostinfo = gethostbyname (hostname);
   sa_in->sin_addr = *(struct in_addr *) hostinfo->h_addr;

   return s;
}

#ifdef WITH_RIEMANN
g_udp_socket *riemann_udp_socket;
g_tcp_socket *riemann_tcp_socket;

pthread_mutex_t  riemann_socket_mutex = PTHREAD_MUTEX_INITIALIZER;

g_udp_socket*
init_riemann_udp_socket (const char *hostname, uint16_t port)
{
   int sockfd;
   g_udp_socket* s;
   struct sockaddr_in *sa_in;
   struct hostent *hostinfo;

   sockfd = socket(AF_INET, SOCK_DGRAM, 0);
   if (sockfd < 0)
      return NULL;

   s = malloc( sizeof( g_udp_socket ) );
   memset( s, 0, sizeof( g_udp_socket ));
   s->sockfd = sockfd;
   s->ref_count = 1;

   /* Set up address and port for connection */
   sa_in = (struct sockaddr_in*) &s->sa;
   sa_in->sin_family = AF_INET;
   sa_in->sin_port = htons (port);
   hostinfo = gethostbyname (hostname);
   if (!hostinfo)
      err_quit("Unknown host %s", hostname);
   sa_in->sin_addr = *(struct in_addr *) hostinfo->h_addr;

   return s;
}

g_udp_socket*
init_riemann_udp6_socket (const char *hostname, uint16_t port)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int sfd;

  g_udp_socket* s;
  int rv;
  char udp_port[MAX_PORT_LEN];

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;          /* Any protocol */
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  snprintf(udp_port, MAX_PORT_LEN, "%d", port);

  rv = getaddrinfo(hostname, udp_port, &hints, &result);
  if (rv != 0)
    err_quit("getaddrinfo: %s\n", gai_strerror(rv));

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == -1)
      continue;
    if (connect(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
      break;
    close(sfd);
  }
  if (rp == NULL)
    return NULL;

  s = malloc( sizeof( g_udp_socket ) );
  memset( s, 0, sizeof( g_udp_socket ));
  s->sockfd = sfd;
  s->ref_count = 1;

  if (riemann_udp_socket)
      close (riemann_udp_socket->sockfd);

  return s;
}


g_tcp_socket*
init_riemann_tcp_socket (const char *hostname, uint16_t port)
{
  int sockfd;
  g_tcp_socket* s;
  struct sockaddr_in *sa_in;
  struct hostent *hostinfo;
  int rv;

  sockfd = socket (AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
      return NULL;

  s = malloc( sizeof( g_tcp_socket ) );
  memset( s, 0, sizeof( g_tcp_socket ));
  s->sockfd = sockfd;
  s->ref_count = 1;

  /* Set up address and port for connection */
  sa_in = (struct sockaddr_in*) &s->sa;
  sa_in->sin_family = AF_INET;
  sa_in->sin_port = htons (port);
  hostinfo = gethostbyname (hostname);
  if (!hostinfo)
      err_quit("Unknown host %s", hostname);
  sa_in->sin_addr = *(struct in_addr *) hostinfo->h_addr;

  if (riemann_tcp_socket)
      close (riemann_tcp_socket->sockfd);

  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = RIEMANN_TCP_TIMEOUT * 1000;

  if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
    {
      close (sockfd);
      free (s);
      return NULL;
    }

  if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
    {
      close (sockfd);
      free (s);
      return NULL;
    }

  rv = connect(sockfd, &s->sa, sizeof(s->sa));
  if (rv != 0)
    {
      close (sockfd);
      free (s);
      return NULL;
    }

  return s;
}

g_tcp_socket*
init_riemann_tcp6_socket (const char *hostname, uint16_t port)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int sfd;

  g_tcp_socket* s;
  int rv;
  char tcp_port[MAX_PORT_LEN];

  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = RIEMANN_TCP_TIMEOUT * 1000;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;    
  hints.ai_protocol = 0;          /* Any protocol */
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  snprintf(tcp_port, MAX_PORT_LEN, "%d", port);

  rv = getaddrinfo(hostname, tcp_port, &hints, &result);
  if (rv != 0)
    err_quit("getaddrinfo: %s\n", gai_strerror(rv));

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == -1)
      continue;
    if (setsockopt (sfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
      close(sfd);
      continue;
    }
    if (setsockopt (sfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
      close(sfd);
      continue;
    }
    if (connect(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
      break;
    close(sfd);
  }
  if (rp == NULL)
    return NULL;
  
  s = malloc( sizeof( g_tcp_socket ) );
  memset( s, 0, sizeof( g_tcp_socket ));
  s->sockfd = sfd;
  s->ref_count = 1;

  if (riemann_tcp_socket)
      close (riemann_tcp_socket->sockfd);

  return s;
}
#endif /* WITH_RIEMANN */

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

  if (!strcmp(gmetad_config.carbon_protocol, "tcp"))
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
      ganglia_scoreboard_inc(METS_SENT_GRAPHITE);
      ganglia_scoreboard_inc(METS_SENT_ALL);
      return EXIT_SUCCESS;

  } else {

      int nbytes;

      pthread_mutex_lock( &carbon_mutex );
      nbytes = sendto (carbon_udp_socket->sockfd, graphite_msg, strlen(graphite_msg), 0,
                         (struct sockaddr_in*)&carbon_udp_socket->sa, sizeof (struct sockaddr_in));
      pthread_mutex_unlock( &carbon_mutex );

      if (nbytes != strlen(graphite_msg))
      {
             err_msg("sendto socket (client): %s", strerror(errno));
             return EXIT_FAILURE;
      }
      ganglia_scoreboard_inc(METS_SENT_GRAPHITE);
      ganglia_scoreboard_inc(METS_SENT_ALL);
      return EXIT_SUCCESS;
  }
}

#ifdef WITH_MEMCACHED
#define MEMCACHED_MAX_KEY_LENGTH 250 /* Maximum allowed by memcached */
int
write_data_to_memcached ( const char *cluster, const char *host, const char *metric, 
                    const char *sum, unsigned int process_time, unsigned int expiry )
{
   time_t expiry_time;
   char s_path[MEMCACHED_MAX_KEY_LENGTH];
   
   // Check whether we are including cluster in the memcached key
   if ( gmetad_config.memcached_include_cluster_in_key ) {
      if (strlen(cluster) + strlen(host) + strlen(metric) + 3 > MEMCACHED_MAX_KEY_LENGTH) {
	  debug_msg("Cluster + host + metric + 3 > %d", MEMCACHED_MAX_KEY_LENGTH);
	  return EXIT_FAILURE;
      }
      sprintf(s_path, "%s/%s/%s", cluster, host, metric);
    } else {
      if ( strlen(host) + strlen(metric) + 3 > MEMCACHED_MAX_KEY_LENGTH) {
	  debug_msg("host + metric + 3 > %d", MEMCACHED_MAX_KEY_LENGTH);
	  return EXIT_FAILURE;
      }
      sprintf(s_path, "%s/%s", host, metric);     
   }
    
    
   if (expiry != 0) {
      expiry_time = expiry;
   } else {
      expiry_time = (time_t) 0;
   }

   memcached_return_t rc;
   memcached_st *memc = memcached_pool_pop(memcached_connection_pool, false, &rc);
   if (rc != MEMCACHED_SUCCESS) {
      debug_msg("Unable to retrieve a memcached connection from the pool");
      return EXIT_FAILURE;
   }
   rc = memcached_set(memc, s_path, strlen(s_path), sum, strlen(sum), expiry_time, (uint32_t)0);
   if (rc != MEMCACHED_SUCCESS) {
      debug_msg("Unable to push %s value %s to the memcached server(s) - %s", s_path, sum, memcached_strerror(memc, rc));
      memcached_pool_push(memcached_connection_pool, memc);
      return EXIT_FAILURE;
   } else {
      debug_msg("Pushed %s value %s to the memcached server(s)", s_path, sum);
      memcached_pool_push(memcached_connection_pool, memc);
      ganglia_scoreboard_inc(METS_SENT_MEMCACHED);
      ganglia_scoreboard_inc(METS_SENT_ALL);
      return EXIT_SUCCESS;
   }
}
#endif /* WITH_MEMCACHED */

/* This function replaces the macros (%s, %m etc..) in the given graphite_string*/
char *
path_macro_replace(char *path, graphite_path_macro *patrn)
{
	char *final=malloc(PATHSIZE); //heap-side so we can pass it back
	char path_cp[PATHSIZE]; //copy of path so we can clobber it
	char *prefix; 
	char *suffix;
	char *offset;     

	strncpy(final, path, PATHSIZE);
	strncpy(path_cp, path, PATHSIZE);
	for(int i=0; patrn[i].torepl != 0; i++){
		while((offset = strstr(path_cp, patrn[i].torepl)))
		{
			prefix=path_cp; //pointer to the beginning of path_cp (for clarity)
			suffix=offset+(strlen(patrn[i].torepl));// get a pointer to after patrn 
			*offset='\0'; // split the path_cp string at the first byte of patrn
			snprintf(final,PATHSIZE,"%s%s%s",prefix,patrn[i].replwith,suffix); //build a new final from the pieces
			strncpy(path_cp, final,PATHSIZE); 
		} 
	}
	return final;
}

int
write_data_to_carbon ( const char *source, const char *host, const char *metric, 
                    const char *sum, unsigned int process_time )
{

	int hostlen=strlen(host);
	char hostcp[hostlen+1]; 
	int sourcelen=strlen(source);		
	char sourcecp[sourcelen+1];
    int metriclen=strlen(metric);
    char metriccp[metriclen+1];
	char s_process_time[15];
   char graphite_msg[ PATHSIZE + 1 ];
   int i;
                                                                                                                                                                                               
	/*  if process_time is undefined, we set it to the current time */
	if (!process_time) process_time = time(0);
	sprintf(s_process_time, "%u", process_time);

   /* prepend everything with graphite_prefix if it's set */
   if (gmetad_config.graphite_prefix != NULL && strlen(gmetad_config.graphite_prefix) > 1) {
     strncpy(graphite_msg, gmetad_config.graphite_prefix, PATHSIZE);
   }

	/*prep the source name*/
   if (source) {

		/* find and replace space for _ in the sourcename*/
		for(i=0; i<=sourcelen; i++){
			if ( source[i] == ' ') {
	  			sourcecp[i]='_';
			}else{
	  			sourcecp[i]=source[i];
			}
      }
		sourcecp[i+1]=0;
      }


   /* prep the host name*/
   if (host) {
		/* find and replace . for _ in the hostname*/
		for(i=0; i<=hostlen; i++){
         if ( host[i] == '.') {
           hostcp[i]='_';
         }else{
           if (gmetad_config.case_sensitive_hostnames == 0) {
             hostcp[i] = tolower(host[i]);
           }
           else {
             hostcp[i] = host[i];
           }
         }
      }
		hostcp[i+1]=0;
      i = strlen(graphite_msg);
      if(gmetad_config.case_sensitive_hostnames == 0) {
         /* Convert the hostname to lowercase */
         for( ; graphite_msg[i] != 0; i++)
            graphite_msg[i] = tolower(graphite_msg[i]);
      }
   }

   if (metric) {
     for(i=0; i <= metriclen; i++) {
       if (metric[i] == ' ') {
         metriccp[i] = '_';
       }
       else {
         metriccp[i] = metric[i];
       }
     }
   }

	/*if graphite_path is set, then process it*/
   if (gmetad_config.graphite_path != NULL && strlen(gmetad_config.graphite_path) > 1) {
		graphite_path_macro patrn[4]; //macros we need to replace in graphite_path
   	char graphite_path_cp[ PATHSIZE + 1 ]; //copy of graphite_path
   	char *graphite_path_ptr; //a pointer to catch returns from path_macro_replace()
   	strncpy(graphite_path_cp,gmetad_config.graphite_path,PATHSIZE);

		patrn[0].torepl="%s";
		patrn[0].replwith=sourcecp;
		patrn[1].torepl="%h";
		patrn[1].replwith=hostcp;
		patrn[2].torepl="%m";
		patrn[2].replwith=metriccp;
		patrn[3].torepl='\0'; //explicitly cap the array

		graphite_path_ptr=path_macro_replace(graphite_path_cp, patrn); 
		strncpy(graphite_path_cp,graphite_path_ptr,PATHSIZE);
		free(graphite_path_ptr);//malloc'd in path_macro_replace()

		/* add the graphite_path to graphite_msg (with a dot first if prefix exists) */
   	if (gmetad_config.graphite_prefix != NULL && strlen(gmetad_config.graphite_prefix) > 1) {
   		strncat(graphite_msg, ".", PATHSIZE-strlen(graphite_msg));
   		strncat(graphite_msg, graphite_path_cp, PATHSIZE-strlen(graphite_msg));
   	} else {
            strncpy(graphite_msg, graphite_path_cp, PATHSIZE);
		}
       
   }else{ /* no graphite_path specified, so do things the old way */
   	if (gmetad_config.graphite_prefix != NULL && strlen(gmetad_config.graphite_prefix) > 1) {
			strncat(graphite_msg, ".", PATHSIZE-strlen(graphite_msg));
   	   strncat(graphite_msg, sourcecp, PATHSIZE-strlen(graphite_msg));
   	} else {
     		strncpy(graphite_msg, sourcecp, PATHSIZE);
   	}
      strncat(graphite_msg, ".", PATHSIZE-strlen(graphite_msg));
      strncat(graphite_msg, hostcp, PATHSIZE-strlen(graphite_msg));
   	strncat(graphite_msg, ".", PATHSIZE-strlen(graphite_msg));
   	strncat(graphite_msg, metric, PATHSIZE-strlen(graphite_msg));
  	}

	/* finish off with the value and date (space separated) */
	strncat(graphite_msg, " ", PATHSIZE-strlen(graphite_msg));
  	strncat(graphite_msg, sum, PATHSIZE-strlen(graphite_msg));
  	strncat(graphite_msg, " ", PATHSIZE-strlen(graphite_msg));
  	strncat(graphite_msg, s_process_time, PATHSIZE-strlen(graphite_msg));
  	strncat(graphite_msg, "\n", PATHSIZE-strlen(graphite_msg));

	graphite_msg[strlen(graphite_msg)+1] = 0;
   return push_data_to_carbon( graphite_msg );
}

#ifdef WITH_RIEMANN

int
tokenize (const char *str, char *delim, char **tokens)
{
  char *copy = strdup (str);
  char *p;
  char *last;
  int i = 0;

  p = strtok_r (copy, delim, &last);
  while (p != NULL) {
    tokens[i] = malloc (strlen (p) + 1);
    if (tokens[i])
      strcpy (tokens[i], p);
    i++;
    p = strtok_r (NULL, delim, &last);
  }
  free (copy);
  return i++;
}

Event *
create_riemann_event (const char *grid, const char *cluster, const char *host, const char *ip,
                      const char *metric, const char *value, const char *type, const char *units,
                      const char *state, unsigned int localtime, const char *tags_str,
                      const char *location, unsigned int ttl)
{
/*
  debug_msg("[riemann] grid=%s, cluster=%s, host=%s, ip=%s, metric=%s, value=%s %s, type=%s, state=%s, "
            "localtime=%u, tags=%s, location=%s, ttl=%u\n", grid, cluster, host, ip, metric, value,
            units, type, state, localtime, tags_str, location, ttl);
*/
  Event *event = malloc (sizeof (Event));
  event__init (event);

  event->host = strdup (host);
  event->service = strdup (metric);

  if (value) {
    if (!strcmp (type, "int")) {
      event->has_metric_sint64 = 1;
      event->metric_sint64 = strtol (value, (char **) NULL, 10);
    }
    else if (!strcmp (type, "float")) {
      event->has_metric_d = 1;
      event->metric_d = (double) strtod (value, (char **) NULL);
    }
    else {
      event->state = strdup (value);
    }
  }

  event->description = strdup (units);

  if (state)
    event->state = strdup (state);

  if (localtime)
    event->time = localtime;

  char *tags[64] = { NULL };

  event->n_tags = tokenize (tags_str, ",", tags);
  event->tags = malloc (sizeof (char *) * (event->n_tags));
  int j;
  for (j = 0; j< event->n_tags; j++) {
     event->tags[j] = strdup (tags[j]);
     free(tags[j]);
  }

  char attr_str[512];
  sprintf(attr_str, "grid=%s,cluster=%s,ip=%s,location=%s%s%s", grid, cluster, ip, location,
        gmetad_config.riemann_attributes ? "," : "",
        gmetad_config.riemann_attributes ? gmetad_config.riemann_attributes : "");

  char *kv[64] = { NULL };
  event->n_attributes = tokenize (attr_str, ",", kv);

  Attribute **attrs;
  attrs = malloc (sizeof (Attribute *) * (event->n_attributes));

  int i;
  for (i = 0; i < event->n_attributes; i++) {

    char *pair[2] = { NULL };
    tokenize (kv[i], "=", pair);
    free(kv[i]);

    attrs[i] = malloc (sizeof (Attribute));
    attribute__init (attrs[i]);
    attrs[i]->key = strdup(pair[0]);
    attrs[i]->value = strdup(pair[1]);
    free(pair[0]);
    free(pair[1]);
  }
  event->attributes = attrs;

  event->has_ttl = 1;
  event->ttl = ttl;
/*
  debug_msg("[riemann] %zu host=%s, service=%s, state=%s, metric_f=%f, metric_d=%lf, metric_sint64=%" PRId64
            ", description=%s, ttl=%f, tags(%zu), attributes(%zu)", event->time, event->host, event->service,
            event->state, event->metric_f, event->metric_d, event->metric_sint64, event->description,
            event->ttl, event->n_tags, event->n_attributes);
*/
  return event;
}

int
send_event_to_riemann (Event *event)
{
   size_t len, nbytes;
   void *buf;
   int errsv;

   Msg *riemann_msg = malloc (sizeof (Msg));
   msg__init (riemann_msg);
   riemann_msg->events = malloc(sizeof (Event));
   riemann_msg->n_events = 1;
   riemann_msg->events[0] = event;

   len = msg__get_packed_size(riemann_msg);
   buf = malloc(len);
   msg__pack(riemann_msg, buf);

   pthread_mutex_lock( &riemann_socket_mutex );
   if ((nbytes = send(riemann_udp_socket->sockfd, buf, len, 0)) == -1)
      errsv = errno;
   pthread_mutex_unlock( &riemann_socket_mutex );
   free (buf);

   destroy_riemann_msg(riemann_msg);

   if (nbytes == -1) {
      err_msg ("[riemann] UDP connection error: %s", strerror (errsv));
      return EXIT_FAILURE;
   } else if (nbytes != len) {
      err_msg ("[riemann] UDP connection error: failed to send all bytes");
      return EXIT_FAILURE;
   } else {
      debug_msg ("[riemann] Sent 1 event in %lu serialized bytes", (unsigned long)len);
   }
   ganglia_scoreboard_inc(METS_SENT_RIEMANN);
   ganglia_scoreboard_inc(METS_SENT_ALL);
   return EXIT_SUCCESS;
}

int
send_message_to_riemann (Msg *message)
{
   debug_msg("[riemann] send_message_to_riemann()");

   if (riemann_circuit_breaker == RIEMANN_CB_CLOSED) {

      uint32_t len;
      ssize_t nbytes;
      struct {
         uint32_t header;
         uint8_t data[0];
      } *buf;
      int errsv;

      if (!message)
         return EXIT_FAILURE;

      len = msg__get_packed_size (message) + sizeof (buf->header);
      buf = malloc (len);
      msg__pack (message, buf->data);
      buf->header = htonl (len - sizeof (buf->header));

      pthread_mutex_lock( &riemann_socket_mutex );
      if ((nbytes = send (riemann_tcp_socket->sockfd, buf, len, 0)) == -1)
         errsv = errno;
      pthread_mutex_unlock( &riemann_socket_mutex );
      free (buf);

      if (nbytes == -1) {
         err_msg("[riemann] TCP connection error: %s", strerror (errsv));
         pthread_mutex_lock( &riemann_cb_mutex );
         riemann_failures++;
         pthread_mutex_unlock( &riemann_cb_mutex );
         return EXIT_FAILURE;
      } else if (nbytes != len) {
         err_msg("[riemann] TCP connection error: failed to send all bytes");
         pthread_mutex_lock( &riemann_cb_mutex );
         riemann_failures++;
         pthread_mutex_unlock( &riemann_cb_mutex );
         return EXIT_FAILURE;
      } else {
         debug_msg("[riemann] Sent %lu events as 1 message in %lu serialized bytes", (unsigned long)message->n_events, (unsigned long)len);
      }
   }
   ganglia_scoreboard_incby(METS_SENT_RIEMANN, message->n_events);
   ganglia_scoreboard_incby(METS_SENT_ALL, message->n_events);
   return EXIT_SUCCESS;
}

int
destroy_riemann_event(Event *event)
{
   int i;
   if (event->host)
      free(event->host);
   if (event->service)
      free(event->service);
   if (event->state)
      free(event->state);
   if (event->description)
      free(event->description);
   for (i = 0; i < event->n_tags; i++)
      free(event->tags[i]);
   if (event->tags)
      free(event->tags);
   for (i = 0; i < event->n_attributes; i++) {
      free(event->attributes[i]->key);
      free(event->attributes[i]->value);
      free(event->attributes[i]);
   }
   if (event->attributes)
      free(event->attributes);
   free (event);
   return 0;
}

int
destroy_riemann_msg(Msg *message)
{
   int i;
   for (i = 0; i < message->n_events; i++) {
     destroy_riemann_event(message->events[i]);
   }
   if (message->events)
     free(message->events);
   free(message);
   return 0;
}
#endif /* WITH_RIEMANN */

