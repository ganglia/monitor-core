#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/sysinfo.h>

/* From old ganglia 2.5.x... */
#include "file.h"
#include "libmetrics.h"
#include "fsusage.h"
/* End old ganglia 2.5.x headers */

#define OSNAME "Linux"
#define OSNAME_LEN strlen(OSNAME)

/* Never changes */
char proc_cpuinfo[BUFFSIZE];
char proc_sys_kernel_osrelease[BUFFSIZE];

typedef struct {
  uint32_t last_read;
  uint32_t thresh;
  char *name;
  char buffer[BUFFSIZE];
} timely_file;

timely_file proc_stat    = { 0, 15, "/proc/stat" };
timely_file proc_loadavg = { 0, 15, "/proc/loadavg" };
timely_file proc_meminfo = { 0, 30, "/proc/meminfo" };
timely_file proc_net_dev = { 0, 30, "/proc/net/dev" };

char *update_file(timely_file *tf)
{
  int now,rval;
  now = time(0);
  if(now - tf->last_read > tf->thresh) {
    rval = slurpfile(tf->name, tf->buffer, BUFFSIZE);
    if(rval == SYNAPSE_FAILURE) {
      err_msg("update_file() got an error from slurpfile() reading %s",
	      tf->name);
    }
    else tf->last_read = now;
  }
  return tf->buffer;
}

/*
** A helper function to determine the number of cpustates in /proc/stat (MKN)
*/
#define NUM_CPUSTATES_24X 4
#define NUM_CPUSTATES_26X 7
static unsigned int num_cpustates;

unsigned int
num_cpustates_func ( void )
{
   char *p;
   unsigned int i=0;

   proc_stat.last_read=0;
   p = update_file(&proc_stat);
   proc_stat.last_read=0;

/*
** Skip initial "cpu" token
*/
   p = skip_token(p);
   p = skip_whitespace(p);
/*
** Loop over file until next "cpu" token is found.
** i=4 : Linux 2.4.x
** i=7 : Linux 2.6.x
*/
   while (strncmp(p,"cpu",3)) {
     p = skip_token(p);
     p = skip_whitespace(p);
     i++;
     }

   return i;
}
/*
 * This function is called only once by the gmond.  Use to 
 * initialize data structures, etc or just return SYNAPSE_SUCCESS;
 */
g_val_t
metric_init(void)
{
   g_val_t rval;

   num_cpustates = num_cpustates_func();

   rval.int32 = slurpfile("/proc/cpuinfo", proc_cpuinfo, BUFFSIZE);
   if ( rval.int32 == SYNAPSE_FAILURE )
      {
         err_msg("metric_init() got an error from slurpfile() /proc/cpuinfo");
         return rval;
      }  

   rval.int32 = slurpfile( "/proc/sys/kernel/osrelease", 
                       proc_sys_kernel_osrelease, BUFFSIZE);
   if ( rval.int32 == SYNAPSE_FAILURE )
      {
         err_msg("kernel_func() got an error from slurpfile()");
         return rval;
      }  
   
   /* Get rid of pesky \n in osrelease */
   proc_sys_kernel_osrelease[rval.int32-1] = '\0';

   rval.int32 = (int) update_file(&proc_net_dev);
   if ( rval.int32 == SYNAPSE_FAILURE )
      {
         err_msg("net_dev_func() got an error from slurpfile()");
         return rval;
      } 

   rval.int32 = SYNAPSE_SUCCESS;
   return rval;
}

g_val_t
pkts_in_func ( void )
{
   char *p;
   register int i;
   static g_val_t val;
   int size;
   static int stamp;
   static double        last_bytes_in,
                last_bytes_out,
                last_pkts_in,
                last_pkts_out;
   double bytes_in=0, bytes_out=0, pkts_in=0, pkts_out=0, t = 0;
   double diff;
   p = update_file(&proc_net_dev);
   if (proc_net_dev.last_read != stamp) {

     size = ( index (p, 0x00) ) - p;
   /*  skip past the two-line header ... */
     p = index (p, '\n')+1;
     p = index (p, '\n')+1;
     p = index (p, '\n')+1;  // and skip loopback, which is always the first one
     while (*p != 0x00 )
       {
       p = index(p, ':')+1; /*  skip past the interface tag portion of this line */
       if ( (*p-1 != 'o') && (*p-2 != 'l') )
          {
          t = strtod( p, &p );
          bytes_in += t;
          t = strtod( p, &p );
          pkts_in += t;
          for (i = 0; i < 6; i++) strtol(p, &p, 10);
          t = strtod( p, &p );
          bytes_out += t;
         pkts_out += strtod( p, &p );
          }
          p = index (p, '\n') + 1;    // skips a line
       }

       diff = pkts_in - last_pkts_in;
       val.f = 0.;
       if ( diff >= 1. )
         {
         t = proc_net_dev.last_read - stamp;
         val.f = diff / t;
         }

       last_bytes_in  = bytes_in;
       last_pkts_in = pkts_in;
       last_pkts_out = pkts_out;
       last_bytes_out = bytes_out;

       stamp = proc_net_dev.last_read;

     }
   debug_msg(" **********  PKTS_IN RETURN:  %f", val.f);
   return val;
}

