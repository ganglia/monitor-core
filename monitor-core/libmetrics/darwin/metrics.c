/*
 *  Stub file so that ganglia will build on Darwin (MacOS X)
 *  by Preston Smith <psmith@physics.purdue.edu>
 *  Mon Oct 14 14:18:01 EST 2002
 *
 *  Additional metrics by Glen Beane <beaneg@umcs.maine.edu> and 
 *  Eric Wages <wages@eece.maine.edu>. net/disk metrics
 *  gathering adapted from the freebsd file.
 *
 */

#include <stdlib.h>
#include "interface.h"
#include <kvm.h>
#include <sys/sysctl.h>

#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include <mach/mach_error.h>
#include <mach/mach_port.h>
#include <mach/mach_types.h>
#include <mach/message.h>
#include <mach/processor_set.h>
#include <mach/task.h>

#include <errno.h>
#include "libmetrics.h"

/* Added for byte/packets in/out */
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <sys/socket.h>
#include <stdio.h>

/* Added for disk statstics */
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>

#include <assert.h>

/* Function prototypes */

static float find_disk_space(double *total, double *tot_avail);
static size_t regetmntinfo(struct statfs **mntbufp, long mntsize, const char **vfslist);
static const char **makevfslist(char *fslist);
static int checkvfsname(const char *vfsname, const char **vfslist);
static char *makenetvfslist(void);

static void
get_netbw(double *in_bytes, double *out_bytes,
    double *in_pkts, double *out_pkts);

static uint64_t counterdiff(uint64_t, uint64_t, uint64_t, uint64_t);
 
static mach_port_t ganglia_port;
static unsigned long long pagesize;



/*
 * This function is called only once by the gmond.  Use to 
 * initialize data structures, etc or just return SYNAPSE_SUCCESS;
 */

g_val_t
metric_init(void)
{
   g_val_t val;

   vm_size_t page;

   /* initialize ganglia_port - all functions can use this instead of 
      calling mach_host_self every time */   
   ganglia_port = mach_host_self();
   
   /* save the get pagesize. we save this so it doesn't have to be looked up
      every time several of the metrics functions are called */
      
   (void)host_page_size(ganglia_port, &page);
 
   pagesize = (unsigned long long) page;
   
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
   size_t len;
   unsigned long cpu_speed;
   int mib[2];

   mib[0] = CTL_HW;
   mib[1] = HW_CPU_FREQ;
   len = sizeof (cpu_speed);

   sysctl(mib, 2, &cpu_speed, &len, NULL, 0);
   cpu_speed /= 1000000;         /* make MHz, it comes in Hz */

   val.uint32 = cpu_speed;

   return val;
}

g_val_t
mem_total_func ( void )
{
  g_val_t val;
#if HW_MEMSIZE
  unsigned long long physmem;
  size_t len = sizeof (physmem);
  int mib[2];

  mib[0] = CTL_HW;
  mib[1] = HW_MEMSIZE;

  sysctl(mib, 2, &physmem, &len, NULL, 0);

  val.f = physmem / 1024;
#else
  val.f = 0;
#endif
  return val;
} 

g_val_t
swap_total_func ( void )
{
   g_val_t val;
   val.uint32 = 0;

   return val;
}

g_val_t
boottime_func ( void )
{
   g_val_t val;

   struct timeval  boottime;
   int mib[2];
   size_t size;

   mib[0] = CTL_KERN;
   mib[1] = KERN_BOOTTIME;
   size = sizeof(boottime);
   if (sysctl(mib, 2, &boottime, &size, NULL, 0) == -1)
       val.uint32 = 0;

   val.uint32 = boottime.tv_sec;

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

   int mib[2];
   size_t len;
   char *prefix, buf[1024];

   prefix = "";

   mib[0] = CTL_HW;
   mib[1] = HW_MACHINE;
   len = sizeof(buf);
   if (sysctl(mib, 2, &buf, &len, NULL, 0) == -1)
        strncpy( val.str, "PowerPC", MAX_G_STRING_SIZE );

   strncpy( val.str, buf, MAX_G_STRING_SIZE );

   return val;
}

g_val_t
os_name_func ( void )
{
   g_val_t val;
   int mib[2];
   size_t len;
   char *prefix, buf[1024];

   prefix = "";

   mib[0] = CTL_KERN;
   mib[1] = KERN_OSTYPE;
   len = sizeof(buf);
   if (sysctl(mib, 2, &buf, &len, NULL, 0) == -1)
        strncpy( val.str, "Darwin", MAX_G_STRING_SIZE );

   strncpy( val.str, buf, MAX_G_STRING_SIZE );

   return val;
}        

