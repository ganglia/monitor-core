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

extern char * rrd_rootdir;

static int push_data_to_summary_rrd( char *rrd, char *sum, char *num, char *polling_interval);
static int push_data_to_rrd( char *rrd, char *sum, char *polling_interval);
static int RRD_create( char *rrd, char *polling_interval);
static int RRD_update( char *rrd, char *sum );
static int summary_RRD_create( char *rrd, char *polling_interval);
static int summary_RRD_update( char *rrd, char *sum, char *num );
static void inline my_mkdir ( char *dir );

int
write_data_to_rrd ( char *cluster, char *host, char *metric, char *sum, char *num, char *polling_interval )
{
   char rrd[ 4096 ];
   int rval;
   char *summary_dir = "__SummaryInfo__";

   strcpy( rrd, rrd_rootdir );
   strcat( rrd, "/" );

   if( ! cluster && ! host )
      {
         strcat( rrd, summary_dir );
         my_mkdir( rrd );
         strcat( rrd, "/" );
         strcat( rrd, metric );
         strcat( rrd, ".rrd" ); 
         return push_data_to_summary_rrd( rrd, sum, num, polling_interval ); 
      }
   else if( cluster && ! host )
      {
         strcat( rrd, cluster );
         my_mkdir( rrd );
         strcat( rrd, "/" );
         strcat( rrd, summary_dir );
         my_mkdir( rrd );  
         strcat( rrd, "/" );
         strcat( rrd, metric );
         strcat( rrd, ".rrd" );
         return push_data_to_summary_rrd( rrd, sum, num, polling_interval );
      }
   else
      {
         strcat( rrd, cluster );
         my_mkdir( rrd );
         strcat( rrd, "/" ); 
         strcat( rrd, host );
         my_mkdir( rrd ); 
         strcat( rrd, "/" );
         strcat( rrd, metric );
         strcat( rrd, ".rrd" );
         return push_data_to_rrd( rrd, sum, polling_interval ); 
      }
   /* Shouldn't get here */
   return 1;
}

static void inline
my_mkdir ( char *dir )
{
   if ( mkdir ( dir, 0755 ) < 0 && errno != EEXIST)
      {
         err_sys("Unable to mkdir(%s)",dir);
      }
}

/* A summary RRD has a "num" and a "sum" DS (datasource) whereas the
   host rrds only have "sum" (since num is always 1) */
static int
push_data_to_summary_rrd( char *rrd, char *sum, char *num, char *polling_interval)
{
   int rval;
   struct stat st;
   
   if( stat(rrd, &st) )
      {
         rval = summary_RRD_create( rrd, polling_interval );
         if( rval )
            return rval;
      }
   return summary_RRD_update( rrd, sum, num );
}

static int
push_data_to_rrd( char *rrd, char *sum, char *polling_interval)
{
  int rval;
  struct stat st;

  if( stat(rrd, &st) )
     {
        rval = RRD_create( rrd, polling_interval );
        if( rval )
           return rval;
     }
  return RRD_update( rrd, sum );
}

static int
RRD_update( char *rrd, char *value )
{
   char *argv[3];
   int   argc = 3;
   char val[128];

   sprintf(val, "N:%s", value);

   argv[0] = "dummy";
   argv[1] = rrd;
   argv[2] = val; 

   optind=0; opterr=0;
   rrd_clear_error();
   rrd_update(argc, argv);
   if(rrd_test_error())
      {
         err_msg("RRD_update: %s", rrd_get_error());
         return 1;
      } 
   debug_msg("Updated rrd %s with value %s", rrd, val);
   return 0;
}

static int
summary_RRD_update( char *rrd, char *sum, char *num )
{
   char *argv[3];
   int   argc = 3;
   char val[128];

   sprintf(val, "N:%s:%s", sum, num);

   argv[0] = "dummy";
   argv[1] = rrd;
   argv[2] = val;

   optind=0; opterr=0;
   rrd_clear_error();
   rrd_update(argc, argv);
   if(rrd_test_error())
      {
         err_msg("RRD_update: %s", rrd_get_error());
         return 1;
      }
   debug_msg("Updated rrd %s with value %s", rrd, val);
   return 0;
}


/* Warning: RRD_create will overwrite a RRdb if it already exists */
static int
RRD_create( char *rrd, char *polling_interval)
{
   char *argv[10];
   int  argc = 10;

   argv[0] = "dummy";
   argv[1] = rrd;
   argv[2] = "--step";
   argv[3] = polling_interval;
   argv[4] = "DS:sum:GAUGE:30:U:U";
   argv[5] = "RRA:AVERAGE:0.5:1:240";
   argv[6] = "RRA:AVERAGE:0.5:24:240";
   argv[7] = "RRA:AVERAGE:0.5:168:240";
   argv[8] = "RRA:AVERAGE:0.5:672:240";
   argv[9] = "RRA:AVERAGE:0.5:5760:370";

   optind=0; opterr=0;  
   rrd_clear_error();
   rrd_create(argc, argv);
   if(rrd_test_error())
      {
         err_msg("RRD_create: %s", rrd_get_error());
         return 1;
      }
   debug_msg("Created rrd %s", rrd);
   return 0;
}

static int
summary_RRD_create( char *rrd, char *polling_interval)
{
   char *argv[11];
   int  argc = 11;

   argv[0] = "dummy";
   argv[1] = rrd;
   argv[2] = "--step";
   argv[3] = polling_interval;
   argv[4] = "DS:sum:GAUGE:30:U:U";
   argv[5] = "DS:num:GAUGE:30:U:U";
   argv[6] = "RRA:AVERAGE:0.5:1:240";
   argv[7] = "RRA:AVERAGE:0.5:24:240";
   argv[8] = "RRA:AVERAGE:0.5:168:240";
   argv[9] = "RRA:AVERAGE:0.5:672:240";
   argv[10] = "RRA:AVERAGE:0.5:5760:370";

   optind=0; opterr=0;
   rrd_clear_error();
   rrd_create(argc, argv);
   if(rrd_test_error())
      {
         err_msg("RRD_create: %s", rrd_get_error());
         return 1;
      }
   debug_msg("Created rrd %s", rrd);
   return 0;
}