g_val_t
pkts_out_func ( void )
{
   char *p;
   register int i;
   static g_val_t val;
   int size;
   static int stamp;
   static double        last_bytes_in,
                last_bytes_out,
                last_pkts_in,
                last_pkts_out;
   double bytes_in=0, bytes_out=0, pkts_in=0, pkts_out=0, t = 0;
   double diff;
   p = update_file(&proc_net_dev);
   if (proc_net_dev.last_read != stamp) {

     size = ( index (p, 0x00) ) - p;
   /*  skip past the two-line header ... */
     p = index (p, '\n')+1;
     p = index (p, '\n')+1;
     p = index (p, '\n')+1;  // and skip loopback, which is always the first one
     while (*p != 0x00 )
       {
       p = index(p, ':')+1; /*  skip past the interface tag portion of this line */
       if ( (*p-1 != 'o') && (*p-2 != 'l') )
          {
          t = strtod( p, &p );
          bytes_in += t;
          t = strtod( p, &p );
          pkts_in += t;
          for (i = 0; i < 6; i++) strtol(p, &p, 10);
          t = strtod( p, &p );
          bytes_out += t;
         pkts_out += strtod( p, &p );
          }
          p = index (p, '\n') + 1;    // skips a line
       }

       diff = pkts_out - last_pkts_out;
       val.f = 0.;
       if ( diff >= 1. )
         {
         t = proc_net_dev.last_read - stamp;
         val.f = diff / t;
         }

       last_bytes_in  = bytes_in;
       last_pkts_in = pkts_in;
       last_pkts_out = pkts_out;
       last_bytes_out = bytes_out;

       stamp = proc_net_dev.last_read;

     }
   debug_msg(" **********  PKTS_OUT RETURN:  %f", val.f);
   return val;
}

g_val_t
bytes_out_func ( void )
{
   char *p;
   register int i;
   static g_val_t val;
   int size;
   static int stamp;
   static double        last_bytes_in,
                last_bytes_out,
                last_pkts_in,
                last_pkts_out;
   double bytes_in=0, bytes_out=0, pkts_in=0, pkts_out=0, t = 0;
   double diff;
   p = update_file(&proc_net_dev);
   if (proc_net_dev.last_read != stamp) {

     size = ( index (p, 0x00) ) - p;
   /*  skip past the two-line header ... */
     p = index (p, '\n')+1;
     p = index (p, '\n')+1;
     p = index (p, '\n')+1;  // and skip loopback, which is always the first one
     while (*p != 0x00 )
       {
       p = index(p, ':')+1; /*  skip past the interface tag portion of this line */
       if ( (*p-1 != 'o') && (*p-2 != 'l') )
          {
          t = strtod( p, &p );
          bytes_in += t;
          t = strtod( p, &p );
          pkts_in += t;
          for (i = 0; i < 6; i++) strtol(p, &p, 10);
          /* Fixed 2003 by Dr Michael Wirtz <m.wirtz@tinnit.de>
            and Phil Radden <P.Radden@rl.ac.uk> */
          t = strtod( p, &p );
          bytes_out += t;
         pkts_out += strtod( p, &p );
          }
          p = index (p, '\n') + 1;    // skips a line
       }

       diff = bytes_out - last_bytes_out;
       val.f = 0.;
       if ( diff >= 1. )
         {
         t = proc_net_dev.last_read - stamp;
         val.f = diff / t;
         }

       last_bytes_in  = bytes_in;
       last_pkts_in = pkts_in;
       last_pkts_out = pkts_out;
       last_bytes_out = bytes_out;

       stamp = proc_net_dev.last_read;

     }
   debug_msg(" **********  BYTES_OUT RETURN:  %f", val.f);
   return val;
}

