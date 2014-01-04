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

#ifdef HAVE___THREAD
static __thread int rrdcached_conn = 0;
#else
static pthread_key_t rrdcached_conn_key;
#endif

static inline void
my_mkdir ( const char *dir )
{
   if ( mkdir ( dir, 0755 ) < 0 && errno != EEXIST)
      {
         err_sys("Unable to mkdir(%s)",dir);
      }
}

static int
RRD_update_cached( char *rrd, const char *sum, const char *num, unsigned int process_time )
{
   int *conn, c, r, off, l, to;
   char *cmd, *str, buf[1024];
   struct pollfd pfd[1];

#ifdef HAVE___THREAD
   conn = &rrdcached_conn;
#else
   conn = pthread_getspecific(&rrdcached_conn_key);
   if (conn == NULL)
      {
         conn = calloc(1, sizeof (*conn));
         pthread_setspecific(&rrdcached_conn_key, conn);
         *conn = 0;
      }
#endif

   c = *conn;
   if (c == 0)
      {
reconnect:
         c = socket(PF_INET, SOCK_STREAM, 0);
         if (c == -1)
            {
               err_sys("Unable to create rrdcached socket");
            }

         if (connect(c, &gmetad_config.rrdcached_address,
                  sizeof (gmetad_config.rrdcached_address)) == -1)
            {
               err_sys("Unable to connect to rrdcached at %s", gmetad_config.rrdcached_addrstr);
            }

         r = 1;
         if (ioctl(c, FIONBIO, &r))
            {
               err_sys("Unable to set socket non-blocking");
            }

         *conn = c;
      }

   cmd = calloc(1, strlen(rrd) + 128);
   if (num)
         sprintf(cmd, "UPDATE %s %u:%s:%s\n", rrd, process_time, sum, num);
   else
         sprintf(cmd, "UPDATE %s %u:%s\n", rrd, process_time, sum);

   l = strlen(cmd);
   off = 0;
   do
      {
         r = write(c, cmd + off, l - off);
         off += r;
      }
   while ((off < l) || (r == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)));

   if (r == -1)
      {
         if (errno == ECONNRESET || errno == EPIPE)
            {
               free(cmd);
               goto reconnect;
            }
         err_sys("Error writing command %s to rrdcached\n", cmd);
      }

   free(cmd);

   pfd[0].fd = c;
   pfd[0].events = POLLIN;
   pfd[0].revents = 0;
   off = 0;
   to = 0;
   l = -1;

   while (1)
      {
         int r = poll(pfd, 1, 250);
         
         if (r == 0)
            {
               to += 250;
               if (to >= 5000)
                  {
                     err_msg("Timed out reading from rrdcached");
                     break;
                  }
               continue;
            }
         else if (r == -1)
            {
               if (errno == EAGAIN)
                  {
                     pfd[0].events = POLLIN;
                     pfd[0].revents = 0;
                     continue;
                  }

               break;
            }
         else
            {
               if (pfd[0].revents & POLLERR || pfd[0].revents & POLLHUP)
                  {
                     /* Hack to avoid looping on a flappy socket */
                     to += 5000;
                     goto reconnect;
                  }

               r = read(c, buf, sizeof(buf));
               if (r == 0)
                  {
                     to += 5000;
                     goto reconnect;
                  }
               else if (r == -1)
                  {
                     err_msg("Error reading from rrdcached");
                     break;
                  }

               if (l == -1)
                  {
                     /*
                      * First character output is number of lines to follow.
                      * Search for that many lines in our buffer.
                      */
                     l = buf[0] - 29;
                  }

               str = buf;
               while (l > 0 && (str = memchr(str, '\n', strlen(str))) != NULL)
                  {
                     l--;
                  }

               /* We've read all the data. */
               if (l == 0)
                  {
                     break;
                  }
            }
      }

   return 0;
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

   /* XXX: cache the results of the stat once hash table is fixed. */
   if( stat(rrd, &st) )
      {
         rval = RRD_create( rrd, summary, step, process_time, slope);
         if( rval )
            return rval;
      }
   if (gmetad_config.rrdcached_addrstr != NULL)
      {
         return RRD_update_cached( rrd, sum, num, process_time );
      }
   else
      {
         return RRD_update( rrd, sum, num, process_time );
      }
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
