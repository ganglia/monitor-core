/*
 * This file is an amended version of solaris.c included in ganglia 2.5.4
 * and ganglia 2.5.5. It has been modified by Adeyemi Adesanya
 * (yemi@slac.stanford.edu) to allow gmond to run as a non-root account. The
 * kvm dependency has been removed and all statistics are now obtained via
 * kstat.
 *
 * It appears to run just fine in Solaris 9 and should be OK in 8 also.
 * Earlier versions of solaris may not provide all the data via kstat.
 * Try running 'kstat cpu_stat' from the command line.
 *
 * Modifications made by Michael Hom:
 *
 * - Stop gmond from coredumping when CPU is "unusual".
 *
 * Modifications made by JB Kim:
 *
 * - Stop gmond from coredumping when CPUs are "offline".
 *
 * Modifications made by Robert Petkus:
 *
 * - Take care of the case that the number of active CPUs is smaller than
 *   number of installed CPUs. That would result in a sparse numering of
 *   CPUs and lead to a core dump with the old algorithm.
 *
 * Modifications made by Martin Knoblauch:
 *
 * - Add proc_run statistics - may need finetuning
 * - Add bytes_in, bytes_out, pkts_in and pkts_out
 * - Fix misallocation of "buffers" (3 needed instead of 2 !!!)
 *   array in determine_cpu_percentages
 * - Port to new get_ifi_info() functionality
 * - Optimize use of kstat_open(). Assuming that the number and
 *   composition of the kstat headers is relatively static compared
 *   to the frequency of metrics calls in the server thread, it is
 *   a lot cheaper to call kstat_open() once [in the context of the
 *   server thread !!!] and then call kstat_chain_update() for each
 *   metrics retrieval. kstat_chain_update() is a noop if the kstat
 *   header chain has not changed between calls.
 * - move cpu_speed and boottime to metric_init
 * - kill get_metric_val. Dead code.
 * - fix potential data corruption when calculating CPU percentages
 *
 * Modifications made by Carlo Marcelo Arenas Belon:
 * - Add disk_total, disk_free, part_max_used
 *
 * Tested on Solaris 7 x86 (32-bit) with gcc-2.8.1
 * Tested on Solaris 8 (64-bit) with gcc-3.3.1
 * Tested on Solaris 9 (64-bit) with gcc-3.4.4
 * Tested on Solaris 10 SPARC (64-bit) and x86 (32-bit and 64-bit)
 */

#include "interface.h"
#include "libmetrics.h"

#include <kstat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <strings.h>
#include <sys/types.h>
#include <dirent.h>
#include <procfs.h>
#include <errno.h>

/*
 * used for swap space determination - swapctl()
 * and anon.h has the data structure  needed for swapctl to be useful
 */

#include <sys/stat.h>
#include <sys/swap.h>
#include <vm/anon.h>
#include <fcntl.h>

/*
 * we get the cpu struct from cpuvar, maybe other mojo too
 */

#include <sys/var.h>
#include <sys/cpuvar.h>
#include <sys/time.h>
#include <sys/processor.h>
/*
 * functions spackled in by swagner -- the CPU-percentage-specific code is
 * largely an imitation (if not a shameless copy) of the Solaris-specific
 * code for top.
 */

/*
 * used for disk space determination - getmntent(), statvfs()
 */

#include <sys/mnttab.h>
#include <sys/statvfs.h>

/*  number of seconds to wait before refreshing/recomputing values off kstat */

#define TICK_SECONDS    30

#ifndef FSCALE
#define FSHIFT  8               /* bits to right of fixed binary point */
#define FSCALE  (1<<FSHIFT)
#endif /* FSCALE */

/*  also imported from top, i'm basically using the same CPU cycle summing algo */

#define CPUSTATES       5
#define CPUSTATE_IDLE   0
#define CPUSTATE_USER   1
#define CPUSTATE_KERNEL 2
#define CPUSTATE_IOWAIT 3
#define CPUSTATE_SWAP   4

int first_run = 1;

/* support macros for the percentage computations */

#define loaddouble(la) ((double)(la) / FSCALE)

static struct utsname unamedata;

struct cpu_info {
    unsigned long bread;
    unsigned long bwrite;
    unsigned long lread;
    unsigned long lwrite;
    unsigned long phread;
    unsigned long phwrite;
};