g_val_t
bytes_in_func ( void )
{
   char *p;
   register int i;
   static g_val_t val;
   int size;
   static int stamp;
   static double last_bytes_in,
     last_bytes_out,
     last_pkts_in,
     last_pkts_out;
   double bytes_in=0, bytes_out=0, pkts_in=0, pkts_out=0, t = 0;
   double diff;
   p = update_file(&proc_net_dev);
   if (proc_net_dev.last_read != stamp) {

     size = ( index (p, 0x00) ) - p;
   /*  skip past the two-line header ... */
     p = index (p, '\n')+1;
     p = index (p, '\n')+1;
     p = index (p, '\n')+1;  // and skip loopback, which is always the first one
     while (*p != 0x00 )
       {
       p = index(p, ':')+1; /*  skip past the interface tag portion of this line */
       debug_msg(" Last two chars: %c%c\n", *p-2, *p-1 );
       if ( (*p-1 != 'o') && (*p-2 != 'l') )
          {
          debug_msg(" Last two chars: %c%c\n", *p-2, *p-1 );
          t = strtod( p, &p );
          bytes_in += t;
          t = strtod( p, &p );
          pkts_in += t;
          for (i = 0; i < 6; i++) strtol(p, &p, 10);
          /* Fixed 2003 by Dr Michael Wirtz <m.wirtz@tinnit.de>
            and Phil Radden <P.Radden@rl.ac.uk>. */
          t = strtod( p, &p );
          bytes_out += t;
          pkts_out += strtod( p, &p );
          }
          p = index (p, '\n') + 1;    // skips a line
       }

       diff = bytes_in - last_bytes_in;
       val.f = 0.;
       if ( diff >= 1. )
         {
         t = proc_net_dev.last_read - stamp;
         val.f = diff / t;
         }

       last_bytes_in  = bytes_in;
       last_pkts_in = pkts_in;
       last_pkts_out = pkts_out;
       last_bytes_out = bytes_out;

       stamp = proc_net_dev.last_read;

     }
   debug_msg(" **********  BYTES_IN RETURN:  %f", val.f);
   return val;
}

g_val_t
cpu_num_func ( void )
{
   static int cpu_num = 0;
   g_val_t val;

   /* Only need to do this once */
   if (! cpu_num) {
      /* We'll use _SC_NPROCESSORS_ONLN to get operating cpus */
      cpu_num = get_nprocs();
   }
   val.uint16 = cpu_num;

   return val;
}

g_val_t
cpu_speed_func ( void )
{
   char *p;
   static g_val_t val = {0};

/* i386, ia64, x86_64 and hppa all report MHz in the same format */
#if defined (__i386__) || defined(__ia64__) || defined(__hppa__) || defined(__x86_64__)
   if (! val.uint32 )
      {
         p = proc_cpuinfo;  
         p = strstr( p, "cpu MHz" );
         if(p) {
           p = strchr( p, ':' );
           p++;
           p = skip_whitespace(p);
           val.uint32 = (uint32_t)strtol( p, (char **)NULL , 10 );
         } else {
           val.uint32 = 0;
         }
      }
#endif
#if defined (__alpha__)
   if (! val.uint32 ) {
         int num;
         p = proc_cpuinfo;
         p = strstr( p, "cycle frequency [Hz]" );
         if(p) {
           p = strchr( p, ':' );
           p++;
           p = skip_whitespace(p);
           sscanf(p, "%d", &num);
           num = num / 1000000;  /* Convert to Mhz */
           val.uint32 = (uint32_t)num;
         } else {
           val.uint32 = 0;
         }
      }
#endif
#if defined (__powerpc__)
   if (! val.uint32 )
      {
         p = proc_cpuinfo;
         p = strstr( p, "clock" );
         if(p) { 
           p = strchr( p, ':' );
           p++;
           p = skip_whitespace(p);
           val.uint32 = (uint32_t)strtol( p, (char **)NULL , 10 );
        } else {
           val.uint32 = 0;
        }
      }
#endif
   return val;
}

g_val_t
mem_total_func ( void )
{
   char *p;
   g_val_t val;

   p = strstr( update_file(&proc_meminfo), "MemTotal:");
   if(p) {
     p = skip_token(p);
     val.uint32 = strtol( p, (char **)NULL, 10 );
   } else {
     val.uint32 = 0;
   }

   return val;
}

