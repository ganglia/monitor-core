/*  "osf.c" - written for HP/Compaq/Digital Tru64/DigitalUNIX/OSF
 *  all development done on a Tru64 V5.1 box.  YMMV.
 *  although the data-collecting functions were assembled here by
 *  Steven Wagner, some of the algo's were ripped from an old
 *  OSF-compatible top machine-specific C file.  these bits are
 *  marked wherever possible.
 *  
 *  the following code belongs to no one and i nor my employer
 *  take no responsibility for any damage incurred as a direct or
 *  indirect result of running this code, reading this code, or
 *  even thinking about this code.  for all i know it won't even
 *  work.
 */
#include "interface.h"
#include "libmetrics.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/swap.h>
#include <sys/types.h>

#include <sys/utsname.h>

#include <stdio.h>
#include <sys/sysinfo.h>
#include <machine/hal_sysinfo.h>

// #include <mach.h>
#include <sys/vm.h>

#include <sys/table.h>
#include <sys/time.h>

#include <unistd.h>

#define CPUSTATES 3

int multiplier;
static struct utsname unamedata;

struct g_metrics_struct {
    g_val_t             boottime;
    g_val_t             cpu_wio;    // wio is not used on OSF
    g_val_t             cpu_idle;
    g_val_t             cpu_nice;
    g_val_t             cpu_system;
    g_val_t             cpu_user;
    g_val_t             cpu_num;
    g_val_t             cpu_speed;
    g_val_t             load_one;
    g_val_t             load_five;
    g_val_t             load_fifteen;
    char *              machine_type;
    g_val_t             mem_buffers;
    g_val_t             mem_cached;
    g_val_t             mem_free;
    g_val_t             mem_shared;
    g_val_t             mem_total;
    char *              os_name;
    char *              os_release;
    g_val_t             proc_run;
    g_val_t             proc_total;
    g_val_t             swap_free;
    g_val_t             swap_total;
    g_val_t             sys_clock;
    g_val_t             bread_sec;
    g_val_t             bwrite_sec;
    g_val_t             lread_sec;
    g_val_t             lwrite_sec;
    g_val_t             phread_sec;
    g_val_t             phwrite_sec;
    g_val_t             rcache;
    g_val_t             wcache;
};

static struct g_metrics_struct metriclist;

/* although i mention this a million times in my comments, everything about
 * the CPU percentages was ripped off from top.  this function was lifted
 * verbatim because it's the algo used to convert CPU cycles to percentages.
 * this is not so much because i am a bad programmer (i am ), but because
 * it's a well-understood and well-recognized way of figuring out these
 * values.  although maybe not the most correct.
 */

long percentages(int cnt, int *out, long *diffs, int total_change)
{
    register int i;
    register long change;
    register long *dp;
    long half_total;

    /* initialization */
    dp = diffs;

    /* avoid divide by zero potential */
    if (total_change == 0)
    {
        total_change = 1;
    }

    /* calculate percentages based on overall change, rounding up */
    half_total = total_change / 2l;
    debug_msg("CPU:percentages - half_total is %d, total_change is %d",
              half_total, total_change);

    for (i = 0; i <= cnt; i++)
    {
        *out++ = (int)((*diffs++ * 1000 + half_total) / total_change);
    }
     
    /* return the total in case the caller wants to use it */
    return(0);
}

