#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <windows.h>
#include <iphlpapi.h>
#include <sys/timeb.h>

/* From old ganglia 2.5.x... */
#include "file.h"
#include "libmetrics.h"
/* End old ganglia 2.5.x headers */

#define OSNAME "Cygwin"
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

static time_t
get_netbw(double *in_bytes, double *out_bytes,
    double *in_pkts, double *out_pkts)
{
  PMIB_IFTABLE iftable;
  double bytes_in = 0, bytes_out = 0, pkts_in = 0, pkts_out = 0;
  static DWORD dwSize;
  DWORD ret, dwInterface;
  struct timeb timebuffer;
  PMIB_IFROW ifrow;

  dwSize = sizeof(MIB_IFTABLE);
  
  iftable = (PMIB_IFTABLE) malloc (dwSize);
  while (ret = GetIfTable(iftable, &dwSize, 1) == ERROR_INSUFFICIENT_BUFFER) {
     iftable = (PMIB_IFTABLE) realloc (iftable, dwSize);
  }
 
  if (ret == NO_ERROR) { 

    _ftime ( &timebuffer );

    /* scan the interface table */
    for (dwInterface = 0; dwInterface < (iftable -> dwNumEntries); dwInterface++) {
      ifrow = &(iftable -> table[dwInterface]);
    
    /* exclude loopback */
      if ( (ifrow -> dwType != MIB_IF_TYPE_LOOPBACK ) && (ifrow -> dwOperStatus ==MIB_IF_OPER_STATUS_OPERATIONAL ) ) {
        bytes_in += ifrow -> dwInOctets;
        bytes_out += ifrow -> dwOutOctets;
      
        /* does not include multicast traffic (dw{In,Out}NUcastPkts) */
        pkts_in += ifrow -> dwInUcastPkts;
        pkts_out += ifrow -> dwOutUcastPkts;
      }
    }
    free (iftable);
  } else {
    err_msg("get_netbw() got an error from GetIfTable()");
  }

  if (in_bytes) *in_bytes = bytes_in;
  if (out_bytes) *out_bytes = bytes_out;
  if (in_pkts) *in_pkts = pkts_in;
  if (out_pkts) *out_pkts = pkts_out;

  return timebuffer.time;
}

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

   strcpy( proc_sys_kernel_osrelease, "cygwin" );

   rval.int32 = SYNAPSE_SUCCESS;
   return rval;
}

g_val_t
pkts_in_func ( void )
{
   double in_pkts=0, t=0;
   time_t stamp;
   static time_t last_stamp;
   static double last_pkts_in;
   g_val_t val;
   unsigned long diff;

   stamp = get_netbw(NULL, NULL, &in_pkts, NULL);
   (unsigned long) diff = in_pkts - last_pkts_in;
   if ( diff && last_stamp ) {
     t = stamp - last_stamp;
     t = diff / t;
     debug_msg("Returning value: %f\n", t);     
   } else t = 0;

   val.f = t;
   last_pkts_in = in_pkts;
   last_stamp = stamp;

   return val;
}

g_val_t
pkts_out_func ( void )
{
   double out_pkts=0, t=0;
   time_t stamp;
   static time_t last_stamp;
   static double last_pkts_out;
   g_val_t val;
   unsigned long diff;

   stamp = get_netbw(NULL, NULL, NULL, &out_pkts);
   (unsigned long) diff = out_pkts - last_pkts_out;
   if ( diff && last_stamp ) {
     t = stamp - last_stamp;
     t = diff / t;
     debug_msg("Returning value: %f\n", t);     
   } else t = 0;

   val.f = t;
   last_pkts_out = out_pkts;
   last_stamp = stamp;

   return val;
}

g_val_t
bytes_out_func ( void )
{
   double out_bytes=0, t=0;
   time_t stamp;
   static time_t last_stamp;
   static double last_bytes_out;
   g_val_t val;
   unsigned long diff;

   stamp = get_netbw(NULL, &out_bytes, NULL, NULL);
   (unsigned long) diff = out_bytes - last_bytes_out;
   if ( diff && last_stamp ) {
     t = stamp - last_stamp;
     t = diff / t;
     debug_msg("Returning value: %f\n", t);     
   } else t = 0;

   val.f = t;
   last_bytes_out = out_bytes;
   last_stamp = stamp;

   return val;
}

g_val_t
bytes_in_func ( void )
{
   double in_bytes=0, t=0;
   time_t stamp;
   static time_t last_stamp;
   static double last_bytes_in;
   g_val_t val;
   unsigned long diff;

   stamp = get_netbw(&in_bytes, NULL, NULL, NULL);
   (unsigned long) diff = in_bytes - last_bytes_in;
   if ( diff && last_stamp ) {
     t = stamp - last_stamp;
     t = diff / t;
     debug_msg("Returning value: %f\n", t);     
   } else t = 0;

   val.f = t;
   last_bytes_in = in_bytes;
   last_stamp = stamp;

   return val;
}

g_val_t
cpu_num_func ( void )
{
#if 0
   static int cpu_num = 0;
#endif
   g_val_t val;

#if 0
   /* Only need to do this once */
   if (! cpu_num) {
      /* We'll use _SC_NPROCESSORS_ONLN to get operating cpus */
      cpu_num = get_nprocs();
   }
#endif
   val.uint16 = 0;

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
   snprintf(val.str, MAX_G_STRING_SIZE, "Cygwin");
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
       system_jiffies += strtod( p , (char **)NULL );
       p = skip_token(p);
       system_jiffies += strtod( p , (char **)NULL );
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

   if(val.uint32)
     val.uint32--;/* don't count ourselves? */ 

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

g_val_t
disk_free_func( void )
{
   g_val_t val;
   val.d = 0;
   return val;
}

g_val_t
disk_total_func( void )
{
   g_val_t val;
   val.d = 0;
   return val;
}

/* --------------------------------------------------------------------------- */
g_val_t
part_max_used_func( void )
{
   g_val_t val;
   val.f = 0.0;
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