g_val_t
swap_total_func ( void )
{
   char *p;
   g_val_t val;
 
   p = strstr( update_file(&proc_meminfo), "SwapTotal:" );
   if(p) {
     p = skip_token(p);
     val.uint32 = strtol( p, (char **)NULL, 10 );  
   } else {
     val.uint32 = 0;
   }

   return val;
}

g_val_t
boottime_func ( void )
{
   char *p;
   g_val_t val;

   p = update_file(&proc_stat); 

   p = strstr ( p, "btime" );
   if(p) { 
     p = skip_token ( p );
     val.uint32 = strtod ( p, (char **)NULL );
   } else {
     val.uint32 = 0;
   }

   return val;
}

g_val_t
sys_clock_func ( void )
{
   g_val_t val;

   val.uint32 = time(NULL);
   return val;
}

g_val_t
machine_type_func ( void )
{
   g_val_t val;
 
#ifdef __i386__
   snprintf(val.str, MAX_G_STRING_SIZE, "x86");
#endif
#ifdef __x86_64__
   snprintf(val.str, MAX_G_STRING_SIZE, "x86_64");
#endif
#ifdef __ia64__
   snprintf(val.str, MAX_G_STRING_SIZE, "ia64");
#endif
#ifdef __sparc__
   snprintf(val.str, MAX_G_STRING_SIZE, "sparc");
#endif
#ifdef __alpha__
   snprintf(val.str, MAX_G_STRING_SIZE, "alpha");
#endif
#ifdef __powerpc__
   snprintf(val.str, MAX_G_STRING_SIZE, "powerpc");
#endif
#ifdef __m68k__
   snprintf(val.str, MAX_G_STRING_SIZE, "m68k");
#endif
#ifdef __mips__
   snprintf(val.str, MAX_G_STRING_SIZE, "mips");
#endif
#ifdef __arm__
   snprintf(val.str, MAX_G_STRING_SIZE, "arm");
#endif
#ifdef __hppa__
   snprintf(val.str, MAX_G_STRING_SIZE, "hppa");
#endif
#ifdef __s390__
   snprintf(val.str, MAX_G_STRING_SIZE, "s390");
#endif

   return val;
}

g_val_t
os_name_func ( void )
{
   g_val_t val;

   snprintf(val.str, MAX_G_STRING_SIZE, "Linux");
   return val;
}

g_val_t
os_release_func ( void )
{
   g_val_t val;

   snprintf(val.str, MAX_G_STRING_SIZE, "%s", proc_sys_kernel_osrelease);
   return val;
}


/*
 * A helper function to return the total number of cpu jiffies
 */
unsigned long
total_jiffies_func ( void )
{
   char *p;
   unsigned long user_jiffies, nice_jiffies, system_jiffies, idle_jiffies,
                 wio_jiffies, irq_jiffies, sirq_jiffies;

   p = update_file(&proc_stat);
   p = skip_token(p);
   p = skip_whitespace(p);
   user_jiffies = strtod( p, &p );
   p = skip_whitespace(p);
   nice_jiffies = strtod( p, &p ); 
   p = skip_whitespace(p);
   system_jiffies = strtod( p , &p ); 
   p = skip_whitespace(p);
   idle_jiffies = strtod( p , &p );

   if(num_cpustates == NUM_CPUSTATES_24X)
     return user_jiffies + nice_jiffies + system_jiffies + idle_jiffies;

   p = skip_whitespace(p);
   wio_jiffies = strtod( p , &p );
   p = skip_whitespace(p);
   irq_jiffies = strtod( p , &p );
   p = skip_whitespace(p);
   sirq_jiffies = strtod( p , &p );
  
   return user_jiffies + nice_jiffies + system_jiffies + idle_jiffies +
          wio_jiffies + irq_jiffies + sirq_jiffies; 
}   


g_val_t
cpu_user_func ( void )
{
   char *p;
   static g_val_t val;
   static int stamp;
   static double last_user_jiffies,  user_jiffies, 
                 last_total_jiffies, total_jiffies, diff;
   
   p = update_file(&proc_stat);
   if(proc_stat.last_read != stamp) {
     stamp = proc_stat.last_read;
     
     p = skip_token(p);
     user_jiffies  = strtod( p , (char **)NULL );
     total_jiffies = total_jiffies_func();
     
     diff = user_jiffies - last_user_jiffies; 
     
     if ( diff )
       val.f = (diff/(total_jiffies - last_total_jiffies))*100;
     else
       val.f = 0.0;
     
     last_user_jiffies  = user_jiffies;
     last_total_jiffies = total_jiffies;
     
   }
   return val;
}

