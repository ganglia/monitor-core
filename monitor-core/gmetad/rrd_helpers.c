/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <rrd.h>
#include <gmetad.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>

#define PATHSIZE 4096
extern char * rrd_rootdir;


pthread_mutex_t rrd_mutex = PTHREAD_MUTEX_INITIALIZER;

static void inline
my_mkdir ( char *dir )
{
   if ( mkdir ( dir, 0755 ) < 0 && errno != EEXIST)
      {
         err_sys("Unable to mkdir(%s)",dir);
      }
}

static int
RRD_update( char *rrd, char *sum, char *num, unsigned int process_time )
{
   char *argv[3];
   int   argc = 3;
   char val[128];

   /*  if process_time is undefined, we set it to the current time */

   if (!process_time)
      process_time = time(0);

   /* If we are a host RRD, we "sum" over only one host. */
   if (num)
      sprintf(val, "%d:%s:%s", process_time, sum, num);
   else
      sprintf(val, "%d:%s", process_time, sum);

   argv[0] = "dummy";
   argv[1] = rrd;
   argv[2] = val; 

   pthread_mutex_lock( &rrd_mutex );
   optind=0; opterr=0;
   rrd_clear_error();
   rrd_update(argc, argv);
   if(rrd_test_error())
      {
         err_msg("RRD_update: %s", rrd_get_error());
         pthread_mutex_unlock( &rrd_mutex );
         return 1;
      } 
   debug_msg("Updated rrd %s with value %s", rrd, val);
   pthread_mutex_unlock( &rrd_mutex );
   return 0;
}


/* Warning: RRD_create will overwrite a RRdb if it already exists */
static int
RRD_create( char *rrd, int summary, unsigned int step)
{
   char *argv[15];
   int  argc=0;
   int heartbeat;
   char s[16];
   char sum[64];
   char num[64];

   /* Our heartbeat is twice the step interval. */
   heartbeat = 8*step;

   argv[argc++] = "dummy";
   argv[argc++] = rrd;
   argv[argc++] = "--step";
   sprintf(s, "%u", step);
   argv[argc++] = s;
   sprintf(sum,"DS:sum:GAUGE:%d:U:U", heartbeat);
   argv[argc++] = sum;
   if (summary) {
      sprintf(num,"DS:num:GAUGE:%d:U:U", heartbeat);
      argv[argc++] = num;
   }
   argv[argc++] = "RRA:AVERAGE:0.5:1:240";
   argv[argc++] = "RRA:AVERAGE:0.5:24:240";
   argv[argc++] = "RRA:AVERAGE:0.5:168:240";
   argv[argc++] = "RRA:AVERAGE:0.5:672:240";
   argv[argc++] = "RRA:AVERAGE:0.5:5760:370";

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
push_data_to_rrd( char *rrd, char *sum, char *num, unsigned int step, unsigned int process_time)
{
   int rval;
   int summary;
   struct stat st;

   if (num)
      summary=1;
   else
      summary=0;

   if( stat(rrd, &st) )
      {
         rval = RRD_create( rrd, summary, step );
         if( rval )
            return rval;
      }
   return RRD_update( rrd, sum, num, process_time );
}


/* Assumes num argument will be NULL for a host RRD. */
int
write_data_to_rrd ( char *source, char *host, char *metric, char *sum, char *num, unsigned int step, unsigned int process_time )
{
   char rrd[ PATHSIZE ];
   char *summary_dir = "__SummaryInfo__";

   /* Build the path to our desired RRD file. Assume the rootdir exists. */
   strcpy(rrd, rrd_rootdir);

   if (source) {
      strncat(rrd, "/", PATHSIZE);
      strncat(rrd, source, PATHSIZE);
      my_mkdir( rrd );
   }

   if (host) {
      strncat(rrd, "/", PATHSIZE);
      strncat(rrd, host, PATHSIZE);
      my_mkdir( rrd );
   }
   else {
      strncat(rrd, "/", PATHSIZE);
      strncat(rrd, summary_dir, PATHSIZE);
      my_mkdir( rrd );
   }

   strncat(rrd, "/", PATHSIZE);
   strncat(rrd, metric, PATHSIZE);
   strncat(rrd, ".rrd", PATHSIZE);

   return push_data_to_rrd( rrd, sum, num, step, process_time );

   /* Shouldn't get here */
   return 1;
}

