/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <rrd.h>

extern char * rrd_rootdir;

#define MAX_RRD_FILENAME_LEN  2024
#define MAKE_RRD_FILENAME(rrd, cluster, host, metric) \ 
   snprintf(rrd, MAX_RRD_FILENAME_LEN,"/tmp/%s_%s_%s.rrd", cluster, host, metric);

int
RRD_update( char *cluster, char *host, char *metric, char *value )
{
   char *argv[3];
   int   argc = 3;
   char rrd[MAX_RRD_FILENAME_LEN];
   char val[128];

   argv[0] = "dummy";
   argv[1] = rrd;
   argv[2] = val; 

   MAKE_RRD_FILENAME(rrd, cluster, host, metric);
   sprintf(val, "N:%s", value); 
  
   optind=0; opterr=0;
   rrd_clear_error();
   rrd_update(argc, argv);
   if(rrd_test_error())
      {
         err_msg("RRD_update: %s", rrd_get_error());
         return 1;
      } 

   debug_msg("Updated rrd %s with value %s", rrd, val);

   /*
   RRDs::update ("$rrd", "N:$val");
   */
   return 0;
}

/* Warning: RRD_create will overwrite a RRdb if it already exists */
int
RRD_create( char *cluster, char *host, char *metric, char *polling_interval)
{
   char *argv[10];
   int  argc = 10;
   char rrd[MAX_RRD_FILENAME_LEN];

   argv[0] = "dummy";
   argv[1] = rrd;
   argv[2] = "--step";
   argv[3] = polling_interval;
   argv[4] = "DS:ganglia:GAUGE:30:U:U";
   argv[5] = "RRA:AVERAGE:0.5:1:240";
   argv[6] = "RRA:AVERAGE:0.5:24:240";
   argv[7] = "RRA:AVERAGE:0.5:168:240";
   argv[8] = "RRA:AVERAGE:0.5:672:240";
   argv[9] = "RRA:AVERAGE:0.5:5760:370";

   MAKE_RRD_FILENAME(rrd, cluster, host, metric);
 
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