g_val_t
cpu_nice_func ( void )
{
   char *p;
   static g_val_t val;
   static int stamp;
   static double last_nice_jiffies,  nice_jiffies,
                 last_total_jiffies, total_jiffies, diff;
 
   p = update_file(&proc_stat);
   if(proc_stat.last_read != stamp) {
     stamp = proc_stat.last_read;
 
     p = skip_token(p);
     p = skip_token(p);
     nice_jiffies  = strtod( p , (char **)NULL );
     total_jiffies = total_jiffies_func();

     diff = (nice_jiffies  - last_nice_jiffies);
 
     if ( diff )
       val.f = (diff/(total_jiffies - last_total_jiffies))*100;
     else
       val.f = 0.0;
 
     last_nice_jiffies  = nice_jiffies;
     last_total_jiffies = total_jiffies;

   }
   return val;
}

g_val_t 
cpu_system_func ( void )
{
   char *p;
   static g_val_t val;
   static int stamp;
   static double last_system_jiffies,  system_jiffies,
                 last_total_jiffies, total_jiffies, diff;
 
   p = update_file(&proc_stat);
   if(proc_stat.last_read != stamp) {
     stamp = proc_stat.last_read;
     
     p = skip_token(p);
     p = skip_token(p);
     p = skip_token(p);
     system_jiffies = strtod( p , (char **)NULL );
     if (num_cpustates > NUM_CPUSTATES_24X) {
       p = skip_token(p);
       p = skip_token(p);
       p = skip_token(p);
       system_jiffies += strtod( p , (char **)NULL ); /* "intr" counted in system */
       p = skip_token(p);
       system_jiffies += strtod( p , (char **)NULL ); /* "sintr" counted in system */
       }
     total_jiffies  = total_jiffies_func();

     diff = system_jiffies  - last_system_jiffies;
 
     if ( diff )
       val.f = (diff/(total_jiffies - last_total_jiffies))*100;
     else
       val.f = 0.0;
 
     last_system_jiffies  = system_jiffies;
     last_total_jiffies = total_jiffies;   

   }
   return val;
}

g_val_t 
cpu_idle_func ( void )
{
   char *p;
   static g_val_t val;
   static int stamp;
   static double last_idle_jiffies,  idle_jiffies,
                 last_total_jiffies, total_jiffies, diff;
 
   p = update_file(&proc_stat);
   if(proc_stat.last_read != stamp) {
     stamp = proc_stat.last_read;
     
     p = skip_token(p);
     p = skip_token(p);
     p = skip_token(p);
     p = skip_token(p);
     idle_jiffies  = strtod( p , (char **)NULL );
     total_jiffies = total_jiffies_func();
     
     diff = idle_jiffies - last_idle_jiffies;
     
     if ( diff ) 
       val.f = (diff/(total_jiffies - last_total_jiffies))*100;
     else
       val.f = 0.0;
     
     last_idle_jiffies  = idle_jiffies;
     last_total_jiffies = total_jiffies;
     
   }
   
   return val;
}

g_val_t 
cpu_aidle_func ( void )
{
   char *p;
   g_val_t val;
   double idle_jiffies, total_jiffies;
   
   p = update_file(&proc_stat);

   p = skip_token(p);
   p = skip_token(p);
   p = skip_token(p);
   p = skip_token(p);
   idle_jiffies  = strtod( p , (char **)NULL );
   total_jiffies = total_jiffies_func();
   
   val.f = (idle_jiffies/total_jiffies)*100;
   return val;
}

g_val_t 
cpu_wio_func ( void )
{
   char *p;
   static g_val_t val;
   static int stamp;
   static double last_wio_jiffies,  wio_jiffies,
                 last_total_jiffies, total_jiffies, diff;
 
   if (num_cpustates == NUM_CPUSTATES_24X) {
     val.f = 0.;
     return val;
     }

   p = update_file(&proc_stat);
   if(proc_stat.last_read != stamp) {
     stamp = proc_stat.last_read;
     
     p = skip_token(p);
     p = skip_token(p);
     p = skip_token(p);
     p = skip_token(p);
     p = skip_token(p);
     wio_jiffies  = strtod( p , (char **)NULL );
     total_jiffies = total_jiffies_func();
     
     diff = wio_jiffies - last_wio_jiffies;
     
     if ( diff ) 
       val.f = (diff/(total_jiffies - last_total_jiffies))*100;
     else
       val.f = 0.0;
     
     last_wio_jiffies  = wio_jiffies;
     last_total_jiffies = total_jiffies;
     
   }
   
   return val;
}

