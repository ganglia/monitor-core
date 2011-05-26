/*
 *  First stab at support for metrics in FreeBSD 
 *  by Preston Smith <psmith@physics.purdue.edu>
 *  Wed Feb 27 14:55:33 EST 2002
 *  Improved by Brooks Davis <brooks@one-eyed-alien.net>,
 *  Fixed libkvm code.
 *  Tue Jul 15 16:42:22 EST 2003
 *  All bugs added by Carlo Marcelo Arenas Belon <carenas@sajinet.com.pe>
 *
 * Tested on FreeBSD 7 (amd64)
 * Tested on FreeBSD 6.2 (amd64 and i386)
 * Tested on FreeBSD 5.5 (i386)
 * Tested on FreeBSD 8 (amd64)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <kvm.h>

#include <sys/param.h> 
#include <sys/mount.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/user.h>
#if __FreeBSD_version < 500101
#include <sys/dkstat.h>
#else
#include <sys/resource.h>
#endif
#include <sys/stat.h>
#include <sys/vmmeter.h>
#include <vm/vm_param.h>

#include <sys/socket.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>

#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <paths.h>

#include "interface.h"
#include "libmetrics.h"

#define MIB_SWAPINFO_SIZE 3

#ifndef MIN_NET_POLL_INTERVAL
#define MIN_NET_POLL_INTERVAL 0.5
#endif

#ifndef MIN_CPU_POLL_INTERVAL
#define MIN_CPU_POLL_INTERVAL 0.5
#endif

#ifndef UINT64_MAX
#define UINT64_MAX	ULLONG_MAX
#endif

#define VFCF_NONLOCAL	(VFCF_NETWORK|VFCF_SYNTHETIC|VFCF_LOOPBACK)

#define timertod(tvp) \
    ((double)(tvp)->tv_sec + (double)(tvp)->tv_usec/(1000*1000))

#ifndef XSWDEV_VERSION
#define XSWDEV_VERSION  1
struct xswdev {
        u_int   xsw_version;
        udev_t  xsw_dev;
        int     xsw_flags;
        int     xsw_nblks;
        int     xsw_used;
};
#endif

struct traffic {
	uint64_t in_bytes;
	uint64_t out_bytes;
	uint64_t in_pkts;
	uint64_t out_pkts;
};

static void get_netbw(double *, double *, double *, double *);
static uint64_t counterdiff(uint64_t, uint64_t, uint64_t, uint64_t);


static char	 *makenetvfslist(void);
static size_t	  regetmntinfo(struct statfs **, long, const char **);
static int	  checkvfsname(const char *, const char **);
static const char **makevfslist(char *);
static float	  find_disk_space(double *, double *);

static int use_vm_swap_info = 0;
static int mibswap[MIB_SWAPINFO_SIZE];
static size_t mibswap_size;
static kvm_t *kd = NULL;
static int pagesize;
static int	  skipvfs = 1;

/* Function prototypes */
static long percentages(int cnt, int *out, register long *new,
                          register long *old, long *diffs);
int cpu_state(int which);
 
/*
 * This function is called only once by the gmond.  Use to 
 * initialize data structures, etc or just return SYNAPSE_SUCCESS;
 */
g_val_t
metric_init(void)
{
   g_val_t val;

   /*
    * Try to use the vm.swap_info sysctl to gather swap data.  If it
    * isn't implemented, fall back to trying to old kvm based interface.
    */
   mibswap_size = MIB_SWAPINFO_SIZE;
   if (sysctlnametomib("vm.swap_info", mibswap, &mibswap_size) == -1) {
      kd = kvm_open(NULL, NULL, NULL, O_RDONLY, "metric_init()");
   } else {
      /*
       * RELEASE versions of FreeBSD with the swap mib have a version
       * of libkvm that doesn't need root for simple proc access so we
       * just open /dev/null to give us a working handle here.
       */
      kd = kvm_open(_PATH_DEVNULL, NULL, NULL, O_RDONLY, "metric_init()");
      use_vm_swap_info = 1;
   }
   pagesize = getpagesize();

   /* Initalize some counters */
   get_netbw(NULL, NULL, NULL, NULL);
   cpu_state(-1);

   val.int32 = SYNAPSE_SUCCESS;
   return val;
}