g_val_t
os_release_func ( void )
{
   g_val_t val;
   int mib[2];
   size_t len;
   char *prefix, buf[1024];

   prefix = "";

   mib[0] = CTL_KERN;
   mib[1] = KERN_OSRELEASE;
   len = sizeof(buf);
   if (sysctl(mib, 2, &buf, &len, NULL, 0) == -1)
        strncpy( val.str, "Unknown", MAX_G_STRING_SIZE );

   strncpy( val.str, buf, MAX_G_STRING_SIZE );


   return val;
}        


g_val_t
cpu_user_func ( void )
{
   static unsigned long long last_userticks, userticks, last_totalticks, totalticks, diff;
   mach_msg_type_number_t count;
   host_cpu_load_info_data_t cpuStats;
   kern_return_t	ret;
   g_val_t val;
   
   count = HOST_CPU_LOAD_INFO_COUNT;
   ret = host_statistics(ganglia_port,HOST_CPU_LOAD_INFO,(host_info_t)&cpuStats,&count);
   
   if (ret != KERN_SUCCESS) {
     err_msg("cpu_user_func() got an error from host_statistics()");
     return val;
   }
   
   userticks = cpuStats.cpu_ticks[CPU_STATE_USER];
   totalticks = cpuStats.cpu_ticks[CPU_STATE_IDLE] + cpuStats.cpu_ticks[CPU_STATE_USER] +
         cpuStats.cpu_ticks[CPU_STATE_NICE] + cpuStats.cpu_ticks[CPU_STATE_SYSTEM];
   diff = userticks - last_userticks;
   
   if ( diff )
       val.f = ((float)diff/(float)(totalticks - last_totalticks))*100;
     else
       val.f = 0.0;
   
   debug_msg("cpu_user_func() returning value: %f\n", val.f);
   
     
   last_userticks = userticks;
   last_totalticks = totalticks;
   
   return val;
}

g_val_t
cpu_nice_func ( void )
{
   static unsigned long long last_niceticks, niceticks, last_totalticks, totalticks, diff;
   mach_msg_type_number_t count;
   host_cpu_load_info_data_t cpuStats;
   kern_return_t	ret;
   g_val_t val;
   
   count = HOST_CPU_LOAD_INFO_COUNT;
   ret = host_statistics(ganglia_port,HOST_CPU_LOAD_INFO,(host_info_t)&cpuStats,&count);
   
   if (ret != KERN_SUCCESS) {
     err_msg("cpu_nice_func() got an error from host_statistics()");
     return val;
   }
   
   niceticks = cpuStats.cpu_ticks[CPU_STATE_NICE];
   totalticks = cpuStats.cpu_ticks[CPU_STATE_IDLE] + cpuStats.cpu_ticks[CPU_STATE_USER] +
         cpuStats.cpu_ticks[CPU_STATE_NICE] + cpuStats.cpu_ticks[CPU_STATE_SYSTEM];
   diff = niceticks - last_niceticks;
   
   if ( diff )
       val.f = ((float)diff/(float)(totalticks - last_totalticks))*100;
     else
       val.f = 0.0;
   
   debug_msg("cpu_nice_func() returning value: %f\n", val.f);
   

   last_niceticks = niceticks;
   last_totalticks = totalticks;

   return val;
}

g_val_t 
cpu_system_func ( void )
{
   static unsigned long long last_systemticks, systemticks, last_totalticks, totalticks, diff;
   mach_msg_type_number_t count;
   host_cpu_load_info_data_t cpuStats;
   kern_return_t	ret;
   g_val_t val;
   
   count = HOST_CPU_LOAD_INFO_COUNT;
   ret = host_statistics(ganglia_port,HOST_CPU_LOAD_INFO,(host_info_t)&cpuStats,&count);
   
   if (ret != KERN_SUCCESS) {
     err_msg("cpu_system_func() got an error from host_statistics()");
     return val;
   }
   
   systemticks = cpuStats.cpu_ticks[CPU_STATE_SYSTEM];
   totalticks = cpuStats.cpu_ticks[CPU_STATE_IDLE] + cpuStats.cpu_ticks[CPU_STATE_USER] +
         cpuStats.cpu_ticks[CPU_STATE_NICE] + cpuStats.cpu_ticks[CPU_STATE_SYSTEM];
   diff = systemticks - last_systemticks;
   
   if ( diff )
       val.f = ((float)diff/(float)(totalticks - last_totalticks))*100;
     else
       val.f = 0.0;
   
   debug_msg("cpu_system_func() returning value: %f\n", val.f);
   
   
   last_systemticks = systemticks;
   last_totalticks = totalticks;

   return val;
}