g_val_t 
cpu_intr_func ( void )
{
   char *p;
   static g_val_t val;
   static int stamp;
   static double last_intr_jiffies,  intr_jiffies,
                 last_total_jiffies, total_jiffies, diff;
 
   if (num_cpustates == NUM_CPUSTATES_24X) {
     val.f = 0.;
     return val;
     }

   p = update_file(&proc_stat);
   if(proc_stat.last_read != stamp) {
     stamp = proc_stat.last_read;
     
     p = skip_token(p);
     p = skip_token(p);
     p = skip_token(p);
     p = skip_token(p);
     p = skip_token(p);
     p = skip_token(p);
     intr_jiffies  = strtod( p , (char **)NULL );
     total_jiffies = total_jiffies_func();
     
     diff = intr_jiffies - last_intr_jiffies;
     
     if ( diff ) 
       val.f = (diff/(total_jiffies - last_total_jiffies))*100;
     else
       val.f = 0.0;
     
     last_intr_jiffies  = intr_jiffies;
     last_total_jiffies = total_jiffies;
     
   }
   
   return val;
}

g_val_t 
cpu_sintr_func ( void )
{
   char *p;
   static g_val_t val;
   static int stamp;
   static double last_sintr_jiffies,  sintr_jiffies,
                 last_total_jiffies, total_jiffies, diff;
 
   if (num_cpustates == NUM_CPUSTATES_24X) {
     val.f = 0.;
     return val;
     }

   p = update_file(&proc_stat);
   if(proc_stat.last_read != stamp) {
     stamp = proc_stat.last_read;
     
     p = skip_token(p);
     p = skip_token(p);
     p = skip_token(p);
     p = skip_token(p);
     p = skip_token(p);
     p = skip_token(p);
     p = skip_token(p);
     sintr_jiffies  = strtod( p , (char **)NULL );
     total_jiffies = total_jiffies_func();
     
     diff = sintr_jiffies - last_sintr_jiffies;
     
     if ( diff ) 
       val.f = (diff/(total_jiffies - last_total_jiffies))*100;
     else
       val.f = 0.0;
     
     last_sintr_jiffies  = sintr_jiffies;
     last_total_jiffies = total_jiffies;
     
   }
   
   return val;
}

g_val_t
load_one_func ( void )
{
   g_val_t val;

   val.f = strtod( update_file(&proc_loadavg), (char **)NULL);

   return val;
}

g_val_t
load_five_func ( void )
{
   char *p;
   g_val_t val;

   p = update_file(&proc_loadavg);
   p = skip_token(p);
   val.f = strtod( p, (char **)NULL);
 
   return val;
}

g_val_t
load_fifteen_func ( void )
{
   char *p;
   g_val_t val;

   p = update_file(&proc_loadavg);
 
   p = skip_token(p);
   p = skip_token(p);
   val.f = strtod( p, (char **)NULL);

   return val;
}

g_val_t
proc_run_func( void )
{
   char *p;
   g_val_t val;

   p = update_file(&proc_loadavg);
   p = skip_token(p);
   p = skip_token(p);
   p = skip_token(p);
   val.uint32 = strtol( p, (char **)NULL, 10 );

   val.uint32--;
   /* This shouldn't happen.. but it might */
   if (val.uint32 <0)
      val.uint32 = 0;

   return val;
}

g_val_t
proc_total_func ( void )
{
   char *p;
   g_val_t val;

   p = update_file(&proc_loadavg);
   p = skip_token(p);
   p = skip_token(p);
   p = skip_token(p); 
   p = skip_whitespace(p);
   while ( isdigit(*p) )
      p++;
   p++;  /* skip the slash-/ */ 
   val.uint32 = strtol( p, (char **)NULL, 10 ); 

   return val;
}

g_val_t
mem_free_func ( void )
{
   char *p;
   g_val_t val;

   p = strstr( update_file(&proc_meminfo), "MemFree:" );
   if(p) {
     p = skip_token(p);
     val.uint32 = strtol( p, (char **)NULL, 10 );
   } else {
     val.uint32 = 0;
   }

   return val;
}