g_val_t
cpu_num_func ( void )
{
   g_val_t val;
   int ncpu;
   size_t len = sizeof (int);
   if (sysctlbyname("hw.ncpu", &ncpu, &len, NULL, 0) == -1 || !len)
        ncpu = 1;

   val.uint16 = ncpu;
   return val;
}

g_val_t
cpu_speed_func ( void )
{
   g_val_t val;
   char buf[1024];
   char *curptr;
   size_t len;
   uint32_t freq = 0, tmpfreq;
   uint64_t tscfreq;
   unsigned int cpu_freq;

   /*
    * Try the portable sysctl (introduced on ia64).
    */
   cpu_freq = 0;
   len = sizeof(cpu_freq);
   if (sysctlbyname("hw.freq.cpu", &cpu_freq, &len, NULL, 0) != -1 &&
       cpu_freq != 0) {
      freq = cpu_freq;
      goto done;
   }

   /*
    * If the system supports it, the cpufreq driver provides
    * access to CPU frequency.  Since we want a constant value, we're
    * looking for the maximum frequency, not the current one.  We
    * don't know what order the driver will report values in so we
    * search for the highest one by parsing the string returned by the
    * dev.cpu.0.freq_levels sysctl.  The format of the string is a space
    * seperated list of MHz/milliwatts.
    */
   tmpfreq = 0;
   len = sizeof(buf);
   if (sysctlbyname("dev.cpu.0.freq_levels", buf, &len, NULL, 0) == -1)
      buf[0] = '\0';
   curptr = buf;
   while (isdigit(curptr[0])) {
      freq = strtol(curptr, &curptr, 10);
      if (freq > tmpfreq)
         tmpfreq = freq;
      /* Skip the rest of this entry */
      while (!isspace(curptr[0]) && curptr[0] != '\0')
         curptr++;
      /* Find the next entry */
      while (!isdigit(curptr[0]) && curptr[0] != '\0')
         curptr++;
   }
   freq = tmpfreq;
   if (freq != 0)
      goto done;

   /*
    * machdep.tsc_freq exists on some i386/amd64 machines and gives the
    * CPU speed in Hz.  If it exists it's a decent value.
    */
   tscfreq = 0;
   len = sizeof(tscfreq);
   if (sysctlbyname("machdep.tsc_freq", &tscfreq, &len, NULL, 0) != -1) {
      freq = tscfreq / 1e6;
      goto done;
   }

done:
   val.uint32 = freq;

   return val;
}

g_val_t
mem_total_func ( void )
{
   g_val_t val;
   size_t len;
   u_long total;

   len = sizeof(total);

   if (sysctlbyname("hw.physmem", &total, &len, NULL, 0) == -1)
      total = 0;

   val.f = total / 1024;

   return val;
}

g_val_t
swap_total_func ( void )
{
   g_val_t val;
   struct kvm_swap swap[1];
   struct xswdev xsw;
   size_t size;
   int totswap, n;
   val.f = 0;
   totswap = 0;

   if (use_vm_swap_info) {
      for (n = 0; ; ++n) {
        mibswap[mibswap_size] = n;
        size = sizeof(xsw);
        if (sysctl(mibswap, mibswap_size + 1, &xsw, &size, NULL, 0) == -1)
           break;
        if (xsw.xsw_version != XSWDEV_VERSION)
           return val;
         totswap += xsw.xsw_nblks;
       }
   } else if(kd != NULL) {
      n = kvm_getswapinfo(kd, swap, 1, 0);
      if (n < 0 || swap[0].ksw_total == 0) {
         val.f = 0;
      }
      totswap = swap[0].ksw_total;
    }
   
   val.f = totswap * (pagesize / 1024);
   return val;
}

g_val_t
boottime_func ( void )
{
   g_val_t val;
   struct timeval boottime;
   size_t size;

   size = sizeof(boottime);
   if (sysctlbyname("kern.boottime", &boottime, &size, NULL, 0) == -1)
       boottime.tv_sec = 0;

   val.uint32 = (uint32_t) boottime.tv_sec;

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
	size_t len = sizeof(val.str);

	if (sysctlbyname("hw.machine", val.str, &len, NULL, 0) == -1 ||
	    (len == 0))
		strlcpy(val.str, "unknown", sizeof(val.str));

	return val;
}

