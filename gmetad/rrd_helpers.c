/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <rrd.h>

int
RRD_update( char *rrd, char *value )
{
   char *argv[3];
   int   argc = 3;
   char val[128];

   argv[0] = "dummy";
   argv[1] = rrd;
   argv[2] = val; 

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
   return 0;
}

int
summary_RRD_update( char *rrd, char *sum, char *num )
{
   char *argv[3];
   int   argc = 3;
   char val[128];

   argv[0] = "dummy";
   argv[1] = rrd;
   argv[2] = val;

   sprintf(val, "N:%s:%s", sum, num);

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
int
RRD_create( char *rrd, char *polling_interval)
{
   char *argv[10];
   int  argc = 10;

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

int
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