g_val_t
mem_shared_func ( void )
{
   char *p;
   g_val_t val;

   p = strstr( update_file(&proc_meminfo), "MemShared:" );
   if (p) {
      p = skip_token(p);
      val.uint32 = strtol( p, (char **)NULL, 10 );
   } else {
      val.uint32 = 0;
   }

   return val;
}

g_val_t
mem_buffers_func ( void )
{
   char *p;
   g_val_t val;

   p = strstr( update_file(&proc_meminfo), "Buffers:" );
   if(p) {
     p = skip_token(p);
     val.uint32 = strtol( p, (char **)NULL, 10 ); 
   } else {
     val.uint32 = 0;
   }

   return val;
}

g_val_t
mem_cached_func ( void )
{
   char *p;
   g_val_t val;

   p = strstr( update_file(&proc_meminfo), "Cached:");
   if(p) {
     p = skip_token(p);
     val.uint32 = strtol( p, (char **)NULL, 10 );
   } else {
     val.uint32 = 0;
   }

   return val;
}

g_val_t
swap_free_func ( void )
{
   char *p;
   g_val_t val;

   p = strstr( update_file(&proc_meminfo), "SwapFree:" );
   if(p) {
     p = skip_token(p);
     val.uint32 = strtol( p, (char **)NULL, 10 ); 
   } else {
     val.uint32 = 0;
   }

   return val;
}

/* --------------------------------------------------------------------------- */
g_val_t
mtu_func ( void )
{
   /* We want to find the minimum MTU (Max packet size) over all UP interfaces. */
   g_val_t val;

   val.uint32 = get_min_mtu();

   /* A val of 0 means there are no UP interfaces. Shouldn't happen. */
   return val;
}

/* ===================================================== */
/* Disk Usage. Using fsusage.c from the GNU fileutils-4.1.5 package. */
#include <inttypes.h>

/* Linux Specific, but we are in the Linux machine file. */
#define MOUNTS "/proc/mounts"

struct nlist {
   struct nlist *next;
   char *name;
};

#define DFHASHSIZE 101
static struct nlist *DFhashvector[DFHASHSIZE];

/* --------------------------------------------------------------------------- */
unsigned int DFhash(const char *s)
{
   unsigned int hashval;
   for (hashval=0; *s != '\0'; s++)
      hashval = *s + 31 * hashval;
   return hashval % DFHASHSIZE;
}

/* --------------------------------------------------------------------------- */
/* From K&R C book, pp. 144-145 */
struct nlist * seen_before(const char *name)
{
   struct nlist *found=0, *np;
   unsigned int hashval;

   /* lookup */
   hashval=DFhash(name);
   for (np=DFhashvector[hashval]; np; np=np->next) {
      if (!strcmp(name,np->name)) {
         found=np;
         break;
      }
   }
   if (!found) {    /* not found */
      np = (struct nlist *) malloc(sizeof(*np));
      if (!np || !(np->name = (char *) strdup(name)))
         return NULL;
      np->next = DFhashvector[hashval];
      DFhashvector[hashval] = np;
      return NULL;
   }
   else /* found name */
      return found;
}

/* --------------------------------------------------------------------------- */
void DFcleanup()
{
   struct nlist *np, *next;
   int i;
   for (i=0; i<DFHASHSIZE; i++) {
      /* Non-standard for loop. Note the last clause happens at the end of the loop. */
      for (np = DFhashvector[i]; np; np=next) {
         next=np->next;
         free(np->name);
         free(np);
      }
      DFhashvector[i] = 0;
   }
}

/* --------------------------------------------------------------------------- */
int remote_mount(const char *device, const char *type)
{
   /* From ME_REMOTE macro in mountlist.h:
   A file system is `remote' if its Fs_name contains a `:'
   or if (it is of type smbfs and its Fs_name starts with `//'). */
   return ((strchr(device,':') != 0)
      || (!strcmp(type, "smbfs") && device[0]=='/' && device[1]=='/')
      || (!strcmp(type, "autofs")));
}