g_val_t 
cpu_idle_func ( void )
{
   static unsigned long long last_idleticks, idleticks, last_totalticks, totalticks, diff;
   mach_msg_type_number_t count;
   host_cpu_load_info_data_t cpuStats;
   kern_return_t	ret;
   g_val_t val;
   
   count = HOST_CPU_LOAD_INFO_COUNT;
   ret = host_statistics(ganglia_port,HOST_CPU_LOAD_INFO,(host_info_t)&cpuStats,&count);
   
   if (ret != KERN_SUCCESS) {
     err_msg("cpu_idle_func() got an error from host_statistics()");
     return val;
   }
   
   idleticks = cpuStats.cpu_ticks[CPU_STATE_IDLE];
   totalticks = cpuStats.cpu_ticks[CPU_STATE_IDLE] + cpuStats.cpu_ticks[CPU_STATE_USER] +
         cpuStats.cpu_ticks[CPU_STATE_NICE] + cpuStats.cpu_ticks[CPU_STATE_SYSTEM];
   diff = idleticks - last_idleticks;
   
   if ( diff )
       val.f = ((float)diff/(float)(totalticks - last_totalticks))*100;
     else
       val.f = 0.0;
   
   debug_msg("cpu_idle_func() returning value: %f\n", val.f);
   
   
   last_idleticks = idleticks;
   last_totalticks = totalticks;
   
   return val;
}

g_val_t 
cpu_aidle_func ( void )
{
   static unsigned long long idleticks, totalticks;
   mach_msg_type_number_t count;
   host_cpu_load_info_data_t cpuStats;
   kern_return_t	ret;
   g_val_t val;
   
   count = HOST_CPU_LOAD_INFO_COUNT;
   ret = host_statistics(ganglia_port,HOST_CPU_LOAD_INFO,(host_info_t)&cpuStats,&count);
   
   if (ret != KERN_SUCCESS) {
     err_msg("cpu_aidle_func() got an error from host_statistics()");
     return val;
   }
   
   idleticks = cpuStats.cpu_ticks[CPU_STATE_IDLE];
   totalticks = cpuStats.cpu_ticks[CPU_STATE_IDLE] + cpuStats.cpu_ticks[CPU_STATE_USER] +
         cpuStats.cpu_ticks[CPU_STATE_NICE] + cpuStats.cpu_ticks[CPU_STATE_SYSTEM];
   
   val.f = ((double)idleticks/(double)totalticks)*100;
   
   debug_msg("cpu_aidle_func() returning value: %f\n", val.f);
   
   
   return val;
}

/*
** FIXME
*/
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


g_val_t 
bytes_in_func ( void )
{
   g_val_t val;
   double in_bytes;
   get_netbw(&in_bytes, NULL, NULL, NULL);

   val.f = (float)in_bytes;
   return val;
}


g_val_t 
bytes_out_func ( void )
{
   g_val_t val;
   double out_bytes;
   get_netbw(NULL, &out_bytes, NULL, NULL);

   val.f = (float)out_bytes;
   return val;
}


g_val_t 
pkts_in_func ( void )
{
   g_val_t val;
   double in_pkts;
   get_netbw(NULL, NULL, &in_pkts, NULL);

   val.f = (float)in_pkts;
   return val;
}


g_val_t 
pkts_out_func ( void )
{
   g_val_t val;
   double out_pkts;
   get_netbw(NULL, NULL, NULL, &out_pkts);

   val.f = (float)out_pkts;
   return val;
}


g_val_t 
disk_free_func ( void )
{
   double total_free=0.0;
   double total_size=0.0;
   g_val_t val;

   find_disk_space(&total_size, &total_free);

   val.d = total_free;
   return val;
}


g_val_t 
disk_total_func ( void )
{
   double total_free=0.0;
   double total_size=0.0;
   g_val_t val;

   find_disk_space(&total_size, &total_free);

   val.d = total_size;
   return val;
}


