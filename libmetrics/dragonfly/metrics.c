/*
 *  Based on dfly-metrics.c,v 1.1 from pkg_src by joerg which was based
 *  on the original FreeBSD metrics by Preston and Brooks.
 *
 *  All bugs added by Carlo Marcelo Arenas Belon <carenas@sajinet.com.pe>
 *
 *  Tested on DragonFlyBSD 1.10.1 (i386)
 *  Tested on DragonFlyBSD 1.12.0 (i386)
 *  Tested on DragonFlyBSD 2.0.1 (i386)
 *  Tested on DragonFlyBSD 2.4.1 (i386, amd64)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kvm.h>

#include <sys/param.h> 
#include <sys/mount.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/user.h>
#include <kinfo.h>
#include <sys/stat.h>
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
static int cpu_state(int which);

static int use_vm_swap_info = 0;
static int mibswap[MIB_SWAPINFO_SIZE];
static size_t mibswap_size;
static kvm_t *kd = NULL;
static int pagesize;
static int	  skipvfs;

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
       * RELEASE versions of DragonFlyBSD with the swap mib have a version
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

   if (kinfo_get_cpus(&ncpu))
        ncpu = 1;

   val.uint16 = ncpu;
   return val;
}

/* FIXME? */
g_val_t
cpu_speed_func ( void )
{
   g_val_t val;
   int cpu_speed;
   size_t len = sizeof(cpu_speed);

   /*
    * machdep.tsc_freq is an i386/amd64 only feature, but it's the best
    * we've got at the moment.
    */
   if (sysctlbyname("machdep.tsc_freq", &cpu_speed, &len, NULL, 0) == -1)
     cpu_speed = 0;
   val.uint16 = cpu_speed /= 1000000;

   return val;
}

g_val_t
mem_total_func ( void )
{
   g_val_t val;
   size_t len;
   long total;

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
		strlcpy(val.str, "DragonFly (unknown)", sizeof(val.str));

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
   static struct kinfo_cputime cp_old, cp_diff;
   static uint64_t total_change, half_change;
   static struct timeval this_time, last_time;

   struct kinfo_cputime cp_time;
   struct timeval time_diff;

   if (which == -1) {
      bzero(&cp_old, sizeof(cp_old));
      bzero(&cp_diff, sizeof(cp_diff));
      total_change = 1;
      half_change = 0;
      bzero(&last_time, sizeof(last_time));
      return 0.0;
   }

   gettimeofday(&this_time, NULL);
   timersub(&this_time, &last_time, &time_diff);
   if (timertod(&time_diff) < MIN_CPU_POLL_INTERVAL) {
      goto output;
   }
   last_time = this_time;

   if (kinfo_get_sched_cputime(&cp_time)) {
      warn("kinfo_get_sched_cputime");
      return 0.0;
   }
   cp_diff.cp_user = cp_time.cp_user - cp_old.cp_user;
   cp_diff.cp_nice = cp_time.cp_nice - cp_old.cp_nice;
   cp_diff.cp_sys = cp_time.cp_sys - cp_old.cp_sys;
   cp_diff.cp_intr = cp_time.cp_intr - cp_old.cp_intr;
   cp_diff.cp_idle = cp_time.cp_idle - cp_old.cp_idle;
   total_change = cp_diff.cp_user + cp_diff.cp_nice + cp_diff.cp_sys
     + cp_diff.cp_sys + cp_diff.cp_intr + cp_diff.cp_idle;
   if (total_change == 0)
     total_change = 1;
   half_change = total_change >> 1;

output:
   switch (which) {
   case 0:
      return (cp_diff.cp_user * 100 + half_change) / total_change;
   case 1:
      return (cp_diff.cp_nice * 100 + half_change) / total_change;
   case 2:
      return (cp_diff.cp_sys * 100 + half_change) / total_change;
   case 3:
      return (cp_diff.cp_intr * 100 + half_change) / total_change;
   case 4:
      return (cp_diff.cp_idle * 100 + half_change) / total_change;
   default:
      return 0;
   }
}

g_val_t
cpu_user_func ( void )
{
   g_val_t val;

   val.f = (float) cpu_state(0);

   return val;
}

g_val_t
cpu_nice_func ( void )
{
   g_val_t val;

   val.f = (float) cpu_state(1);

   return val;
}

g_val_t 
cpu_system_func ( void )
{
   g_val_t val;

   val.f = (float) cpu_state(2);

   return val;
}

g_val_t 
cpu_idle_func ( void )
{
   g_val_t val;

   val.f = (float) cpu_state(4);

   return val;
}

/*
** FIXME - This metric is not valid on DragonFlyBSD.
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

   val.f = (float) cpu_state(3);

   return val;
}

/*
** FIXME - This metric is not valid on DragonFlyBSD.
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
   size_t len = 0;

   sysctlbyname("kern.proc.all", NULL, &len, NULL, 0);

   val.uint32 = (len / sizeof (struct kinfo_proc)); 

   return val;
}

/* FIXME */
g_val_t
proc_run_func( void )
{
   g_val_t val;

   val.uint32 = 0;

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
 * FIXME: DragonFlyBSD don't seem to report this anywhere.  It's actually
 * quite complicated as there is SysV shared memory, POSIX shared memory,
 * and mmap shared memory at a minimum.
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
	free(netvfslist);

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
		skipvfs = 1;
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
	int cnt, i;

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

	cnt = 0;
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

	if (cnt == 0)
		goto done;

	/*
	 * Count up the string lengths, we need a extra byte to hold
	 * the between entries ',' or the NUL at the end.
	 */
	for (i = 0; i < cnt; i++)
		slen = strlen(listptr[i]) + 1;
	/* Add 2 for initial "no". */
	slen += 2;

	if ((str = malloc(slen)) == NULL) {
		warnx("malloc failed");
		goto done;
	}

	str[0] = 'n';
	str[1] = 'o';
	for (i = 0, strptr = str + 2; i < cnt; i++, strptr++) {
		strcpy(strptr, listptr[i]);
		strptr += strlen(listptr[i]);
		*strptr = ',';
	}
	*strptr = '\0';

done:
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
