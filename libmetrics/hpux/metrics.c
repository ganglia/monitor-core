/*
 *  hpux.c - Ganglia monitor core gmond module for HP-UX
 *
 * Change log: 
 *
 *   15oct2002 - Martin Knoblauch <martin.knoblauch@mscsoftware.com>
 *               Original version.
 *
 *   04nov2002 - Jack Perdue <j-perdue@tamu.edu>
 *               Filled in some empty metrics.  Stole MTU code from linux.c.
 *               Replaced Martin's cpu_user/idle/sys/etc_func's with something
 *               a little cleaner.  Tried to put error checking on all system calls. 
 *
 *   18nov2002 - Martin Knoblauch <martin.knoblauch@mscsoftware.com>
 *               - Put back removal of system processes in proc_run_func()
 *               - Add cpu_intr_func, cpu_wait_func, cpu_ssys_func
 *
 *   27nov2002 - Martin Knoblauch <martin.knoblauch@mscsoftware.com>
 *               - Add mem_arm, mem_rm, mem_avm, mem_vm
 *               - Add standalone test
 *
 *   26may2004 - Martin Knoblauch <martin.knoblauch@mscsoftware.com>
 *               - Rename cpu_wait to cpu_wio for Linux/Solaris compatibility
 *               - Rename cpu_ssys to cpu_sintr for Linux compatibility
 *               - Remove WAIT from SYSTEM category
 *
 * Notes:
 *
 *  - requires use of -D_PSTAT64 on 64-bit (wide) HP-UX 11.0... building
 *    the core also requires -lpthreads but configure doesn't seem to pick
 *    it up, so you will probably want something like:
 *
 *      env CFLAGS=-D_PSTAT64 LDFLAGS=-lpthread ./configure --prefix=/opt/ganglia
 *
 *    on wide HP-UX and the same line without the CFLAGS for narrow HP-UX.
 *
 *  - don't know if it will work on HP-UX 10.20... it definitely will
 *    NOT work on versions of HP-UX before that
 *
 * #defines:
 *
 *  MOREPOSSIBLE:
 *              Wraps some interesting new metrics.
 *  SIMPLE_SYS:
 *              If defined, only account CP_SYS, otherwise account anything that
 *              does not fall into user/nice/idle
 *  SIMPLE_SYSPROC: 
 *              If defined, count all processes in "R" state for proc_run_func(). Otherwise
 *              remove "system/kelrnel" processes.
 *
 * To do:
 *
 *  - mem_buffers_func()
 *  - verify mem_cached_func() makes sense
 *  - decide if memory swap shold be counted
 */

#include <time.h>
#include <unistd.h>
#include <sys/dk.h>
#include <sys/pstat.h>
#include <sys/utsname.h>
#include "libmetrics.h"
#include "interface.h"

/* buffer and keep global some info */
struct pst_static staticinfo;
int cpu_num_func_cpu_num = 0;
long clk_ticks;

g_val_t cpu_func( int );  /* prototype to make metric_init() happy */

/*
 * This function is called only once by the gmond.  Use to 
 * initialize data structures, etc or just return SYNAPSE_SUCCESS;
 */
g_val_t
metric_init( void )
{
   g_val_t rval;
   struct pst_dynamic p;

   if( -1 == pstat_getstatic( &staticinfo, sizeof(struct pst_static), 1, 0))
      {
      err_ret("metric_init() got an error from pstat_getstatic()");
      rval.int32 = SYNAPSE_FAILURE;
      }
   else if( -1 == (clk_ticks = sysconf(_SC_CLK_TCK)))
      {
      err_ret("metric_init() got an error from sysconf()"); 
      rval.int32 = SYNAPSE_FAILURE;
      }
   /* get num cpus now because things depend on it */
   else if( -1 == pstat_getdynamic( &p, sizeof(struct pst_dynamic), 1, 0))
      {
      err_ret("metric_init() got an error from pstat_getdynamic()");
      cpu_num_func_cpu_num = 1; /* if we wanted something */
      rval.int32 = SYNAPSE_FAILURE; /* but we don't */
      }
   else 
      {
      cpu_num_func_cpu_num = p.psd_max_proc_cnt;
      rval.int32 = SYNAPSE_SUCCESS;
      }

   cpu_func(-1);  /* initialize first set of counts */
   sleep(5);      /* wait a little bit */
   cpu_func(-1);  /* initialze second set of counts */

   return rval;
}