struct g_metrics_struct {
    g_val_t boottime;
    g_val_t cpu_wio;
    g_val_t cpu_idle;
    g_val_t cpu_aidle;
    g_val_t cpu_nice;
    g_val_t cpu_system;
    g_val_t cpu_user;
    g_val_t cpu_num;
    g_val_t cpu_speed;
    g_val_t load_one;
    g_val_t load_five;
    g_val_t load_fifteen;
     char * machine_type;
    g_val_t mem_buffers;
    g_val_t mem_cached;
    g_val_t mem_free;
    g_val_t mem_shared;
    g_val_t mem_total;
     char * os_name;
     char * os_release;
    g_val_t proc_run;
    g_val_t proc_total;
    g_val_t swap_free;
    g_val_t swap_total;
    g_val_t sys_clock;
    g_val_t bread_sec;
    g_val_t bwrite_sec;
    g_val_t lread_sec;
    g_val_t lwrite_sec;
    g_val_t phread_sec;
    g_val_t phwrite_sec;
    g_val_t rcache;
    g_val_t wcache;
    g_val_t pkts_in;
    g_val_t pkts_out;
    g_val_t bytes_in;
    g_val_t bytes_out;
};

static struct g_metrics_struct metriclist;
static kstat_ctl_t *kc=NULL;
static int g_ncpus;

int
get_kstat_val(g_val_t *val, char *km_name, char *ks_name, char *name)
{
   /* Warning.. always assuming a KSTAT_DATA_ULONG here */
   kstat_t *ks;
   kstat_named_t *kn;

   /*
    * Get a kstat_ctl handle, or update the kstat chain.
    */
   if (kc == NULL)
      kc = kstat_open();
   else
      kstat_chain_update(kc);

   if (kc == NULL)
      {
         debug_msg("couldn't open kc...");
         err_ret("get_kstat_val() kstat_open() error");
         return SYNAPSE_FAILURE;
      }

   debug_msg( "Lookup up kstat:  km (unix?)='%s', ks (system_misc?)='%s',kn (resulting metric?)='%s'", km_name, ks_name, name);
   debug_msg( "%s: kc is %p", name, kc);
   ks = kstat_lookup(kc, km_name, 0, ks_name);
   debug_msg("%s: Just did kstat_lookup().",name);

   /*
    * A hack contributed by Michael Hom <michael_hom_work@yahoo.com>
    * cpu_info0 doesn't always exist on Solaris, as the first CPU
    * need not be in slot 0.
    * Therefore, if ks == NULL after kstat_lookup(), we try
    * to find the first valid instance using the query:
    *   ks = kstat_lookup(kc, km_name, -1, NULL);
    */

   if ((strcmp(km_name, "cpu_info") == 0) && (ks == NULL))  {
      debug_msg( "Lookup up kstat:  km (unix?)='%s', ks (system_misc?)='NULL',kn (resulting metric?)='%s'", km_name, name);
      ks = kstat_lookup(kc, km_name, -1, NULL);
      debug_msg("Just did kstat_lookup() on first instance of module %s.\n",km_name);
   }

   if (ks == NULL)
      {
      perror("ks");
      }
   debug_msg("%s: Looked up.", name);
    if (kstat_read(kc, ks, 0) == -1) {
        perror("kstat_read");
        return SYNAPSE_FAILURE;
    }
   kn = kstat_data_lookup(ks, name);
   if ( kn == NULL )
      {
         err_ret("get_kstat_val() kstat_data_lookup() kstat_read() error");
         return SYNAPSE_FAILURE;
      }
   debug_msg( "%s: Kstat data type:  %d, Value returned: %u, %d %u %d", name, (int)kn->data_type, (int)kn->value.ui32, (int)kn->value.l, kn->value.ul, kn->value.ui64);
//    ks = kstat_lookup(kc, "unix", 0, "system_misc");

   if (kn->value.ui32 == 0)
      val->uint32 = (unsigned long)kn->value.ul;
   else
      val->uint32 = (int)kn->value.ui32;
   sleep(0);
   debug_msg("%s: Kernel close.  Val returned: %d", name, val->uint32);

   return SYNAPSE_SUCCESS;
}

unsigned int
pagetok( int pageval )
{
    unsigned int foo;
    foo = pageval;
    foo = foo * (sysconf(_SC_PAGESIZE) / 1024);
    debug_msg("PageToK():  %u * PAGESIZE (%u) / 1024 (Kb conversion) = %u", pageval, sysconf(_SC_PAGESIZE), foo);
    return foo;
}

/*
 *  there's too much legwork for each function to handle its metric.
 *  hence the updater.
 */

void
determine_swap_space( unsigned int *total, unsigned int *fr )
{
    struct anoninfo anon;
    if (swapctl(SC_AINFO, &anon)  == -1 )  {
       *total = *fr = 0;
       return;
    }
    /*  we are going from swap pages to kilobytes, so the conversion works... */
    *total = pagetok(anon.ani_max);
    *fr = pagetok((anon.ani_max - anon.ani_resv));
    debug_msg("Old/new:  Total = %u/%u , Free = %u/%u", anon.ani_max,*total,(anon.ani_max - anon.ani_resv),*fr);
    return;
}