void get_cpu_percentages (void)
{
   static struct timeval lasttime = {0, 0};
   struct timeval thistime;
   double timediff;
   int cpu_states[CPUSTATES+1];
   static unsigned long cpu_old[CPUSTATES+1];
   unsigned long cpu_now[CPUSTATES+1];
   unsigned long cpu_diff[CPUSTATES+1];
   double alpha, beta;  // lambda lambda lambda!!!
   static unsigned long last_refresh;
   register int i, j;
   register long cycledelta;

   struct tbl_sysinfo si;

/*  ripped from top by swagner in the hopes of getting
 *  top-like CPU percentages ...
 */
   gettimeofday (&thistime, NULL);

   if (lasttime.tv_sec)
     timediff = ((double) thistime.tv_sec * 1.0e7 +
                ((double) thistime.tv_usec * 10.0)) -
                ((double) lasttime.tv_sec * 1.0e7 +
                ((double) lasttime.tv_usec * 10.0));
   else
     timediff = 1.0e7;

  /*
   * constants for exponential average.  avg = alpha * new + beta * avg
   * The goal is 50% decay in 30 sec.  However if the sample period
   * is greater than 30 sec, there's not a lot we can do.
   */
  if (timediff < 30.0e7)
    {
      alpha = 0.5 * (timediff / 30.0e7);
      beta = 1.0 - alpha;
      debug_msg("Setting alpha to %f and beta to %f because timediff = %d",
                alpha, beta, timediff);
    }                                                                       
  else
    {
      alpha = 0.5;
      beta = 0.5;
    }

    lasttime = thistime;

/*  END SECTION RIPPED BLATANTLY FROM TOP :) */

   if ( table(TBL_SYSINFO, 0,&si,1,sizeof(struct tbl_sysinfo)) != 1)
       perror("osf.c:CPU:get_cpu_percentages");

   debug_msg("CPU: Just ran table().  Got: usr %d, nice %d, sys %d, idle %d, %dhz.", si.si_user, si.si_nice, si.si_sys, si.si_idle, si.si_hz);
   cpu_now[0] = labs(si.si_user);
   cpu_now[1] = labs(si.si_nice);
   cpu_now[2] = labs(si.si_sys);
   cpu_now[3] = labs(si.si_idle);
   debug_msg("CPU:--before-------------------------------------------------------------\n CPU cycles:");
   debug_msg("CPU: now: %d, %d, %d, %d old: %d, %d, %d, %d diffs: %d, %d, %d, %d",           cpu_now[0],cpu_now[1],cpu_now[2],cpu_now[3],
             cpu_old[0],cpu_old[1],cpu_old[2],cpu_old[3],
             cpu_diff[0],cpu_diff[1],cpu_diff[2],cpu_diff[3]);

   if (metriclist.boottime.uint32 == 0)
      metriclist.boottime.uint32 = si.si_boottime;

   cycledelta = 0;
   for (i = 0; i <= CPUSTATES; i++)
       {
       cpu_diff[i] = cpu_now[i] - cpu_old[i];
       if (i == CPUSTATES)
	  {
	  cpu_diff[i] = labs(si.si_idle) - cpu_old[i];
	  cpu_now[i] = labs(si.si_idle);
	  }
       cycledelta += cpu_diff[i];
       debug_msg("CPU:i is %d : new - old = difference, delta  %d - %d = %d,%d",i, cpu_now[i], cpu_old[i], cpu_diff[i], cycledelta);
       cpu_old[i] = cpu_now[i];
       cpu_states[i] = 0;
       }
   percentages(CPUSTATES, cpu_states, cpu_diff, cycledelta);
   // a test, to force the issue (end of array seems not to be getting right idle val)
   cpu_old[3] = labs(si.si_idle);
   debug_msg("CPU:--after--------------------------------------------------------------\n CPU cycles:");
   debug_msg("CPU: later: %d, %d, %d, %d old: %d, %d, %d, %d diffs: %d, %d, %d, %d",         cpu_now[0],cpu_now[1],cpu_now[2],cpu_now[3],
             cpu_old[0],cpu_old[1],cpu_old[2],cpu_old[3],
             cpu_diff[0],cpu_diff[1],cpu_diff[2],cpu_diff[3]);

   debug_msg ("CPU: Are percentages electric? Try user %d%%, nice %d%%, sys %d%%, idle %d%%", cpu_states[0], cpu_states[1], cpu_states[2], cpu_states[3]);
   metriclist.cpu_idle.f = (float) cpu_states[3] / 10;
   metriclist.cpu_user.f = (float) cpu_states[0] / 10;
   metriclist.cpu_system.f = (float)cpu_states[2] / 10;
   metriclist.cpu_nice.f = (float) cpu_states[1] / 10;
}

void
get_mem_stats(void)
{
    struct tbl_vmstats vmstats;
    if(table(TBL_VMSTATS,0,&vmstats,1,sizeof(struct tbl_vmstats))>0) {
	debug_msg("Vmstats:  Free %d Active %d Inactive %d Wire %d",
                  vmstats.free_count, vmstats.active_count,
                  vmstats.inactive_count, vmstats.wire_count);

        metriclist.mem_total.uint32 = (vmstats.free_count +
                                       vmstats.active_count +
                                       vmstats.inactive_count +
                                       vmstats.wire_count) * multiplier;

	metriclist.mem_free.uint32 = metriclist.mem_total.uint32 - \
                                     (vmstats.active_count * multiplier);
    }
    else {
	perror("WARNING:  Cannot open vmstat kernel table.  Memory stats will be inaccurate.");
    }
    debug_msg("Came out with total == %d  ... free = %d",
              metriclist.mem_total.uint32, metriclist.mem_free.uint32);
}