g_val_t
os_name_func ( void )
{
	g_val_t val;
	size_t len = sizeof(val.str);

	if (sysctlbyname("kern.ostype", val.str, &len, NULL, 0) == -1 ||
	    (len == 0))
		strlcpy(val.str, "FreeBSD (unknown)", sizeof(val.str));

	return val;
}        

g_val_t
os_release_func ( void )
{
	g_val_t val;
	size_t len = sizeof(val.str);

	if (sysctlbyname("kern.osrelease", val.str, &len, NULL, 0) == -1 ||
	    (len == 0))
		strlcpy(val.str, "unknown", sizeof(val.str));

	return val;
}        

/* Get the CPU state given by index, from kern.cp_time
 * Use the constants in <sys/dkstat.h>
 * CP_USER=0, CP_NICE=1, CP_SYS=2, CP_INTR=3, CP_IDLE=4
 */
int cpu_state(int which) {

   long cp_time[CPUSTATES];
   long cp_diff[CPUSTATES];
   static long cp_old[CPUSTATES];
   static int cpu_states[CPUSTATES];
   static struct timeval this_time, last_time;
   struct timeval time_diff;
   size_t len = sizeof(cp_time);

   if (which == -1) {
      bzero(cp_old, sizeof(cp_old));
      bzero(&last_time, sizeof(last_time));
      return 0.0;
   }

   gettimeofday(&this_time, NULL);
   timersub(&this_time, &last_time, &time_diff);
   if (timertod(&time_diff) < MIN_CPU_POLL_INTERVAL) {
      goto output;
   }
   last_time = this_time;

   /* puts kern.cp_time array into cp_time */
   if (sysctlbyname("kern.cp_time", &cp_time, &len, NULL, 0) == -1) {
      warn("kern.cp_time");
      return 0.0;
   }
   /* Use percentages function lifted from top(1) to figure percentages */
   percentages(CPUSTATES, cpu_states, cp_time, cp_old, cp_diff);

output:
   return cpu_states[which];
}

g_val_t
cpu_user_func ( void )
{
   g_val_t val;

   val.f = (float) cpu_state(CP_USER)/10;

   return val;
}

g_val_t
cpu_nice_func ( void )
{
   g_val_t val;

   val.f = (float) cpu_state(CP_NICE)/10;

   return val;
}

g_val_t 
cpu_system_func ( void )
{
   g_val_t val;

   val.f = (float) cpu_state(CP_SYS)/10;

   return val;
}

g_val_t 
cpu_idle_func ( void )
{
   g_val_t val;

   val.f = (float) cpu_state(CP_IDLE)/10;

   return val;
}

/*
** FIXME - This metric is not valid on FreeBSD.
*/
g_val_t 
cpu_wio_func ( void )
{
   g_val_t val;
   
   val.f = 0.0;
   return val;
}

/*
** FIXME - Idle time since startup.  The scheduler apparently knows
** this, but we it's fairly pointless so it's not exported.
*/
g_val_t 
cpu_aidle_func ( void )
{
   g_val_t val;
   val.f = 0.0;
   return val;
}

g_val_t 
cpu_intr_func ( void )
{
   g_val_t val;

   val.f = (float) cpu_state(CP_INTR)/10;

   return val;
}

/*
** FIXME - This metric is not valid on FreeBSD.
*/
g_val_t 
cpu_sintr_func ( void )
{
   g_val_t val;
   val.f = 0.0;
   return val;
}

g_val_t
load_one_func ( void )
{
   g_val_t val;
   double load[3];

   getloadavg(load, 3);
   val.f = load[0];

   return val;
}

g_val_t
load_five_func ( void )
{
   g_val_t val;
   double load[3];

   getloadavg(load, 3);
   val.f = load[1];

   return val;
}

g_val_t
load_fifteen_func ( void )
{
   g_val_t val;
   double load[3];

   getloadavg(load, 3);
   val.f = load[2];

   return val;
}

g_val_t
proc_total_func ( void )
{
   g_val_t val;
   struct vmtotal total;
   size_t len;

   /* computed every 5 seconds */
   len = sizeof(total);
   sysctlbyname("vm.vmtotal", &total, &len, NULL, 0);

   val.uint32 = total.t_rq + \
      total.t_dw + total.t_pw + total.t_sl + total.t_sw; 

   return val;
}