int
update_metric_data ( void )
{
   debug_msg("running update_metric_data() ... ");
   get_kstat_val(&metriclist.load_fifteen, "unix", "system_misc","avenrun_15min");
   get_kstat_val(&metriclist.load_five,    "unix", "system_misc","avenrun_5min");
   get_kstat_val(&metriclist.load_one,     "unix", "system_misc","avenrun_1min");
   get_kstat_val(&metriclist.proc_total,   "unix", "system_misc", "nproc");
/*
 * memory usage stats are arguably VERY broken.
 */
   get_kstat_val(&metriclist.mem_free,     "unix", "system_pages", "pagesfree");
   get_kstat_val(&metriclist.mem_total,    "unix", "system_pages", "pagestotal");
   debug_msg("Before PageToK():  mem_free = %u, mem_total = %u",metriclist.mem_free.uint32,metriclist.mem_total.uint32);
   metriclist.mem_free.uint32 = pagetok(metriclist.mem_free.uint32);
   metriclist.mem_total.uint32 = pagetok(metriclist.mem_total.uint32);
   determine_swap_space(&metriclist.swap_total.uint32,&metriclist.swap_free.uint32);
//   (void)determine_cpu_percentages();
//   sleep(5);

   /*  update the timestamp.  we use this to determine freshening times as well. */
   metriclist.sys_clock.uint32 = time(NULL);
   return 0;
}

/*
 * another function ripped from top.  after all we want the CPU percentage
 * stuff to match.
 */

long percentages(int cnt, int *out, register unsigned long *new, register unsigned long *old, unsigned long *diffs)
{
    register int i;
    register long change;
    register long total_change;
    register unsigned long *dp;
    long half_total;

    /* initialization */
    total_change = 0;
    dp = diffs;

    /* calculate changes for each state and the overall change */
    for (i = 0; i < cnt; i++)
    {
        if ((change = *new - *old) < 0)
        {
            /* this only happens when the counter wraps */
            change = (int)
                ((unsigned long)*new-(unsigned long)*old);
        }
        total_change += (*dp++ = change);
        *old++ = *new++;
    }

    /* avoid divide by zero potential */
    if (total_change == 0)
    {
        total_change = 1;
    }

    /* calculate percentages based on overall change, rounding up */
    half_total = total_change / 2l;
    for (i = 0; i < cnt; i++)
    {
        *out++ = (int)((*diffs++ * 1000 + half_total) / total_change);
    }

    /* return the total in case the caller wants to use it */
    return(total_change);
}

/*
 * the process for figuring out CPU usage is a little involved, so it has
 * been folded into this function.  also because this way it's easier to
 * rip off top.  :)
 */