g_val_t 
part_max_used_func ( void )
{
   double total_free=0.0;
   double total_size=0.0;
   float most_full;
   g_val_t val;

   most_full = find_disk_space(&total_size, &total_free);

   val.f = most_full;
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
   int mib[3];
   size_t len;

   mib[0] = CTL_KERN;
   mib[1] = KERN_PROC;
   mib[2] = KERN_PROC_ALL;

   sysctl(mib, 3, NULL, &len, NULL, 0);

   val.uint32 = (len / sizeof (struct kinfo_proc)); 

   return val;


}


g_val_t
proc_run_func( void )
{

   /*  proc_run_func() - get number of running processes
    *
    *  Glen Beane, beaneg@umcs.maine.edu
    *
    *  At first I used sysctl and got a BSD style process listing, and 
    *  I counted up p_stat == SRUN,  but this counted practically every 
    *  process as running (say 80 running vs top reporting 3 running). 
    *  Now I get mach tasks, and Ilook at the state of all the threads 
    *  belonging to that task. If the task has any thread that is running,
    *  then I count the process as running. This seems to agree with top.
    *
    *  1/25/05
    *
    */


   g_val_t val;	/* function return value */
	
	
   kern_return_t            error;		/* return type of mach calls */
   mach_port_t              port;		/* mach port for host */
   mach_msg_type_number_t   count;
   processor_set_t          *psets, pset;
   task_t                   *tasks;
   task_info_t              a_task;
   task_basic_info_data_t   ti;
   thread_t*                threads;	/* pointer to a list of ports to all
											threads	on current task */
   struct thread_basic_info th_info;
	
	
   unsigned pcnt;       /* number of Mach processor sets */ 
   unsigned tcnt;       /* number of Mach tasks on a processor set */
   unsigned thcnt;      /* number of Mach threads in a task */
   unsigned procRun;    /* calculated number of running processes */
	
   char errmsg[192];    /* error string to pass to err_msg */

   int i,j,k;           /* loop counters */
	
	
	
	
	
	
   count = THREAD_BASIC_INFO_COUNT;

   procRun    = 0;
   val.uint32 = 0;
	
   tcnt = thcnt = pcnt = 0;

   error = host_processor_sets(ganglia_port, &psets, &pcnt);
   if(error != KERN_SUCCESS){
      sprintf(errmsg, "host_processor_sets: %s\n", mach_error_string(error));
      err_msg(errmsg);   
      goto ret;   
   }
	
	
   for (i = 0; i < pcnt; i++) {
      error = host_processor_set_priv(ganglia_port, psets[i], &pset);
      if (error != KERN_SUCCESS) {
         sprintf(errmsg, "host_processor_set_priv: %s\n", mach_error_string(error));
         err_msg(errmsg);
         goto ret;
      }

      error = processor_set_tasks(pset, &tasks, &tcnt);
      if (error != KERN_SUCCESS) {
           sprintf(errmsg, "processor_set_tasks: %s\n", mach_error_string(error));
           err_msg(errmsg);
           goto ret;
      }

		
      for(j = 0; j < tcnt; j++){
			
         error = task_threads(tasks[j], &threads, &thcnt);
         if(error != KERN_SUCCESS){
            sprintf(errmsg, "task_threads: %s\n", mach_error_string(error));
            err_msg(errmsg);
            goto ret;
         }
			
         for(k = 0; k<thcnt; k++){
				
            error = thread_info(threads[k], THREAD_BASIC_INFO, (thread_info_t)&th_info, &count);
            if(error != KERN_SUCCESS){
               sprintf(errmsg, "thread_info: %s\n", mach_error_string(error));
               err_msg(errmsg);
               goto ret;
            }
            
            if(th_info.run_state == 1){
               /* we found a running thread in this task. Increment procRun,
                  and go on to the next task */
               procRun++;
               break;
            }
				
				
         }
         
         /* done with this task, deallocate thread ports and thread list for
            this task */
            
         /* deallocate thread ports */
         for (k = 0; k < thcnt; k++) {
            mach_port_deallocate(mach_task_self(), threads[k]);
         }         
         /* Deallocate the thread list */
         vm_deallocate(mach_task_self(), (vm_address_t) threads, sizeof(threads) * thcnt);
         thcnt = 0;
      }
      
      /* done with this Mach processor set. deallocate task ports and task list
         for this processor set */
         
      /* deallocate ports */
      for(k=0; k < tcnt; k++){
         mach_port_deallocate(mach_task_self(), tasks[k]);
      }
      /* deallocate task list */
      vm_deallocate(mach_task_self(), (vm_address_t)tasks, sizeof(tasks)*tcnt);
      tcnt = 0;
      
      
   }
   
   val.uint32 = procRun;

ret:	

   /* deallocate psets */
   for(k=0; k < pcnt; k++){
      mach_port_deallocate(mach_task_self(), psets[k]);
   }
   vm_deallocate(mach_task_self(), (vm_address_t)psets, sizeof(psets)*pcnt);


   /* if we got here on an error condition we might need to deallocate threads */
   if(thcnt != 0){
      for (i = 0; i < thcnt; i++) {
         mach_port_deallocate(mach_task_self(), threads[i]);
      }
	
      /* Deallocate the thread list */
      vm_deallocate(mach_task_self(), (vm_address_t) threads, sizeof(threads)*thcnt);
   }
	
   /* if we got here on an error condition we might need to deallocate tasks */
   if(tcnt != 0){
      for (i = 0; i < tcnt; i++) {
         mach_port_deallocate(mach_task_self(), tasks[i]);
   }
	
      /* Deallocate the task list */
      vm_deallocate(mach_task_self(), (vm_address_t)tasks, sizeof(tasks)*tcnt);
   }


	
   return val;
  
}