g_val_t
proc_run_func( void )
{
   struct kinfo_proc *kp;
   int i;
   int state;
   int nentries;
   int what = KERN_PROC_ALL;
   g_val_t val;

   val.uint32 = 0;

   if (kd == NULL)
      goto output;
#ifdef KERN_PROC_NOTHREADS
   what |= KERN_PROC_NOTHREADS
#endif
   if ((kp = kvm_getprocs(kd, what, 0, &nentries)) == 0 || nentries < 0)
      goto output;

   for (i = 0; i < nentries; kp++, i++) {
      /* This is a per-CPU idle thread. */ /* idle thread */
      if ((kp->ki_tdflags & TDF_IDLETD) != 0)
         continue;
      /* Ignore during load avg calculations. */ /* swi or idle thead */
#ifdef TDF_NOLOAD
      /* Introduced in FreeBSD 8.3 */
      if ((kp->ki_tdflags & TDF_NOLOAD) != 0)
#else
      if ((kp->ki_flag & P_NOLOAD) != 0)
#endif
         continue;
#ifdef KINFO_PROC_SIZE
      state = kp->ki_stat;
#else
      state = kp->kp_proc.p_stat;
#endif
      switch(state) {
         case SRUN:
         case SIDL:
            val.uint32++;
            break;
      }
   }

   if (val.uint32 > 0)
      val.uint32--;

output:
   return val;
}

/*
** FIXME - The whole ganglia model of memory is bogus.  Free memory is
** generally a bad idea with a modern VM and so is reporting it.  There
** is simply no way to report a value for "free" memory that makes any
** kind of sense.  Free+inactive might be a decent value for "free".
*/
g_val_t
mem_free_func ( void )
{
   g_val_t val;
   size_t len; 
   int free_pages;

   len = sizeof (free_pages);
   if((sysctlbyname("vm.stats.vm.v_free_count", &free_pages, &len, NULL, 0) 
      == -1) || !len) free_pages = 0; 

   val.f = free_pages * (pagesize / 1024);
   return val;
}

/*
** FreeBSD don't seem to report this anywhere.  It's actually quite
** complicated as there is SysV shared memory, POSIX shared memory,
** and mmap shared memory at a minimum.
*/
g_val_t
mem_shared_func ( void )
{
   g_val_t val;

   val.f = 0;

   return val;
}

/*
** FIXME - this isn't really valid.  It lists some VFS buffer space,
** but the real picture is much more complex.
*/
g_val_t
mem_buffers_func ( void )
{
   g_val_t val;
   size_t len;
   int buffers;

   len = sizeof (buffers);
   if((sysctlbyname("vfs.bufspace", &buffers, &len, NULL, 0) == -1) || !len)
      buffers = 0; 

   buffers /= 1024;
   val.f = buffers;

   return val;
}

/*
** FIXME - this isn't really valid.  It lists some VM cache space,
** but the real picture is more complex.
*/
g_val_t
mem_cached_func ( void )
{
   g_val_t val;
   size_t len;
   int cache;

   len = sizeof (cache);
   if((sysctlbyname("vm.stats.vm.v_cache_count", &cache, &len, NULL, 0) == -1) 
      || !len)
      cache = 0; 

   val.f = cache * (pagesize / 1024);
   return val;
}