int
determine_cpu_percentages ( void )
{
/*
 * hopefully this doesn't get too confusing.
 * cpu_snap is a structure from <sys/cpuvar.h> and is the container into which
 * we read the current CPU metrics.
 * the static array "cpu_old" contains the last iteration's summed cycle
 * counts.
 * the array "cpu_now" contains the current iteration's summed cycle
 * counts.
 * "cpu_diff" holds the delta.
 * across CPUs of course. :)
 * buffers[0..2] holds past, present and diff info for the "other" CPU stats.
 */

   static struct cpu_info buffers[3];
   static struct timeval lasttime = {0, 0};
   struct timeval thistime;
   double timediff;
   int cpu_states[CPUSTATES];
   unsigned int ncpus;
   static unsigned long cpu_old[CPUSTATES];
   static unsigned long cpu_now[CPUSTATES];
   static unsigned long cpu_diff[CPUSTATES];
   unsigned long diff_cycles = 0L;
   unsigned long time_delta = 0L;
   double alpha, beta;  // lambda lambda lambda!!!
   static unsigned long last_refresh;
   register int j;
   char* ks_name;
   kstat_t *ks;
   char* km_name = "cpu_stat";
   cpu_stat_t cpuKstats;
   int ki;
   processorid_t i;
   int cpu_id = sysconf(_SC_NPROCESSORS_ONLN);

/*
 * ripped from top by swagner in the hopes of getting
 * top-like CPU percentages ...
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
      debug_msg("* * * * Setting alpha to %f and beta to %f because timediff = %d",alpha,beta,timediff);
    }
  else
    {
      alpha = 0.5;
      beta = 0.5;
    }

    lasttime = thistime;

/*  END SECTION RIPPED BLATANTLY FROM TOP :) */

   ncpus = metriclist.cpu_num.uint32;

   for (j = 0; j < CPUSTATES; j++)
      cpu_now[j] = 0;

// will C let me do this?  :)

   buffers[0].bread = 0L;
   buffers[0].bwrite = 0L;
   buffers[0].lread = 0L;
   buffers[0].lwrite = 0L;
   buffers[0].phread = 0L;
   buffers[0].phwrite = 0L;

   if (first_run == 1)
      {
   debug_msg("Initializing old read/write buffer... ");
   buffers[1].bread = 0L;
   buffers[1].bwrite = 0L;
   buffers[1].lread = 0L;
   buffers[1].lwrite = 0L;
   buffers[1].phread = 0L;
   buffers[1].phwrite = 0L;
   time_delta = 0L;
      }

   /*
    * Get a kstat_ctl handle, or update the kstat chain.
    */
   if (kc == NULL)
      kc = kstat_open();
   else
      kstat_chain_update(kc);

   if (kc == NULL)
   {
      debug_msg("couldn't open kc...");
      err_ret("determine_cpu_percentages() kstat_open() error");
      return SYNAPSE_FAILURE;
   }

   ks_name = (char*) malloc(30 * sizeof (char) );

/*
 * Modified by Robert Petkus <rpetkus@bnl.gov>
 * Get stats only for online CPUs. Previously, gmond segfaulted if
 * the CPUs were not numbered sequentially; i.e., cpu0, cpu2, etc.
 * Tested on 64 bit Solaris 8 and 9 with GCC 3.3 and 3.3.2
 */
   for (i = 0; cpu_id > 0; i++)
   {
      /*
       * Submitted by JB Kim <jbremnant@hotmail.com>
       * also skip the loop if CPU is "off-line"
       */
      int n = p_online(i, P_STATUS);
      if (n == 1) continue;

      if (n == -1 && errno == EINVAL) continue;

      sprintf(ks_name,"cpu_stat%d",i);
      ki = i;
      cpu_id--;

      debug_msg( "getting kstat:  km ='%s', ki ='%d',ks='%s'", km_name, ki, ks_name);
      ks = kstat_lookup(kc, km_name, ki, ks_name);
      if(ks == NULL)
        continue;   /* could be a CPU in state P_FAILED, see bug 321
         http://bugzilla.ganglia.info/cgi-bin/bugzilla/show_bug.cgi?id=321 */

      if (kstat_read(kc, ks,&cpuKstats) == -1) {
        perror("kstat_read");
        return SYNAPSE_FAILURE;
      }

      /* sum up to the wait state counter, the last two we determine ourselves */
      for (j = 0; j < CPU_WAIT; j++){
         cpu_now[j] += (unsigned long) cpuKstats.cpu_sysinfo.cpu[j];
      }


      cpu_now[CPUSTATE_IOWAIT] += (unsigned long) cpuKstats.cpu_sysinfo.wait[W_IO] +
                                  (unsigned long) cpuKstats.cpu_sysinfo.wait[W_PIO];
      cpu_now[CPUSTATE_SWAP] += (unsigned long) cpuKstats.cpu_sysinfo.wait[W_SWAP];

      buffers[0].bread += (long)cpuKstats.cpu_sysinfo.bread;
      buffers[0].bwrite += (long)cpuKstats.cpu_sysinfo.bwrite;
      buffers[0].lread += (long)cpuKstats.cpu_sysinfo.lread;
      buffers[0].lwrite += (long)cpuKstats.cpu_sysinfo.lwrite;
      buffers[0].phread += (long)cpuKstats.cpu_sysinfo.phread;
      buffers[0].phwrite += (long)cpuKstats.cpu_sysinfo.phwrite;

      }
   free(ks_name);

/*
 * now we have our precious "data" and have to manipulate it - compare new
 * to old and calculate percentages and sums.
 */
   buffers[2].bread = buffers[0].bread - buffers[1].bread;
   buffers[2].bwrite = buffers[0].bwrite - buffers[1].bwrite;
   buffers[2].lread = buffers[0].lread - buffers[1].lread;
   buffers[2].lwrite = buffers[0].lwrite - buffers[1].lwrite;
   buffers[2].phread = buffers[0].phread - buffers[1].phread;
   buffers[2].phwrite = buffers[0].phwrite - buffers[1].phwrite;

   debug_msg("Raw:  bread / bwrite / lread / lwrite / phread / phwrite\n%u,%u,%u / %u,%u,%u / %u,%u,%u / %u,%u,%u / %u,%u,%u / %u,%u,%u",
   buffers[0].bread,buffers[1].bread,buffers[2].bread,
   buffers[0].bwrite,buffers[1].bwrite,buffers[2].bwrite,
   buffers[0].lread,buffers[1].lread,buffers[2].lread,
   buffers[0].lwrite,buffers[1].lwrite,buffers[2].lwrite,
   buffers[0].phread,buffers[1].phread,buffers[2].phread,
   buffers[0].phwrite,buffers[1].phwrite,buffers[2].phwrite);

   time_delta = (unsigned long)time(NULL) - (unsigned long)last_refresh;
   if (time_delta == 0)
      time_delta = 1;

/*
 * decay stuff
 * semi-stolen from top.  :)  added by swagner on 8/20/02
 */
   if (time_delta < 30) {
      alpha = 0.5 * (time_delta / 30);
      beta = 1.0 - alpha;
   } else {
      alpha = 0.5;
      beta = 0.5;
   }
   metriclist.bread_sec.f = (float)buffers[2].bread / (float)time_delta;
   if (buffers[1].bread == buffers[0].bread)
      metriclist.bread_sec.f = 0.;

   metriclist.bwrite_sec.f  = (float)buffers[2].bwrite / (float)time_delta;
   if (buffers[1].bwrite == buffers[0].bwrite)
      metriclist.bwrite_sec.f = 0.;

   metriclist.lread_sec.f   = (float)buffers[2].lread / (float)time_delta;
   if (buffers[1].bwrite == buffers[0].lread)
      metriclist.lread_sec.f = 0.;

   metriclist.lwrite_sec.f  = (float)buffers[2].lwrite / (float)time_delta;
   if (buffers[1].bwrite == buffers[0].lwrite)
      metriclist.lwrite_sec.f = 0.;

   metriclist.phread_sec.f  = (float)buffers[2].phread / (float)time_delta;
   if (buffers[1].bwrite == buffers[0].phread)
      metriclist.phread_sec.f = 0.;

   metriclist.phwrite_sec.f = (float)buffers[2].phwrite / (float)time_delta;
   if (buffers[1].bwrite == buffers[0].phwrite)
      metriclist.phwrite_sec.f = 0.;

   debug_msg("Aftermath: %f %f %f %f %f %f delta = %u",
           metriclist.bread_sec.f, metriclist.bwrite_sec.f,
           metriclist.lread_sec.f, metriclist.lwrite_sec.f,
           metriclist.phread_sec.f, metriclist.phwrite_sec.f,
           time_delta
   );

   buffers[1].bread = buffers[0].bread;
   buffers[1].bwrite = buffers[0].bwrite;
   buffers[1].lread = buffers[0].lread;
   buffers[1].lwrite = buffers[0].lwrite;
   buffers[1].phread = buffers[0].phread;
   buffers[1].phwrite = buffers[0].phwrite;

   diff_cycles = percentages(CPUSTATES, cpu_states, cpu_now, cpu_old, cpu_diff);

   debug_msg ("** ** ** ** ** Are percentages electric?  Try %d%%, %d%% , %d%% , %d%% , %d%% %d%%", cpu_states[0],cpu_states[1],cpu_states[2],cpu_states[3],cpu_states[4]);

/*
 * i don't know how you folks do things in new york city, but around here folks
 * don't go around dividing by zero.
 */
   if (diff_cycles < 1)
       {
       debug_msg("diff_cycles < 1 ... == %f %u!", diff_cycles, diff_cycles);
       diff_cycles = 1;
       }

/*
 * could this be ANY HARDER TO READ?  sorry.  through hacking around i found
 * that explicitly casting everything as floats seems to work...
 */
   metriclist.cpu_idle.f = (float) cpu_states[CPUSTATE_IDLE] / 10;
   metriclist.cpu_user.f = (float) cpu_states[CPUSTATE_USER] / 10;
   metriclist.cpu_system.f = (float)(cpu_states[CPUSTATE_KERNEL] + cpu_states[CPUSTATE_SWAP]) / 10;
   metriclist.cpu_wio.f = (float) cpu_states[CPUSTATE_IOWAIT] / 10;

   metriclist.rcache.f = 100.0 * ( 1.0 - ( (float)buffers[0].bread / (float)buffers[0].lread ) );
   metriclist.wcache.f = 100.0 * ( 1.0 - ( (float)buffers[0].bwrite / (float)buffers[0].lwrite ) );

   last_refresh = time(NULL);
   return(0);
}