/* --------------------------------------------------------------------------- */
float device_space(char *mount, char *device, double *total_size, double *total_free)
{
   struct fs_usage fsu;
   uint32_t blocksize;
   uint32_t free;
   uint32_t size;
   /* The percent used: used/total * 100 */
   float pct=0.0;

   /* Avoid multiply-mounted disks - not done in df. */
   if (seen_before(device)) return pct;

   if (get_fs_usage(mount, device, &fsu)) {
      /* Ignore funky devices... */
      return pct;
   }

   free = fsu.fsu_bavail;
   /* Is the space available negative? */
   if (fsu.fsu_bavail_top_bit_set) free = 0;
   size = fsu.fsu_blocks;
   blocksize = fsu.fsu_blocksize;
   /* Keep running sum of total used, free local disk space. */
   *total_size += size * (double) blocksize;
   *total_free += free * (double) blocksize;
   /* The percentage of space used on this partition. */
   pct = size ? ((size - free) / (float) size) * 100 : 0.0;
   return pct;
}

/* --------------------------------------------------------------------------- */
float find_disk_space(double *total_size, double *total_free)
{
   FILE *mounts;
   char procline[256];
   char mount[128], device[128], type[32];
   /* We report in GB = 1e9 bytes. */
   double reported_units = 1e9;
   /* Track the most full disk partition, report with a percentage. */
   float thispct, max=0.0;
   int rc;

   /* Read all currently mounted filesystems. */
   mounts=fopen(MOUNTS,"r");
   if (!mounts) {
      debug_msg("Df Error: could not open mounts file %s. Are we on the right OS?\n", MOUNTS);
      return max;
   }
   while ( fgets(procline, sizeof(procline), mounts) ) {
      rc=sscanf(procline, "%s %s %s ", device, mount, type);
      if (!rc) continue;
      if (remote_mount(device, type)) continue;
      if (strncmp(device, "/dev/", 5)) continue;
      thispct = device_space(mount, device, total_size, total_free);
      debug_msg("Counting device %s (%.2f GB)", device, thispct);
      if (!max || max<thispct)
         max = thispct;
   }
   fclose(mounts);

   *total_size = *total_size / reported_units;
   *total_free = *total_free / reported_units;
   /* debug_msg("For all disks: %.3f GB total, %.3f GB free for users.", *total_size, *total_free); */

   DFcleanup();
   return max;
}

/* --------------------------------------------------------------------------- */
g_val_t
disk_free_func( void )
{
   double total_free=0.0;
   double total_size=0.0;
   g_val_t val;

   find_disk_space(&total_size, &total_free);

   val.d = total_free;
   return val;
}


/* --------------------------------------------------------------------------- */
g_val_t
disk_total_func( void )
{
   double total_free=0.0;
   double total_size=0.0;
   g_val_t val;

   find_disk_space(&total_size, &total_free);

   val.d = total_size;
   return val;
}

/* --------------------------------------------------------------------------- */
g_val_t
part_max_used_func( void )
{
   double total_free=0.0;
   double total_size=0.0;
   float most_full;
   g_val_t val;

   most_full = find_disk_space(&total_size, &total_free);

   val.f = most_full;
   return val;
}

/* ---------------------------------- */
/*
 * Stub for doing stand-alone testing
 *
 * Compile with:
 *
 *
gcc -g -O2 -D_REENTRANT -Wall -I.. -I../lib -I../lib/dnet -DHAVE_CONFIG_H \
   -DSTAND_ALONE_TEST  -o gmo.tst machine.c ../lib/.libs/libganglia.a \
   ../lib/libgetopthelper.a -ldl -lresolv -lnsl -lpthread
 *
 */
#ifdef STAND_ALONE_TEST
int main()
{
g_val_t tval;

metric_init();
printf("num_cpustates = %d\n",num_cpustates);

tval = proc_total_func();
printf("Tot proc: %d\n",tval.uint32);

cpu_user_func();
cpu_nice_func();
cpu_system_func();
cpu_idle_func();
cpu_wio_func();
cpu_intr_func();
cpu_sintr_func();


sleep(5);

tval = cpu_user_func();
printf("User pct.: %f\n",tval.f);
tval = cpu_nice_func();
printf("Nice pct.: %f\n",tval.f);
tval = cpu_system_func();
printf("Sys  pct.: %f\n",tval.f);
tval = cpu_idle_func();
printf("Idle pct.: %f\n",tval.f);
tval = cpu_wio_func();
printf("Wio  pct.: %f\n",tval.f);
tval = cpu_intr_func();
printf("Intr pct.: %f\n",tval.f);
tval = cpu_sintr_func();
printf("Sntr pct.: %f\n",tval.f);

return 0;
}
#endif