g_val_t
swap_free_func ( void )
{
   g_val_t val;

   struct kvm_swap swap[1];
   struct xswdev xsw;
   size_t size;
   int totswap, usedswap, freeswap, n;
   val.f = 0;
   totswap = 0;
   usedswap = 0;
   if (use_vm_swap_info) {
      for (n = 0; ; ++n) {
        mibswap[mibswap_size] = n;
        size = sizeof(xsw);
        if (sysctl(mibswap, mibswap_size + 1, &xsw, &size, NULL, 0) == -1)
           break;
        if (xsw.xsw_version != XSWDEV_VERSION)
           return val;
         totswap += xsw.xsw_nblks;
         usedswap += xsw.xsw_used;
       }
   } else if(kd != NULL) {
      n = kvm_getswapinfo(kd, swap, 1, 0);
      totswap = swap[0].ksw_total;
      usedswap = swap[0].ksw_used;
   }
   freeswap = totswap - usedswap;
   val.f = freeswap * (pagesize / 1024);
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


/*
 * Function to get cpu percentages.
 * Might be changed ever so slightly, but is still mostly:
 * AUTHOR:  Christos Zoulas <christos@ee.cornell.edu>
 *          Steven Wallace  <swallace@freebsd.org>
 *          Wolfram Schneider <wosch@FreeBSD.org>
 *
 * $FreeBSD: src/usr.bin/top/machine.c,v 1.29.2.2 2001/07/31 20:27:05 tmm Exp $
 */

static long percentages(int cnt, int *out, register long *new, 
                        register long *old, long *diffs)  {

    register int i;
    register long change;
    register long total_change;
    register long *dp;
    long half_total;

    /* initialization */
    total_change = 0;
    dp = diffs;

    /* calculate changes for each state and the overall change */
    for (i = 0; i < cnt; i++) {
        if ((change = *new - *old) < 0) {
            /* this only happens when the counter wraps */
            change = (int)
                ((unsigned long)*new-(unsigned long)*old);
        }
        total_change += (*dp++ = change);
        *old++ = *new++;
    }
    /* avoid divide by zero potential */
    if (total_change == 0) { total_change = 1; }

    /* calculate percentages based on overall change, rounding up */
    half_total = total_change / 2l;

    /* Do not divide by 0. Causes Floating point exception */
    if(total_change) {
        for (i = 0; i < cnt; i++) {
          *out++ = (int)((*diffs++ * 1000 + half_total) / total_change);
        }
    }

    /* return the total in case the caller wants to use it */
    return(total_change);
}

g_val_t
pkts_in_func ( void )
{
   double in_pkts;
   g_val_t val;

   get_netbw(NULL, NULL, &in_pkts, NULL);

   val.f = (float)in_pkts;
   return val;
}

g_val_t
pkts_out_func ( void )
{
   double out_pkts;
   g_val_t val;

   get_netbw(NULL, NULL, NULL, &out_pkts);

   val.f = (float)out_pkts;
   return val;
}

g_val_t
bytes_out_func ( void )
{
   double out_bytes;
   g_val_t val;

   get_netbw(NULL, &out_bytes, NULL, NULL);

   val.f = (float)out_bytes;
   return val;
}

g_val_t
bytes_in_func ( void )
{
   double in_bytes;
   g_val_t val;

   get_netbw(&in_bytes, NULL, NULL, NULL);

   val.f = (float)in_bytes;
   return val;
}

/*
 * Disk space reporting functions from Linux code.  find_disk_space()
 * body derived from FreeBSD df and mount code.
 */

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


/*
 * Copyright (c) 1980, 1983, 1990, 1993, 1994, 1995
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 * NOTE: The copyright of UC Berkeley's Berkeley Software Distribution
 * ("BSD") source has been updated.  The copyright addendum may be found
 * at ftp://ftp.cs.berkeley.edu/pub/4bsd/README.Impt.License.Change.
 */

/* FIXME: doesn't filter out devfs, bad reporting in amd64 */
static float
find_disk_space(double *total, double *tot_avail)
{
	struct statfs *mntbuf;
	const char *fstype;
	const char **vfslist;
	char *netvfslist;
	size_t i, mntsize;
	size_t used, availblks;
	const double reported_units = 1e9;
	double toru;
	float pct;
	float most_full = 0.0;

	*total = 0.0;
	*tot_avail = 0.0;

	fstype = "ufs";

	netvfslist = makenetvfslist();
	vfslist = makevfslist(netvfslist);

	mntsize = getmntinfo(&mntbuf, MNT_NOWAIT);
	mntsize = regetmntinfo(&mntbuf, mntsize, vfslist);
	for (i = 0; i < mntsize; i++) {
		if ((mntbuf[i].f_flags & MNT_IGNORE) == 0) {
			used = mntbuf[i].f_blocks - mntbuf[i].f_bfree;
			availblks = mntbuf[i].f_bavail + used;
			pct = (availblks == 0 ? 100.0 :
			    (double)used / (double)availblks * 100.0);
			if (pct > most_full)
				most_full = pct;

			toru = reported_units/mntbuf[i].f_bsize;
			*total += mntbuf[i].f_blocks / toru;
			*tot_avail += mntbuf[i].f_bavail / toru;
		}
	}
	free(vfslist);
	free(netvfslist);

	return most_full;
}

/*
 * Make a pass over the file system info in ``mntbuf'' filtering out
 * file system types not in vfslist and possibly re-stating to get
 * current (not cached) info.  Returns the new count of valid statfs bufs.
 */
static size_t
regetmntinfo(struct statfs **mntbufp, long mntsize, const char **vfslist)
{
	int i, j;
	struct statfs *mntbuf;

	if (vfslist == NULL)
		return (getmntinfo(mntbufp, MNT_WAIT));

	mntbuf = *mntbufp;
	for (j = 0, i = 0; i < mntsize; i++) {
		if (checkvfsname(mntbuf[i].f_fstypename, vfslist))
			continue;
		(void)statfs(mntbuf[i].f_mntonname,&mntbuf[j]);
		j++;
	}
	return (j);
}

static int
checkvfsname(vfsname, vfslist)
	const char *vfsname;
	const char **vfslist;
{

	if (vfslist == NULL)
		return (0);
	while (*vfslist != NULL) {
		if (strcmp(vfsname, *vfslist) == 0)
			return (skipvfs);
		++vfslist;
	}
	return (!skipvfs);
}

static const char **
makevfslist(fslist)
	char *fslist;
{
	const char **av;
	int i;
	char *nextcp;

	if (fslist == NULL)
		return (NULL);
	if (fslist[0] == 'n' && fslist[1] == 'o') {
		fslist += 2;
		skipvfs = 0;
	}
	for (i = 0, nextcp = fslist; *nextcp; nextcp++)
		if (*nextcp == ',')
			i++;
	if ((av = malloc((size_t)(i + 2) * sizeof(char *))) == NULL) {
		warnx("malloc failed");
		return (NULL);
	}
	nextcp = fslist;
	i = 0;
	av[i++] = nextcp;
	while ((nextcp = strchr(nextcp, ',')) != NULL) {
		*nextcp++ = '\0';
		av[i++] = nextcp;
	}
	av[i++] = NULL;
	return (av);
}

static char *
makenetvfslist(void)
{
	char *str = NULL, *strptr, **listptr = NULL;
	size_t slen = 0;
	int cnt = 0, i;

#if __FreeBSD_version > 500000
	struct xvfsconf *xvfsp, *keep_xvfsp = NULL;
	size_t buflen;
	int maxvfsconf;

	if (sysctlbyname("vfs.conflist", NULL, &buflen, NULL, 0) < 0) {
		warn("sysctl(vfs.conflist)");
		goto done;
	}
	keep_xvfsp = xvfsp = malloc(buflen);
	if (xvfsp == NULL) {
		warnx("malloc failed");
		goto done;
	}
	if (sysctlbyname("vfs.conflist", xvfsp, &buflen, NULL, 0) < 0) {
		warn("sysctl(vfs.conflist)");
		goto done;
	}
	maxvfsconf = buflen / sizeof(struct xvfsconf);

	if ((listptr = malloc(sizeof(char*) * maxvfsconf)) == NULL) {
		warnx("malloc failed");
		goto done;
	}

	for (i = 0; i < maxvfsconf; i++, xvfsp++) {
		if (xvfsp->vfc_typenum == 0)
			continue;
		if (xvfsp->vfc_flags & VFCF_NONLOCAL)
			continue;

		listptr[cnt] = strdup(xvfsp->vfc_name);
		if (listptr[cnt] == NULL) {
			warnx("malloc failed");
			goto done;
		}
		cnt++;
	}
#else
	int mib[3], maxvfsconf;
	size_t miblen;
	struct ovfsconf *ptr;

	mib[0] = CTL_VFS; mib[1] = VFS_GENERIC; mib[2] = VFS_MAXTYPENUM;
	miblen=sizeof(maxvfsconf);
	if (sysctl(mib, (unsigned int)(sizeof(mib) / sizeof(mib[0])),
	    &maxvfsconf, &miblen, NULL, 0)) {
		warnx("sysctl failed");
		goto done;
	}

	if ((listptr = malloc(sizeof(char*) * maxvfsconf)) == NULL) {
		warnx("malloc failed");
		goto done;
	}

	while ((ptr = getvfsent()) != NULL && cnt < maxvfsconf) {
		if (ptr->vfc_flags & VFCF_NONLOCAL)
			continue;

		listptr[cnt] = strdup(ptr->vfc_name);
		if (listptr[cnt] == NULL) {
			warnx("malloc failed");
			goto done;
		}
		cnt++;
	}
#endif

	if (cnt == 0)
		goto done;

	/*
	 * Count up the string lengths, we need a extra byte to hold
	 * the between entries ',' or the NUL at the end.
	 */
	slen = 0;
	for (i = 0; i < cnt; i++)
		slen += strlen(listptr[i]);
	/* for ',' */
	slen += cnt - 1;
	/* Add 3 for initial "no" and the NUL. */
	slen += 3;

	if ((str = malloc(slen)) == NULL) {
		warnx("malloc failed");
		goto done;
	}

	str[0] = 'n';
	str[1] = 'o';
	for (i = 0, strptr = str + 2; i < cnt; i++) {
		if (i > 0)
		    *strptr++ = ',';
		strcpy(strptr, listptr[i]);
		strptr += strlen(listptr[i]);
	}
	*strptr = '\0';

done:
#if __FreeBSD_version > 500000
	if (keep_xvfsp != NULL)
		free(keep_xvfsp);
#endif
	if (listptr != NULL) {
		for(i = 0; i < cnt && listptr[i] != NULL; i++)
			free(listptr[i]);
		free(listptr);
	}
	return (str);

}

static void
get_netbw(double *in_bytes, double *out_bytes,
    double *in_pkts, double *out_pkts)
{
#ifdef NETBW_DEBUG
	char		name[IFNAMSIZ];
#endif
	struct		if_msghdr *ifm, *nextifm;
	struct		sockaddr_dl *sdl;
	char		*buf, *lim, *next;
	size_t		needed;
	int		mib[6];
	int		i;
	int		index;
	static double	ibytes, obytes, ipkts, opkts;
	struct timeval	this_time;
	struct timeval	time_diff;
	struct traffic	traffic;
	static struct timeval last_time = {0,0};
	static int	indexes = 0;
	static int	*seen = NULL;
	static struct traffic *lastcount = NULL;
	static double	o_ibytes, o_obytes, o_ipkts, o_opkts;

	ibytes = obytes = ipkts = opkts = 0.0;

	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;
	mib[3] = 0;			/* address family */
	mib[4] = NET_RT_IFLIST;
	mib[5] = 0;		/* interface index */

	gettimeofday(&this_time, NULL);
	timersub(&this_time, &last_time, &time_diff);
	if (timertod(&time_diff) < MIN_NET_POLL_INTERVAL) {
		goto output;
	}

	if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0)
		errx(1, "iflist-sysctl-estimate");
	if ((buf = malloc(needed)) == NULL)
		errx(1, "malloc");
	if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0)
		errx(1, "actual retrieval of interface table");
	lim = buf + needed;

	next = buf;
	while (next < lim) {

		ifm = (struct if_msghdr *)next;
		
		if (ifm->ifm_type == RTM_IFINFO) {
			sdl = (struct sockaddr_dl *)(ifm + 1);
		} else {
			fprintf(stderr, "out of sync parsing NET_RT_IFLIST\n");
			fprintf(stderr, "expected %d, got %d\n", RTM_IFINFO,
				ifm->ifm_type);
			fprintf(stderr, "msglen = %d\n", ifm->ifm_msglen);
			fprintf(stderr, "buf:%p, next:%p, lim:%p\n", buf, next,
				lim);
			goto output;
		}

		next += ifm->ifm_msglen;
		while (next < lim) {
			nextifm = (struct if_msghdr *)next;

			if (nextifm->ifm_type != RTM_NEWADDR)
				break;

			next += nextifm->ifm_msglen;
		}

		if ((ifm->ifm_flags & IFF_LOOPBACK) || 
		    !(ifm->ifm_flags & IFF_UP))
			continue;

		index = ifm->ifm_index;

		/* If we don't have a previous value yet, make a slot. */
		if (index >= indexes) {
			seen = realloc(seen, sizeof(*seen)*(index+1));
			lastcount = realloc(lastcount,
			    sizeof(*lastcount)*(index+1));

			/* Initalize the new slots */
			for (i = indexes; i <= index; i++) {
				seen[i] = 0;
			}
			indexes = index+1;
		}

		/*
		 * If this is the first time we've seen this interface,
		 * set the last values to the current ones.  That causes
		 * us to see no bandwidth on the interface the first
		 * time, but that's OK.
		 */
		if (!seen[index]) {
			seen[index] = 1;
			lastcount[index].in_bytes = ifm->ifm_data.ifi_ibytes;
			lastcount[index].out_bytes = ifm->ifm_data.ifi_obytes;
			lastcount[index].in_pkts = ifm->ifm_data.ifi_ipackets;
			lastcount[index].out_pkts = ifm->ifm_data.ifi_opackets;
		}

		traffic.in_bytes = counterdiff(lastcount[index].in_bytes,
		    ifm->ifm_data.ifi_ibytes, ULONG_MAX, 0);
		traffic.out_bytes = counterdiff(lastcount[index].out_bytes,
		    ifm->ifm_data.ifi_obytes, ULONG_MAX, 0);
		traffic.in_pkts = counterdiff(lastcount[index].in_pkts,
		    ifm->ifm_data.ifi_ipackets, ULONG_MAX, 0);
		traffic.out_pkts = counterdiff(lastcount[index].out_pkts,
		    ifm->ifm_data.ifi_opackets, ULONG_MAX, 0);

		lastcount[index].in_bytes = ifm->ifm_data.ifi_ibytes;
		lastcount[index].out_bytes = ifm->ifm_data.ifi_obytes;
		lastcount[index].in_pkts = ifm->ifm_data.ifi_ipackets;
		lastcount[index].out_pkts = ifm->ifm_data.ifi_opackets;

#ifdef NETBW_DEBUG
		if_indextoname(index, name);
		printf("%s: \n", name);
		printf("\topackets=%llu ipackets=%llu\n",
		    traffic.out_pkts, traffic.in_pkts);
		printf("\tobytes=%llu ibytes=%llu\n",
		    traffic.out_bytes, traffic.in_bytes);
#endif

		if (timerisset(&last_time)) {
			ibytes += (double)traffic.in_bytes / timertod(&time_diff);
			obytes += (double)traffic.out_bytes / timertod(&time_diff);
			ipkts += (double)traffic.in_pkts / timertod(&time_diff);
			opkts += (double)traffic.out_pkts / timertod(&time_diff);
		}
	}
	free(buf);

	/* Save the values from this time */
	last_time = this_time;
	o_ibytes = ibytes;
	o_obytes = obytes;
	o_ipkts = ipkts;
	o_opkts = opkts;