/*
 * This function is called only once by the gmond.  Use to 
 * initialize data structures, etc or just return SYNAPSE_SUCCESS;
 */
g_val_t 
metric_init(void)
{
   g_val_t val;
   int memsize;
   struct tbl_processor cpuinfo;

/* first we get the uname data (hence my including <sys/utsname.h> ) */
   (void) uname( &unamedata );

/*  now we get the size of memory pages on this box ... */
   multiplier = getpagesize() / 1024;

   get_cpu_percentages();
/* these values don't change from tick to tick.  at least, they shouldn't ...
 * also, these strings don't use the ganglia g_val_t metric struct!
 */
   metriclist.os_name = unamedata.sysname;
   metriclist.os_release = unamedata.release;
   metriclist.machine_type = unamedata.machine;
/* here's another value we should only need to get once - total memory.
 * getsysinfo() returns it in kbytes, but we want bytes.
 */
   get_mem_stats();

/* and another.  although this only gets the CPU speed of the first available CPU,
 * this works for me because i am lazy.  also because all the systems i monitor
 * are SMP boxes - no mixed-speed CPU modules.
 * not to say that this couldn't walk through ALL the processor structs and
 * average the results.
 */

   table(TBL_PROCESSOR_INFO,0,&cpuinfo,1,sizeof(struct tbl_processor) );
   metriclist.cpu_speed.uint32 = cpuinfo.mhz;

   val.int32 = SYNAPSE_SUCCESS;

   return val;
}

g_val_t
cpu_num_func ( void )
{
   g_val_t val;

   val.uint16 = sysconf(_SC_NPROCESSORS_ONLN);

   return val;
}

g_val_t
cpu_speed_func ( void )
{
   g_val_t val;
/*   struct cpu_info allcpuinfo;

   getsysinfo(GSI_CPU_INFO,allcpuinfo,sizeof(allcpuinfo),0);
*/
   val.uint16 = metriclist.cpu_speed.uint32;

   return val;
}

g_val_t
mem_total_func ( void )
{
   g_val_t val;
   struct tbl_pmemstats pmbuf;

   val.f = metriclist.mem_total.uint32;

   return val;
}

g_val_t
swap_total_func ( void )
{
   g_val_t val;
   struct tbl_swapinfo swainfo;
   
   table(TBL_SWAPINFO,-1,&swainfo,1,sizeof(struct tbl_swapinfo) );
   val.f = swainfo.size;

   return val;
}

g_val_t
boottime_func ( void )
{
   g_val_t val;

   val.uint32 = metriclist.boottime.uint32;

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
   long size;
   
   strncpy( val.str, metriclist.machine_type, MAX_G_STRING_SIZE );

   return val;
}

g_val_t
os_name_func ( void )
{
   g_val_t val;
   long size;
   
   strncpy( val.str, metriclist.os_name, MAX_G_STRING_SIZE );

   return val;
}        

g_val_t
os_release_func ( void )
{
   g_val_t val;
   long size;
   
   strncpy( val.str, metriclist.os_release, MAX_G_STRING_SIZE );

   return val;
}        

g_val_t
cpu_user_func ( void )
{
   g_val_t val;

   val.f = metriclist.cpu_user.f;

   return val;
}

g_val_t
cpu_nice_func ( void )
{
   g_val_t val;

   val.f = metriclist.cpu_nice.f;

   return val;
}

g_val_t 
cpu_system_func ( void )
{
   g_val_t val;

   val.f = metriclist.cpu_system.f;

   return val;
}

g_val_t 
cpu_idle_func ( void )
{
   g_val_t val;

   get_cpu_percentages();
   val.f = metriclist.cpu_idle.f;

   return val;
}

g_val_t 
cpu_wio_func ( void )
{
   g_val_t val;
   
   val.f = 0.0;

   return val;
}

/*
** FIXME
*/
g_val_t 
cpu_aidle_func ( void )
{
   g_val_t val;
   val.f = 0.0;
   return val;
}