g_val_t
mem_free_func ( void )
{
   g_val_t val;

   vm_statistics_data_t vm_stat;
   int host_count;
   unsigned long long free_size;
   host_t host_port;


   host_count = sizeof(vm_stat)/sizeof(integer_t);

   host_statistics(ganglia_port, HOST_VM_INFO,
                        (host_info_t)&vm_stat, &host_count);

   free_size = (unsigned long long) vm_stat.free_count   * pagesize;
   val.f = free_size / 1024;

   return val;
}


/*
 * FIXME
 */
g_val_t
mem_shared_func ( void )
{
   g_val_t val;

   val.f = 0;
   return val;
}

/*
 * FIXME
 */
g_val_t
mem_buffers_func ( void )
{
   g_val_t val;
   val.f = 0;
   return val;
}

/*
 * FIXME
 */
g_val_t
mem_cached_func ( void )
{
   g_val_t val;
   val.f = 0;
   return val;
}

/*
 * FIXME
 */
g_val_t
swap_free_func ( void )
{
   g_val_t val;
   val.f = 0;
   return val;
}

g_val_t
mtu_func ( void )
{
   /* We want to find the minimum MTU (Max packet size) over all UP interfaces.*/
   unsigned int min=0;
   g_val_t val;
   val.uint32 = get_min_mtu();
   /* A val of 0 means there are no UP interfaces. Shouldn't happen. */
   return val;
}




/*  internal functions */




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

/*
 * functions, constants, structs, and macros used for bytes/packets in/out
 *  adapted from the freebsd metrics.c by Glen Beane <beaneg@umcs.maine.edu>
 *
 */

#define timertod(tvp) \
    ((double)(tvp)->tv_sec + (double)(tvp)->tv_usec/(1000*1000))

struct traffic {
        uint64_t in_bytes;
        uint64_t out_bytes;
        uint64_t in_pkts;
        uint64_t out_pkts;
};

#ifndef MIN_NET_POLL_INTERVAL
#define MIN_NET_POLL_INTERVAL 0.5
#endif