output:
	if (in_bytes != NULL)
		*in_bytes = o_ibytes;
	if (out_bytes != NULL)
		*out_bytes = o_obytes;
	if (in_pkts != NULL)
		*in_pkts = o_ipkts;
	if (out_pkts != NULL)
		*out_pkts = o_opkts;
}

static uint64_t
counterdiff(uint64_t oldval, uint64_t newval, uint64_t maxval, uint64_t maxdiff)
{
	uint64_t diff;

	if (maxdiff == 0)
		maxdiff = maxval;

	/* Paranoia */
	if (oldval > maxval || newval > maxval)
		return 0;

	/*
	 * Tackle the easy case.  Don't worry about maxdiff here because
	 * we're SOL if it happens (i.e. assuming a reset just makes
	 * matters worse).
	 */
	if (oldval <= newval)
		return (newval - oldval);

	/*
	 * Now the tricky part.  If we assume counters never get reset,
	 * this is easy.  Unfortunaly, they do get reset on some
	 * systems, so we need to try and deal with that.  Our huristic
	 * is that if out difference is greater then maxdiff and newval
	 * is less or equal to maxdiff, then we've probably been reset
	 * rather then actually wrapping.  Obviously, you need to be
	 * careful to poll often enough that you won't exceed maxdiff or
	 * you will get undersized numbers when you do wrap.
	 */
	diff = maxval - oldval + newval;
	if (diff > maxdiff && newval <= maxdiff)
		return newval;

	return diff;
}