/*
** FIXME
*/
g_val_t 
cpu_intr_func ( void )
{
   g_val_t val;
   val.f = 0.0;
   return val;
}

/*
** FIXME
*/
g_val_t 
cpu_sintr_func ( void )
{
   g_val_t val;
   val.f = 0.0;
   return val;
}

/*
** FIXME
*/
g_val_t 
bytes_in_func ( void )
{
   g_val_t val;
   val.f = 0.0;
   return val;
}

/*
** FIXME
*/
g_val_t 
bytes_out_func ( void )
{
   g_val_t val;
   val.f = 0.0;
   return val;
}

/*
** FIXME
*/
g_val_t 
pkts_in_func ( void )
{
   g_val_t val;
   val.f = 0.0;
   return val;
}

/*
** FIXME
*/
g_val_t 
pkts_out_func ( void )
{
   g_val_t val;
   val.f = 0.0;
   return val;
}

/*
** FIXME
*/
g_val_t 
disk_free_func ( void )
{
   g_val_t val;
   val.d = 0;
   return val;
}

/*
** FIXME
*/
g_val_t 
disk_total_func ( void )
{
   g_val_t val;
   val.d = 0;
   return val;
}

/*
** FIXME
*/
g_val_t 
part_max_used_func ( void )
{
   g_val_t val;
   val.f = 0.0;
   return val;
}

g_val_t
load_one_func ( void )
{
   g_val_t val;
   struct tbl_loadavg labuf;

   if (table(TBL_LOADAVG,0,&labuf,1,sizeof(struct tbl_loadavg))<0) {
        perror("TBL_LOADAVG");
   }
   if (labuf.tl_lscale)   /* scaled */
        {
	    val.f = ((double)labuf.tl_avenrun.l[0] / (double)labuf.tl_lscale);
        }
   else                   /* not scaled */
	val.f = labuf.tl_avenrun.l[0];

   return val;
}

g_val_t
load_five_func ( void )
{
   g_val_t val;
   struct tbl_loadavg labuf;

   if (table(TBL_LOADAVG,0,&labuf,1,sizeof(struct tbl_loadavg))<0) {
        perror("TBL_LOADAVG");
   }
   if (labuf.tl_lscale)   /* scaled */
        {
            val.f = ((double)labuf.tl_avenrun.l[1] / (double)labuf.tl_lscale);
        }
   else                   /* not scaled */
        val.f = labuf.tl_avenrun.l[1];

   return val;
}

g_val_t
load_fifteen_func ( void )
{
   g_val_t val;
   struct tbl_loadavg labuf;

   if (table(TBL_LOADAVG,0,&labuf,1,sizeof(struct tbl_loadavg))<0) {
        perror("TBL_LOADAVG");
   }
   if (labuf.tl_lscale)   /* scaled */
        {
            val.f = ((double)labuf.tl_avenrun.l[2] / (double)labuf.tl_lscale);
        }
   else                   /* not scaled */
        val.f = labuf.tl_avenrun.l[2];

   return val;
}

g_val_t
proc_run_func( void )
{
   g_val_t val;

   val.uint32 = 0;

   return val;
}

g_val_t
proc_total_func ( void )
{
   g_val_t val;

   val.uint32 = 0;

   return val;
}

g_val_t
mem_free_func ( void )
{
   g_val_t val;

   get_mem_stats();
   val.f = metriclist.mem_free.uint32;

   return val;
}

g_val_t
mem_shared_func ( void )
{
   g_val_t val;

   val.f = 0;

   return val;
}

g_val_t
mem_buffers_func ( void )
{
   g_val_t val;

   val.f = ( 10 * multiplier);

   return val;
}

g_val_t
mem_cached_func ( void )
{
   g_val_t val;

   val.f = 0;

   return val;
}

g_val_t
swap_free_func ( void )
{
   g_val_t val;

   struct tbl_swapinfo swainfo;
   table(TBL_SWAPINFO,-1,&swainfo,1,sizeof(swainfo) );
   val.f = swainfo.free;

   return val;
}

g_val_t
mtu_func ( void )
{
   /* We want to find the minimum MTU (Max packet size) over all UP interfaces.
*/
   g_val_t val;
   val.uint32 = get_min_mtu();
   /* A val of 0 means there are no UP interfaces. Shouldn't happen. */
   return val;
}