/*
 * The following two functions retrieve statistics from all physical
 * network interfaces.
 */
static uint64_t oifctr[4];
static uint64_t nifctr[4];

static int
extract_if_data(kstat_t *ks)
{
   kstat_named_t *kn;

   if (strcmp(ks->ks_name, "lo0") == 0)
      return 0;

   if (kstat_read(kc, ks, 0) == -1) {
      debug_msg("couldn't open kc...");
      err_ret("extract_if_data() kstat_read() error");
      return SYNAPSE_FAILURE;
    }

   kn = kstat_data_lookup(ks, "rbytes64");
   if (kn) nifctr[0] += kn->value.ui64;
   kn = kstat_data_lookup(ks, "obytes64");
   if (kn) nifctr[1] += kn->value.ui64;
   kn = kstat_data_lookup(ks, "ipackets64");
   if (kn) nifctr[2] += kn->value.ui64;
   kn = kstat_data_lookup(ks, "opackets64");
   if (kn) nifctr[3] += kn->value.ui64;
   /* fprintf(stderr,"kn = %x %u\n",kn,kn->value.ui64); */

   return 0;
}

static void
update_if_data(void)
{
   static int init_done = 0;
   static struct timeval lasttime={0,0};
   struct timeval thistime;
   double timediff;
   kstat_t *info;
   char buff[20];

   /*
    * Compute time between calls
    */
   gettimeofday (&thistime, NULL);
   if (lasttime.tv_sec)
     timediff = ((double) thistime.tv_sec * 1.0e6 +
                 (double) thistime.tv_usec -
                 (double) lasttime.tv_sec * 1.0e6 -
                 (double) lasttime.tv_usec) / 1.0e6;
   else
     timediff = 1.0;

   /*
    * Do nothing if we are called to soon after the last call
    */
   if (init_done && (timediff < 10.)) return;

   lasttime = thistime;

   /*
    * Get a kstat_ctl handle, or update the kstat chain.
    */
   if (kc == NULL)
      kc = kstat_open();
   else
      kstat_chain_update(kc);

   if (kc == NULL)
      {
         debug_msg("couldn't open kc...");
         err_ret("update_if_data() kstat_open() error");
         return;
      }
   /* fprintf(stderr,"kc = %x\n",kc); */

   /*
    * Loop over all interfaces to get statistics
    */
   nifctr[0] = nifctr[1] = nifctr[2] = nifctr[3] = 0;

   info = kc->kc_chain;
   while (info) {
      if (strcmp(info->ks_class, "net") == 0) {
         sprintf(buff, "%s%d", info->ks_module, info->ks_instance);
         if (strcmp(info->ks_name, buff) == 0) {
            extract_if_data(info);
         }
      }
      info = info->ks_next;
   }

   if (init_done) {
     if (nifctr[0] >= oifctr[0]) metriclist.bytes_in.f = (double)(nifctr[0] - oifctr[0])/timediff;
     if (nifctr[1] >= oifctr[1]) metriclist.bytes_out.f = (double)(nifctr[1] - oifctr[1])/timediff;
     if (nifctr[2] >= oifctr[2]) metriclist.pkts_in.f = (double)(nifctr[2] - oifctr[2])/timediff;
     if (nifctr[3] >= oifctr[3]) metriclist.pkts_out.f = (double)(nifctr[3] - oifctr[3])/timediff;
   }
   else {
     init_done = 1;
     metriclist.bytes_in.f = 0.;
     metriclist.bytes_out.f = 0.;
     metriclist.pkts_in.f = 0.;
     metriclist.pkts_out.f = 0.;
   }

   oifctr[0] = nifctr[0];
   oifctr[1] = nifctr[1];
   oifctr[2] = nifctr[2];
   oifctr[3] = nifctr[3];

   /*
   fprintf(stderr,"inb = %f\n",metriclist.bytes_in.f);
   fprintf(stderr,"onb = %f\n",metriclist.bytes_out.f);
   fprintf(stderr,"ipk = %f\n",metriclist.pkts_in.f);
   fprintf(stderr,"opk = %f\n",metriclist.pkts_out.f);
   */

   return;
}