static void
get_netbw(double *in_bytes, double *out_bytes,
    double *in_pkts, double *out_pkts)
{
#ifdef NETBW_DEBUG
        char            name[IFNAMSIZ];
#endif
        struct          if_msghdr *ifm, *nextifm;
        struct          sockaddr_dl *sdl;
        char            *buf, *lim, *next;
        size_t          needed;
        int             mib[6];
        int             i;
        int             index;
        static double   ibytes, obytes, ipkts, opkts;
        struct timeval  this_time;
        struct timeval  time_diff;
        struct traffic  traffic;
        static struct timeval last_time = {0,0};
        static int      indexes = 0;
        static int      *seen = NULL;
        static struct traffic *lastcount = NULL;
        static double   o_ibytes, o_obytes, o_ipkts, o_opkts;

        ibytes = obytes = ipkts = opkts = 0.0;

        mib[0] = CTL_NET;
        mib[1] = PF_ROUTE;
        mib[2] = 0;
        mib[3] = 0;                     /* address family */
        mib[4] = NET_RT_IFLIST;
        mib[5] = 0;             /* interface index */

        gettimeofday(&this_time, NULL);
        timersub(&this_time, &last_time, &time_diff);
        if (timertod(&time_diff) < MIN_NET_POLL_INTERVAL) {
                goto output;
        }


        if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0)
                errx(1, "iflist-sysctl-estimate");
        if ((buf = (char*)malloc(needed)) == NULL)
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
                        seen = (int*)realloc(seen, sizeof(*seen)*(index+1));
                        lastcount = (struct traffic*)realloc(lastcount,
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

/* functions used for reporting disk metrics.  Adapted from freebsd metrics
 * (probably originally from FreeBSD du)
 *
 */

#define MNT_IGNORE 0

static int        skipvfs;

static float
find_disk_space(double *total, double *tot_avail)
{

   struct statfs *mntbuf;
   /* const char *fstype; */
   const char **vfslist;
   char *str;
   size_t i, mntsize;
   size_t used, availblks;
   const double reported_units = 1e9;
   double toru;
   float pct;
   float most_full = 0.0;
   
   *total = 0.0;
   *tot_avail = 0.0;
   
   /* fstype = "ufs"; */
   
   /* fix memory leak 
   vfslist = makevfslist(makenetvfslist()); */
   
   str = makenetvfslist();
   vfslist = makevfslist(str);
   free(str);
   
   
   mntsize = getmntinfo(&mntbuf, MNT_NOWAIT);   
   mntsize = regetmntinfo(&mntbuf, mntsize, vfslist);
   
   for (i = 0; i < mntsize; i++) {
      if ((mntbuf[i].f_flags & MNT_IGNORE) == 0) {
         used = mntbuf[i].f_blocks - mntbuf[i].f_bfree;
         availblks = mntbuf[i].f_bavail + used;
         pct = (availblks == 0 ? 100.0 : (double)used / (double)availblks * 100.0);
         if (pct > most_full)
            most_full = pct;
         toru = reported_units/mntbuf[i].f_bsize;
         *total += mntbuf[i].f_blocks / toru;
         *tot_avail += mntbuf[i].f_bavail / toru;
      }
   }

   free(vfslist);
   return most_full;

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
        if ((av = (const char**)malloc((size_t)(i + 2) * sizeof(char *))) == NULL) {
                err_msg("malloc failed\n");
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

 char *
makenetvfslist(void)
{
        char *str, *strptr, **listptr;

        int mib[4], maxvfsconf, cnt=0, i;
        size_t miblen;
        struct vfsconf vfc;

        mib[0] = CTL_VFS; mib[1] = VFS_GENERIC; mib[2] = VFS_MAXTYPENUM;
        miblen=sizeof(maxvfsconf);
        if (sysctl(mib, 3, &maxvfsconf, &miblen, NULL, 0)) {
                warnx("sysctl failed"); 
                return (NULL);
        }

        if ((listptr = (char**)malloc(sizeof(char*)*maxvfsconf)) == NULL) {
                warnx("malloc failed");
                return (NULL);
        }

        miblen = sizeof (struct vfsconf);
        mib[2] = VFS_CONF;
        for (i = 0; i < maxvfsconf; i++) {
                mib[3] = i;
                if (sysctl(mib, 4, &vfc, &miblen, NULL, 0) == 0) {
                        if (!(vfc.vfc_flags & MNT_LOCAL)) {
                                listptr[cnt++] = strdup(vfc.vfc_name);
                                if (listptr[cnt-1] == NULL) {
                                        warnx("malloc failed");
                                        return (NULL);
                                }
                        }
                }
        }

        if (cnt == 0 ||
            (str = (char*)malloc(sizeof(char) * (32 * cnt + cnt + 2))) == NULL) {
                if (cnt > 0)
                        warnx("malloc failed");
                free(listptr);
                return (NULL);
        }

        *str = 'n'; *(str + 1) = 'o';
        for (i = 0, strptr = str + 2; i < cnt; i++, strptr++) {
                strncpy(strptr, listptr[i], 32);
                strptr += strlen(listptr[i]); 
                *strptr = ','; 
                free(listptr[i]);
        }
        *(--strptr) = NULL;
        free(listptr);
        return (str);
}