g_val_t
boottime_func ( void )
{
   g_val_t val;

   val.uint32 = staticinfo.boot_time;

   return val;
}

g_val_t
cpu_num_func ( void )
{
   g_val_t val;

   val.uint16 = cpu_num_func_cpu_num;

   return val;
}

g_val_t
cpu_speed_func ( void )
{
   g_val_t val;
   struct pst_processor p;

   /* assume all CPUs the same speed as the first one (reasonable?) */
   if ( -1 != pstat_getprocessor(&p, sizeof(struct pst_processor), 1, 0)) 
      val.uint32 = (p.psp_iticksperclktick*clk_ticks/1000000);
   else
      {
      err_ret("cpu_speed_func() got an error from pstat_getprocessor()");
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
   struct utsname uts;

   if( -1 == uname(&uts))
      {
      err_ret("machine_type_func() got an error from uname()");
      strncpy( val.str, "unknown", MAX_G_STRING_SIZE);
      }
   else
      strncpy( val.str, uts.machine , MAX_G_STRING_SIZE );

   return val;
}

g_val_t
os_name_func ( void )
{
   g_val_t val;
   struct utsname uts;
  
   if( -1 == uname(&uts))
      {
      err_ret("os_name_func() got an error from uname()");
      strncpy( val.str, "HP-UX", MAX_G_STRING_SIZE);
      }
   else
      strncpy( val.str, uts.sysname , MAX_G_STRING_SIZE );

   return val;
}        

g_val_t
os_release_func ( void )
{
   g_val_t val;
   struct utsname uts;

   if( -1 == uname(&uts))
      {
      err_ret("os_release_func() got an error from uname()");
      strncpy( val.str, "unknown", MAX_G_STRING_SIZE);
      }
   else
      {
      strncpy( val.str, uts.release , MAX_G_STRING_SIZE );
      strcat ( val.str, "-");
      strncat( val.str, uts.version, MAX_G_STRING_SIZE );
      }
      
   return val;
}        

/*
 * A helper function to return the total number of cpu timespent
 */
unsigned long
#if !defined(_PSTAT64)
total_timespent_func ( uint32_t totals[])
#else
total_timespent_func ( uint64_t totals[])
#endif
{
   int i,j;
   struct pst_processor p;
#if !defined(_PSTAT64)
   uint32_t total_timespent = 0;
#else
   uint64_t total_timespent = 0L;
#endif

   for( j = 0; j < CPUSTATES; j++) 
      totals[j] = 0;

   for( i = 0; i < cpu_num_func_cpu_num; i++) 
      {
      if( -1 == pstat_getprocessor( &p, sizeof(struct pst_processor), 1, i))
         {
         err_ret("total_timespent_func() got an error from pstat_getprocessor()");
         return 0;
         }
      for( j = 0; j < CPUSTATES; j++) 
         totals[j] += p.psp_cpu_time[j];
      }

   for( j = 0; j < CPUSTATES; j++)
      { 
      total_timespent += totals[j];
      }

   return total_timespent; 
}   

#define CPU_THRESHOLD  60   /* how many seconds between collecting stats */

g_val_t
cpu_func ( int which ) /* see <sys/dk.h> for which... -1 means initialize counts */
{
   g_val_t val;
   int j, alltime=0;
   time_t stamp;
   static time_t laststamp = 0;
   double diff;
#if !defined(_PSTAT64)
   static uint32_t total_timespent, last_total_timespent,
                   timespent[CPUSTATES], last_timespent[CPUSTATES];
#else
   static uint64_t total_timespent, last_total_timespent,
                   timespent[CPUSTATES], last_timespent[CPUSTATES];
#endif

   if( which >= CPUSTATES)
      {
      alltime = 1;                /* stats since boot (???) */
      which   -= CPUSTATES; 
      }

   stamp = time(NULL); 
   /* don't call total_timespent_func() too often */ 
   if(  stamp - CPU_THRESHOLD > laststamp || which < 0) 
      {
      laststamp = stamp;
      last_total_timespent = total_timespent;
      for( j = 0; j < CPUSTATES; j++)
         last_timespent[j] = timespent[j];
      total_timespent = total_timespent_func(timespent);
      }

   val.f = 0.0;
   if( which >= 0)
      {
      if( alltime) /* percentage since systems has been up (???) */
         {
         if( total_timespent != 0) 
            val.f = (timespent[which]*100.0)/total_timespent;
         }
      else /* percentage in last time slice */
         {
         diff = total_timespent - last_total_timespent;
         if( diff != 0.0) 
            val.f = ((timespent[which] - last_timespent[which])/diff)*100;
         else
            debug_msg("cpu_func() - no difference in total_timespent %ld %ld", 
              (unsigned long) total_timespent, (unsigned long) last_total_timespent);
         }
      }
     
   if( val.f < 0.0) val.f = 0;  /* catch counter wraps (rare, not very necessary) */
   return val;
}

g_val_t
cpu_user_func ( void )
{
  return cpu_func(CP_USER);
}

g_val_t
cpu_nice_func ( void )
{
   return cpu_func(CP_NICE);
}

g_val_t 
cpu_system_func ( void )
{
#ifdef SIMPLE_SYS
   return cpu_func(CP_SYS);
#else /* use Martin's definition of everything else */
   g_val_t val, tval;
#define NUM_SYSSTATES	5

/*
 * Add CP_INTR, CP_SSYS to system time to satisfy the ganglia web-frontend.
 * CP_BLOCK and CP_SWAIT are supposed to be obsolete, but I just want to be sure :-)
 */
   int i, sysstates[NUM_SYSSTATES] = { CP_SYS, CP_BLOCK, CP_SWAIT, CP_INTR, CP_SSYS };

   val.f = 0.0;
   for( i = 0; i < NUM_SYSSTATES; ++i)
      {
        tval = cpu_func(sysstates[i]);
        val.f += tval.f;
      }
   return val;
#endif
}

g_val_t 
cpu_idle_func ( void )
{
   return cpu_func(CP_IDLE);
}

/* 
 * Time spent processing interrupts
 */
g_val_t 
cpu_intr_func ( void )
{
   return cpu_func(CP_INTR);
}

/*
 * Time spent waiting
 */
g_val_t 
cpu_wio_func ( void )
{
   return cpu_func(CP_WAIT);
}

/*
 * Time in kernel mode
 */
g_val_t 
cpu_sintr_func ( void )
{
   return cpu_func(CP_SSYS);
}

/*
 * FIXME?
 */
g_val_t 
cpu_aidle_func ( void )
{
   return cpu_func(CP_IDLE + CPUSTATES);
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

#ifdef MOREPOSSIBLE
g_val_t 
cpu_auser_func ( uint32_t i )
{
   return cpu_func(CP_USER + CPUSTATES);
}

g_val_t 
cpu_anice_func ( uint32_t i )
{
   return cpu_func(CP_NICE + CPUSTATES);
}
g_val_t 

cpu_asys_func ( uint32_t i )
{
   return cpu_func(CP_SYS + CPUSTATES);
}
#endif

enum {LOAD_1, LOAD_5, LOAD_15};

g_val_t
load_func ( int which)
{
   g_val_t val;
   struct pst_dynamic p;

   val.f = 0.0;
   if( -1 == pstat_getdynamic( &p, sizeof(struct pst_dynamic), 1, 0))
      {
      err_ret("load_func() got an error from pstat_getdynamic()");
      }
   else
      switch(which) 
         {
         case LOAD_1:  val.f = cpu_num_func_cpu_num * p.psd_avg_1_min;  break;
         case LOAD_5:  val.f = cpu_num_func_cpu_num * p.psd_avg_5_min;  break;
         case LOAD_15: val.f = cpu_num_func_cpu_num * p.psd_avg_15_min; break;
         default:
           err_sys("load_func() - invalid value for which = %d", which);
         }

   return val;
}

g_val_t
load_one_func ( void )
{
   return load_func(LOAD_1);
}

g_val_t
load_five_func ( void )
{
   return load_func(LOAD_5);
}

g_val_t
load_fifteen_func ( void )
{
   return load_func(LOAD_15);
}

/* NOTE: setting this to 80 or above screws everything up on 64-bit (wide) HP-UX... 
         I don't know why. - jkp 
*/ 
#define PROC_BUFFERSIZE	50 

g_val_t
proc_func( int pstate )
{
   g_val_t val;
   int i, got, cruft=0, index = 0;
   struct pst_status p[PROC_BUFFERSIZE];

   val.uint32 = 0;
   do {
      got = pstat_getproc( p, sizeof(struct pst_status), PROC_BUFFERSIZE, index);
      if( -1 == got) 
         {
         err_ret("proc_func() got an error from pstat_getproc(%d) - got:%d",index, got);
         val.uint32 = 0;
         return val;
         }

      if( !pstate)
         val.uint32 += got;  /* total */
      else
         for( i = 0; i < got; i++) 
            {
#if !defined(SIMPLE_SYSPROC)
/*
 * Compute "system cruft" for PS_RUN case, MKN
 */
            if ((p[i].pst_stat == PS_RUN) &&  /* Process in Run state */
                ((p[i].pst_ppid == 0) ||      /* Owned by Kernel */
                 ((p[i].pst_ppid == 1) && (p[i].pst_uid == 0)))) /* or owned by init, uid=0 */
                 cruft++;
#endif
            if(pstate == p[i].pst_stat) /* total by state */
               {
               ++val.uint32;
               }
            }
      if ( got) /* make sure we actually got something (we might have got 0) */
         index = p[got-1].pst_idx + 1;
   } while( got == PROC_BUFFERSIZE);

#if !defined(SIMPLE_SYSPROC)
   if ( pstate == PS_RUN ) val.uint32 -= cruft;
#endif

   return val;
}

g_val_t
proc_run_func( void )
{
   return proc_func(PS_RUN);
}

g_val_t
proc_total_func ( void )
{
   return proc_func(0);
}

#ifdef MOREPOSSIBLE
g_val_t
proc_sleep_func( void )
{
   return proc_func(PS_SLEEP);
}
  
g_val_t
proc_stop_func( void )
{
   return proc_func(PS_STOP);
}
#endif

g_val_t
mem_total_func ( void )
{
   g_val_t val;

   /* assumes page size is a multiple of 1024 */
   val.f = staticinfo.physical_memory * (staticinfo.page_size / 1024);

   return val;
}  

g_val_t
mem_free_func ( void )
{
   g_val_t val;
   struct pst_dynamic p;

   if( -1 == pstat_getdynamic( &p, sizeof(struct pst_dynamic), 1, 0))
      {
      err_ret("mem_free_func() got an error from pstat_getdynamic()");
      val.f = 0;
      }
   else
      {
      /* assumes page size is a multiple of 1024 */
      val.f = p.psd_free * (staticinfo.page_size / 1024); 
      }
   return val;
}

g_val_t
mem_rm_func ( void )
{
   g_val_t val;
   struct pst_dynamic p;

   if( -1 == pstat_getdynamic( &p, sizeof(struct pst_dynamic), 1, 0))
      {
      err_ret("mem_rm_func() got an error from pstat_getdynamic()");
      val.f = 0;
      }
   else
      {
      /* assumes page size is a multiple of 1024 */
      val.f = p.psd_rm * (staticinfo.page_size / 1024); 
      }
   return val;
}

g_val_t
mem_arm_func ( void )
{
   g_val_t val;
   struct pst_dynamic p;

   if( -1 == pstat_getdynamic( &p, sizeof(struct pst_dynamic), 1, 0))
      {
      err_ret("mem_arm_func() got an error from pstat_getdynamic()");
      val.f = 0;
      }
   else
      {
      /* assumes page size is a multiple of 1024 */
      val.f = p.psd_arm * (staticinfo.page_size / 1024); 
      }
   return val;
}

g_val_t
mem_vm_func ( void )
{
   g_val_t val;
   struct pst_dynamic p;

   if( -1 == pstat_getdynamic( &p, sizeof(struct pst_dynamic), 1, 0))
      {
      err_ret("mem_vm_func() got an error from pstat_getdynamic()");
      val.f = 0;
      }
   else
      {
      /* assumes page size is a multiple of 1024 */
      val.f = p.psd_vm * (staticinfo.page_size / 1024); 
      }
   return val;
}

g_val_t
mem_avm_func ( void )
{
   g_val_t val;
   struct pst_dynamic p;

   if( -1 == pstat_getdynamic( &p, sizeof(struct pst_dynamic), 1, 0))
      {
      err_ret("mem_avm_func() got an error from pstat_getdynamic()");
      val.f = 0;
      }
   else
      {
      /* assumes page size is a multiple of 1024 */
      val.f = p.psd_avm * (staticinfo.page_size / 1024); 
      }
   return val;
}

#define SHMEM_BUFFERSIZE	16
g_val_t
mem_shared_func ( void )
{
   g_val_t val;
   int i, got, index = 0;
   struct pst_shminfo p[SHMEM_BUFFERSIZE];

   val.f = 0;
   do {

      if( -1 == ( got = pstat_getshm(p,sizeof(struct pst_shminfo), SHMEM_BUFFERSIZE, index)))
         {
         err_ret("mem_shared_func() got an error from pstat_getshm()");
         return val;
         }

      for( i = 0; i < got; i++)
         {
            val.f += p[i].psh_segsz; /* size in bytes */
         }

      if( got) /* make sure we actually got something (we might have got 0) */
         index = p[got-1].psh_idx + 1;

   } while ( got == SHMEM_BUFFERSIZE);
   val.f /= 1024;  /* KB */ 
   return val;
}

g_val_t
mem_buffers_func ( void )
{
   g_val_t val;

   /* FIXME HPUX */
   val.f = 0;

   return val;
}

g_val_t
mem_cached_func ( void )
{
   g_val_t val;
   struct pst_dynamic p;

   if ( -1 == pstat_getdynamic(&p, sizeof(struct pst_dynamic), (size_t)1, 0)) 
      {
      err_ret("mem_cached_func() got an error from pstat_getdynamic()");
      val.f = 0;
      }
   else
      {
      /* This was Martin Knoblauch's solution... don't know about correctness - JKP */
      /* assumes page size is a multiple of 1024 */
      val.f = (staticinfo.physical_memory - p.psd_free - p.psd_rm) * (staticinfo.page_size / 1024);
      }

   return val;
}

enum {SWAP_FREE, SWAP_TOTAL};

#define THELONGWAY

#ifdef THELONGWAY

/* this way may provide more metrics on swap usage in the future (???) */

#define SWAP_BUFFERSIZE		8

g_val_t
swap_func( int which)
{
   g_val_t val;
   int i, got, kpp, index = 0;
   struct pst_swapinfo p[SWAP_BUFFERSIZE];

   /* assumes page size is a multiple of 1024 */
   kpp = staticinfo.page_size / 1024;
   val.f = 0;
   do {

      if( -1 == ( got = pstat_getswap(p,sizeof(struct pst_swapinfo), SWAP_BUFFERSIZE, index)))
         {
         err_ret("swap_func() got an error from pstat_getswap()");
         return val;
         }

      for( i = 0; i < got; i++) 
         {
         if( p[i].pss_flags & SW_ENABLED)
            {
            switch( which)
               {
               case SWAP_FREE:  
                  val.f += p[i].pss_nfpgs * kpp; 
                  break;
               case SWAP_TOTAL: 
                  if( p[i].pss_flags & SW_BLOCK)
                     val.f += p[i].pss_nblksenabled   /* * p[i].pss_swapchunk / 1024) */;
                  else if( p[i].pss_flags & SW_FS)
                     /* THIS IS A GUESS AND HAS NOT BEEN TESTED */
                     val.f += p[i].pss_allocated * p[i].pss_swapchunk / 1024;
                  else
                     {
                     err_msg("swap_func() - unknow swap type - pss_flags=0x%x", p[i].pss_flags);
                     }
                  break;
               default: 
                  err_sys("swap_func() - invalid value for which = %d", which);
               }
            }
         }

      if( got)  /* make sure we actually got something (we might have got 0) */
         index = p[got-1].pss_idx + 1;

   } while ( got == SWAP_BUFFERSIZE);

   return val;
}

#else  /* the short way */

/* #define SWAP_COUNTMEM to include memory reserved if swap fills up */

g_val_t
swap_func( int which)
{
   g_val_t val;
   struct pst_vminfo p;

   val.f = 0;
   if ( -1 == pstat_getvminfo(&p, sizeof(struct pst_vminfo), 1, 0))
      {
         err_ret("swap_func() got an error from pstat_getswap()");
      }
   else
      {
      switch( which)
         {
         case SWAP_FREE:
            /* assumes page size is a multiple of 1024 */
            val.f = p.psv_swapspc_cnt;
#ifdef SWAP_COUNTMEM
            val.f += p.psv_swapmem_cnt;
#endif
            val.f *= (staticinfo.page_size / 1024);
            break;
         case SWAP_TOTAL:
            /* assumes page size is a multiple of 1024 */
            val.f = p.psv_swapspc_max;
#ifdef SWAP_COUNTMEM
            val.f += p.psv_swapmem_max;
#endif
            val.f *= (staticinfo.page_size / 1024);
            break;
         default:
            err_sys("swap_func() - invalid value for which = %d", which);
         }
      }

   return val;
}

#endif

g_val_t
swap_free_func ( void )
{
  return swap_func(SWAP_FREE);
}

g_val_t
swap_total_func ( void )
{
  return swap_func(SWAP_TOTAL);
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

/* EOF - hpux.c */