/*
 * This function is called only once by the gmond.  Use to
 * initialize data structures, etc or just return SYNAPSE_SUCCESS;
 */
g_val_t
metric_init( void )
{

/*
 * swagner's stuff below, initialization for reading running kernel data ...
 */

   g_val_t val;

   get_kstat_val(&metriclist.cpu_num, "unix","system_misc","ncpus");
   debug_msg("metric_init: Assigning cpu_num value (%d) to ncpus.",(int)metriclist.cpu_num.uint32);
   g_ncpus = metriclist.cpu_num.uint32;

   get_kstat_val(&metriclist.boottime, "unix","system_misc","boot_time");
   get_kstat_val(&metriclist.cpu_speed,    "cpu_info","cpu_info0","clock_MHz");

/* first we get the uname data (hence my including <sys/utsname.h> ) */
   (void) uname( &unamedata );
/*
 * these values don't change from tick to tick.  at least, they shouldn't ...
 * also, these strings don't use the ganglia metric struct!
 */
   metriclist.os_name = unamedata.sysname;
   metriclist.os_release = unamedata.release;
   metriclist.machine_type = unamedata.machine;
   update_metric_data();
   update_if_data();
   debug_msg("solaris.c: metric_init() ok.");
   val.int32 = SYNAPSE_SUCCESS;
   first_run = 0;
/*
 * We need to make sure that every server thread gets their own copy of "kc".
 * The next metric that needs a kc-handle will reopen it for the server thread.
 */
   if (kc) {
     kstat_close(kc);
     kc = NULL;
   }
   return val;
}

void
metric_tick ( void )
{
   double thetime = time(NULL);
   /*  update every 30 seconds */
   if ( thetime >= ( metriclist.sys_clock.uint32 + TICK_SECONDS) ) {
      update_metric_data();
   }
}

g_val_t
cpu_num_func ( void )
{
   g_val_t val;

   val.uint16 = metriclist.cpu_num.uint32;
   return val;
}

g_val_t
mtu_func ( void )
{
   g_val_t val;
   val.uint32 = get_min_mtu();
   /* A val of 0 means there are no UP interfaces. Shouldn't happen. */
   return val;
}

/* ------------------------------------------------------------------------- */

g_val_t
bytes_in_func(void)
{
   g_val_t val;

   update_if_data();
   val.f = metriclist.bytes_in.f;
   return val;
}

g_val_t
bytes_out_func(void)
{
   g_val_t val;

   update_if_data();
   val.f = metriclist.bytes_out.f;
   return val;
}

g_val_t
pkts_in_func(void)
{
   g_val_t val;

   update_if_data();
   val.f = metriclist.pkts_in.f;
   return val;
}

g_val_t
pkts_out_func(void)
{
   g_val_t val;

   update_if_data();
   val.f = metriclist.pkts_out.f;
   return val;
}

/* --- snip!  preceding code lifted from linux.c --- */

g_val_t
cpu_speed_func ( void )
{
   g_val_t val;

   val.uint32 = metriclist.cpu_speed.uint32;
   return val;
}

g_val_t
mem_total_func ( void )
{
   g_val_t val;

   val.f = metriclist.mem_total.uint32;
//   val.uint32 = pagetok(sysconf(_SC_PHYS_PAGES));
   return val;
}

g_val_t
swap_total_func ( void )
{
   g_val_t val;

   metric_tick();
   val.f = metriclist.swap_total.uint32;
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

   metric_tick();
   val.uint32 = (uint32_t)time(NULL);
   return val;
}

g_val_t
machine_type_func ( void )
{
   g_val_t val;

   strncpy( val.str, metriclist.machine_type, MAX_G_STRING_SIZE );
   return val;
}

g_val_t
os_name_func ( void )
{
   g_val_t val;

   strncpy( val.str, unamedata.sysname, MAX_G_STRING_SIZE );
   return val;
}

g_val_t
os_release_func ( void )
{
   g_val_t val;

   strncpy( val.str, unamedata.release, MAX_G_STRING_SIZE );
   return val;
}

g_val_t
cpu_user_func ( void )
{
   g_val_t val;

   determine_cpu_percentages();
   val.f = metriclist.cpu_user.f;
   return val;
}

/* FIXME: ? */
g_val_t
cpu_nice_func ( void )
{
   g_val_t val;

   val.f = 0.0;   /*  no more mr. nice procs ... */

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

   val.f = metriclist.cpu_idle.f;
   return val;
}

/* FIXME: always 0? */
g_val_t
cpu_wio_func ( void )
{
   g_val_t val;

   val.f = metriclist.cpu_wio.f;
   return val;
}

g_val_t
load_one_func ( void )
{
   g_val_t val;

   metric_tick();
   val.f = metriclist.load_one.uint32;
   val.f = loaddouble(val.f);
   return val;
}

g_val_t
load_five_func ( void )
{
   g_val_t val;

   metric_tick();
   val.f = metriclist.load_five.uint32;
   val.f = loaddouble(val.f);
   return val;
}

g_val_t
load_fifteen_func ( void )
{
   g_val_t val;

   metric_tick();
   val.f = metriclist.load_fifteen.uint32;
   val.f = loaddouble(val.f);
   return val;
}

/*
 * The definition of a "running" Process seems to be different from Linux :-)
 * Anyway, the numbers look sane. Suggestions are welcome. (MKN)
 */
#define PROCFS          "/proc"
g_val_t
proc_run_func( void )
{
   char filename_buf[64];
   DIR *procdir;
   struct dirent *direntp;
   psinfo_t psinfo;
   int fd,proc_no;
   g_val_t val;

   val.uint32 = 0;

   if (!(procdir = opendir(PROCFS))) {
     (void) fprintf(stderr, "Unable to open %s\n",PROCFS);
     return val;
   }

   strcpy(filename_buf, "/proc/");
   for (proc_no = 0; (direntp = readdir (procdir)); ) {
     if (direntp->d_name[0] == '.')
       continue;

     sprintf(&filename_buf[6],"%s/psinfo", direntp->d_name);
     if ((fd = open (filename_buf, O_RDONLY)) < 0)
       continue;

     if (read (fd, &psinfo, sizeof(psinfo_t)) != sizeof(psinfo_t)) {
       (void) close(fd);
       continue;
     }
     (void) close(fd);

     if (psinfo.pr_lwp.pr_sname == 'O') {
       val.uint32++;
       /*fprintf(stderr, "Filename = <%s> %i <%c>\n",filename_buf,psinfo.pr_lwp.pr_state,psinfo.pr_lwp.pr_sname);*/
     }
   }
   closedir(procdir);

   return val;
}

g_val_t
proc_total_func ( void )
{
   g_val_t val;

   metric_tick();
   val = metriclist.proc_total;
   return val;
}

g_val_t
mem_free_func ( void )
{
   g_val_t val;

   metric_tick();
   val.f = metriclist.mem_free.uint32;
   return val;
}

/*
 * FIXME ? MKN
 */
g_val_t
mem_shared_func ( void )
{
   g_val_t val;

   val.f = 0;

   return val;
}

/*
 * FIXME ? MKN
 */
g_val_t
mem_buffers_func ( void )
{
   g_val_t val;

   val.f = 0;

   return val;
}

/*
 * FIXME ? MKN
 */
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

   metric_tick();
   val.f = metriclist.swap_free.uint32;
   return val;
}

/* some solaris-specific stuff.  enjoy. */

g_val_t
bread_sec_func(void)
{
   g_val_t val;

   val.f = metriclist.bread_sec.f;
   return val;
}

g_val_t
bwrite_sec_func(void)
{
   g_val_t val;

   val.f = metriclist.bwrite_sec.f;
   return val;
}

g_val_t
lread_sec_func(void)
{
   g_val_t val;

   val.f = metriclist.lread_sec.f;
   return val;
}

g_val_t
lwrite_sec_func(void)
{
   g_val_t val;

   val.f = metriclist.lwrite_sec.f;
   return val;
}

g_val_t
phread_sec_func(void)
{
   g_val_t val;

   val.f = metriclist.phread_sec.f;
   return val;
}

g_val_t
phwrite_sec_func(void)
{
   g_val_t val;

   val.f = metriclist.phwrite_sec.f;
   return val;
}

g_val_t
rcache_func(void)
{
   g_val_t val;

   val.f = metriclist.rcache.f;
   return val;
}

g_val_t
wcache_func(void)
{
   g_val_t val;

   val.f = metriclist.wcache.f;
   return val;
}

/*
 * FIXME
 */
g_val_t
cpu_aidle_func ( void )
{
   g_val_t val;
   val.f = 0.0;
   return val;
}

/*
 * FIXME
 */
g_val_t
cpu_intr_func ( void )
{
   g_val_t val;
   val.f = 0.0;
   return val;
}

/*
 * FIXME
 */
g_val_t
cpu_sintr_func ( void )
{
   g_val_t val;
   val.f = 0.0;
   return val;
}

/*
 * Solaris Specific path.  but this is a Solaris file even if mostly
 * stolen from the Linux one
 */
#define MOUNTS "/etc/mnttab"

/*
 * Prior to Solaris 8 was a regular plain text file which should be locked
 * on read to ensure consistency; a read-only filesystem in newer releases
 */

/* ------------------------------------------------------------------------- */
int valid_mount_type(const char *type)
{
   return ((strncmp(type, "ufs", 3) == 0) || (strncmp(type, "vxfs", 4) == 0));
}

/* ------------------------------------------------------------------------- */
float device_space(char *mount, char *device, double *total_size, double *total_free)
{
   struct statvfs buf;
   u_long blocksize;
   fsblkcnt_t free, size;
   float pct = 0.0;

   statvfs(mount, &buf);
   size = buf.f_blocks;
   free = buf.f_bavail;
   blocksize = buf.f_frsize;
   /* Keep running sum of total used, free local disk space. */
   *total_size += size * (double)blocksize;
   *total_free += free * (double)blocksize;
   pct = size ? ((size - free) / (float)size) * 100 : 0.0;
   return pct;
}

/* ------------------------------------------------------------------------- */
float find_disk_space(double *total_size, double *total_free)
{
   FILE *mounts;
   struct mnttab mp;
   char *mount, *device, *type;
   /* We report in GB = 1 thousand million bytes */
   const double reported_units = 1e9;
   /* Track the most full disk partition, report with a percentage. */
   float thispct, max=0.0;

   /* Read all currently mounted filesystems. */
   mounts=fopen(MOUNTS,"r");
   if (!mounts) {
      debug_msg("Df Error: could not open mounts file %s. Are we on the right OS?\n", MOUNTS);
      return max;
   }

   while (getmntent(mounts, &mp) == 0) {
      mount = mp.mnt_mountp;
      device = mp.mnt_special;
      type = mp.mnt_fstype;

      if (!valid_mount_type(type)) continue;

      thispct = device_space(mount, device, total_size, total_free);
      debug_msg("Counting device %s (%.2f %%)", device, thispct);
      if (!max || max<thispct)
         max = thispct;
   }
   fclose(mounts);

   *total_size = *total_size / reported_units;
   *total_free = *total_free / reported_units;
   debug_msg("For all disks: %.3f GB total, %.3f GB free for users.", *total_size, *total_free);

   return max;
}

g_val_t
disk_free_func ( void )
{
   double total_free = 0.0;
   double total_size = 0.0;
   g_val_t val;

   find_disk_space(&total_size, &total_free);

   val.d = total_free;
   return val;
}

g_val_t
disk_total_func ( void )
{
   double total_free = 0.0;
   double total_size = 0.0;
   g_val_t val;

   find_disk_space(&total_size, &total_free);

   val.d = total_size;
   return val;
}

g_val_t
part_max_used_func ( void )
{
   double total_free = 0.0;
   double total_size = 0.0;
   float most_full;
   g_val_t val;

   most_full = find_disk_space(&total_size, &total_free);

   val.f = most_full;
   return val;
}

g_val_t
cpu_steal_func ( void )
{
   static g_val_t val=0;
   return val;
}

