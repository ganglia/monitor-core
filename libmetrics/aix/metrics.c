/*
 *  Rewrite of AIX metrics using libperfstat API
 *
 *  Libperfstat can deal with a 32-bit and a 64-bit Kernel and does not require root authority.
 *
 *  The code is tested with AIX 5.2 (32Bit- and 64Bit-Kernel), but 5.1 and 5.3 shoud be OK too
 *
 *  by Andreas Schoenfeld, TU Darmstadt, Germany (4/2005) 
 *  E-Mail: Schoenfeld@hrz.tu-darmstadt.de
 *
 *  Its based on the 
 *  First stab at support for metrics in AIX
 *  by Preston Smith <psmith@physics.purdue.edu>
 *  Wed Feb 27 14:55:33 EST 2002
 *
 *  AIX V5 support, bugfixes added by Davide Tacchella <tack@cscs.ch>
 *  May 10, 2002
 *
 *  you may still find some code (like "int bos_level(..)"  ) and the basic structure of this version. 
 *
 *  Some code fragments of the network statistics are "borowed" from  
 *  the Solaris Metrics   
 *
 *  Fix proc_total, proc_run, swap_free and swap_total. Implement mem_cached (MKN, 16-Jan-2006)
 *
 */

#include "interface.h"
#include <stdlib.h>
#include <utmp.h>
#include <stdio.h>
#include <procinfo.h>
#include <strings.h>
#include <signal.h>
#include <odmi.h>
#include <cf.h>
#include <sys/utsname.h>
#include <sys/proc.h>
#include <sys/types.h>
#include <time.h>

#include <libperfstat.h>

#include "libmetrics.h"




struct Class *My_CLASS;

struct product {
        char filler[12];
        char lpp_name[145];      /* offset: 0xc ( 12) */
        char comp_id[20];        /* offset: 0x9d ( 157) */
        short update;            /* offset: 0xb2 ( 178) */
        long cp_flag;            /* offset: 0xb4 ( 180) */
        char fesn[10];           /* offset: 0xb8 ( 184) */
        char *name;              /*[42] offset: 0xc4 ( 196) */
        short state;             /* offset: 0xc8 ( 200) */
        short ver;               /* offset: 0xca ( 202) */
        short rel;               /* offset: 0xcc ( 204) */
        short mod;               /* offset: 0xce ( 206) */
        short fix;               /* offset: 0xd0 ( 208) */
        char ptf[10];            /* offset: 0xd2 ( 210) */
        short media;             /* offset: 0xdc ( 220) */
        char sceded_by[10];      /* offset: 0xde ( 222) */
        char *fixinfo;           /* [1024] offset: 0xe8 ( 232) */
        char *prereq;            /* [1024] offset: 0xec ( 236) */
        char *description;       /* [1024] offset: 0xf0 ( 240) */
        char *supersedes;        /* [512] offset: 0xf4 ( 244) */
};

#if defined(_AIX43)
#ifndef SBITS
/*
 * For multiplication of fractions that are stored as integers, including
 * p_pctcpu.  Not allowed to do floating point arithmetic in the kernel.
 */
#define SBITS   16
#endif
#endif

#define MAX_CPUS  64

#define INFO_TIMEOUT   10
#define CPU_INFO_TIMEOUT INFO_TIMEOUT

#define MEM_KB_PER_PAGE (4096/1024)


struct cpu_info {
  time_t timestamp;  
  u_longlong_t total_ticks;
  u_longlong_t user;        /*  raw total number of clock ticks spent in user mode */
  u_longlong_t sys;         /* raw total number of clock ticks spent in system mode */
  u_longlong_t idle;        /* raw total number of clock ticks spent idle */
  u_longlong_t wait;        /* raw total number of clock ticks spent waiting for I/O */
};

struct net_stat{
  double ipackets;
  double opackets;
  double ibytes;
  double obytes;
} cur_net_stat;






int ci_flag=0;
int ni_flag=0;

perfstat_cpu_total_t cpu_total_buffer;
perfstat_netinterface_total_t ninfo[2],*last_ninfo, *cur_ninfo ;


struct cpu_info cpu_info[2], 
  *last_cpu_info,
  *cur_cpu_info;


  

int aixver, aixrel, aixlev, aixfix;
static time_t boottime;

static int isVIOserver;

/* Prototypes
 */
void  update_ifdata(void);
void get_cpuinfo(void);
int bos_level(int *aix_version, int *aix_release, int *aix_level, int *aix_fix);





/*
 * This function is called only once by the gmond.  Use to 
 * initialize data structures, etc or just return SYNAPSE_SUCCESS;
 */
g_val_t
metric_init(void)
{
   g_val_t val;
   FILE *f;


/* find out if we are running on a VIO server */

   f = fopen( "/usr/ios/cli/ioscli", "r" );

   if (f)
   {
      isVIOserver = 1;
      fclose( f );
   }
   else
      isVIOserver = 0;


   last_cpu_info = &cpu_info[ci_flag];
   ci_flag^=1;
   cur_cpu_info  = &cpu_info[ci_flag];
   cur_cpu_info->total_ticks = 0;
   
   update_ifdata();
   
   get_cpuinfo();
   sleep(CPU_INFO_TIMEOUT+1);
   get_cpuinfo();

   update_ifdata();

   bos_level(&aixver, &aixrel, &aixlev, &aixfix);

   val.int32 = SYNAPSE_SUCCESS;
   return val;
}

g_val_t
cpu_speed_func ( void )
{
   g_val_t val;
   perfstat_cpu_total_t c;

   if (perfstat_cpu_total(NULL,  &c, sizeof(perfstat_cpu_total_t), 1) == -1)
      val.uint32 = 0;
   else
      val.uint32 = c.processorHZ/1000000;

   return val;
}

g_val_t
boottime_func ( void )
{
   g_val_t val;
   struct utmp buf;
   FILE *utmp;
   
   if (!boottime) {
      utmp = fopen(UTMP_FILE, "r");

      if (utmp == NULL) {
         /* Can't open utmp, use current time as boottime */
         boottime = time(NULL);
      } else {
         while (fread((char *) &buf, sizeof(buf), 1, utmp) == 1) {
            if (buf.ut_type == BOOT_TIME) {
               boottime = buf.ut_time;
               break;
            }
         }
	 fclose (utmp);
      }
   }
   val.uint32 = boottime;

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
   perfstat_cpu_total_t c;

   if (perfstat_cpu_total (NULL, &c, sizeof( perfstat_cpu_total_t), 1) == -1)
      strcpy (val.str, "unknown");
   else
      strncpy( val.str, c.description, MAX_G_STRING_SIZE );

   return val;
}

g_val_t
os_name_func ( void )
{
   g_val_t val;
   struct utsname uts;

   if (isVIOserver)
      strcpy( val.str, "Virtual I/O Server" );
   else
   {
      uname( &uts );
      strncpy( val.str, uts.sysname, MAX_G_STRING_SIZE );
   }

   return val;
}        


g_val_t
os_release_func ( void )
{
   g_val_t val;
   char oslevel[MAX_G_STRING_SIZE];

   sprintf(oslevel, "%d.%d.%d.%d", aixver, aixrel, aixlev, aixfix);
   strncpy( val.str, oslevel, MAX_G_STRING_SIZE );

   return val;
}        


/* AIX  defines
   CPU_IDLE, CPU_USER, CPU_SYS(CPU_KERNEL), CPU_WAIT
   so no metrics for cpu_nice, or cpu_aidle
*/


#define CALC_CPUINFO(type) ((100.0*(cur_cpu_info->type - last_cpu_info->type))/(1.0*(cur_cpu_info->total_ticks - last_cpu_info->total_ticks)))

g_val_t
cpu_user_func ( void )
{
   g_val_t val;
   
   
   get_cpuinfo();
   
   val.f = CALC_CPUINFO(user);

   if(val.f < 0) val.f = 0.0;
   return val;
}


/*
** AIX does not have this
** FIXME -- 
*/
g_val_t
cpu_nice_func ( void )
{
   g_val_t val;
   val.f = 0;
   return val;
}

g_val_t 
cpu_system_func ( void )
{
   g_val_t val;

   get_cpuinfo();
   val.f = CALC_CPUINFO(sys) ;
   if(val.f < 0) val.f = 0.0;
   return val;
}
g_val_t 

cpu_wio_func ( void )
{
   g_val_t val;
   
   get_cpuinfo();
   val.f = CALC_CPUINFO(wait);


   if(val.f < 0) val.f = 0.0;
   return val;
}

g_val_t 
cpu_idle_func ( void )
{
   g_val_t val;


   get_cpuinfo();
   val.f = CALC_CPUINFO(idle);


   if(val.f < 0) val.f = 0.0;
   return val;
}

/*
** AIX does not have this
** FIXME -- 
*/
g_val_t 
cpu_aidle_func ( void )
{
   g_val_t val;
   val.f = 0.0;
   return val;
}

/*
** Don't know what it is 
** FIXME -- 
*/
g_val_t 
cpu_intr_func ( void )
{
   g_val_t val;
   val.f = 0.0;
   return val;
}

/* Don't know what it is 
** FIXME -- 
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
   update_ifdata();
   val.f = cur_net_stat.ibytes;
   return val;
}


g_val_t 
bytes_out_func ( void )
{
   g_val_t val;

   update_ifdata();
   val.f = cur_net_stat.obytes;
   
   return val;
}


g_val_t 
pkts_in_func ( void )
{
   g_val_t val;

   update_ifdata();
   val.f = cur_net_stat.ipackets;

   return val;
}


g_val_t 
pkts_out_func ( void )
{
   g_val_t val;


   update_ifdata();
   val.f = cur_net_stat.opackets;

   return val;
}

g_val_t 
disk_free_func ( void )
{
   g_val_t val;
   perfstat_disk_total_t d;
   
   if (perfstat_disk_total(NULL, &d, sizeof(perfstat_disk_total_t), 1) == -1)
      val.d = 0.0;
   else
      val.d = (double)d.free / 1024;

   return val;
}

g_val_t 
disk_total_func ( void )
{
   g_val_t val;
   perfstat_disk_total_t d;

   if (perfstat_disk_total(NULL, &d, sizeof(perfstat_disk_total_t), 1) == -1)
      val.d = 0.0;
   else
      val.d = (double)d.size / 1024;

   return val;
}

/* FIXME */
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
   perfstat_cpu_total_t c;
   
   if (perfstat_cpu_total(NULL,  &c, sizeof(perfstat_cpu_total_t), 1) == -1)
      val.f = 0.0;
   else
      val.f = (float)c.loadavg[0]/(float)(1<<SBITS);
   return val;
}

g_val_t
load_five_func ( void )
{  
   g_val_t val;
   perfstat_cpu_total_t c;

   if (perfstat_cpu_total(NULL,  &c, sizeof(perfstat_cpu_total_t), 1) == -1)
      val.f = 0.0;
   else
      val.f = (float)c.loadavg[1]/(float)(1<<SBITS);
   return val;
}

g_val_t
load_fifteen_func ( void )
{
   g_val_t val;
   perfstat_cpu_total_t c;

   if (perfstat_cpu_total(NULL, &c, sizeof(perfstat_cpu_total_t), 1) == -1)
      val.f = 0.0;
   else
      val.f = (float)c.loadavg[2]/(float)(1<<SBITS);

   return val;
}

g_val_t
cpu_num_func ( void )
{
   g_val_t val;
   perfstat_cpu_total_t c;

   if (perfstat_cpu_total(NULL,  &c, sizeof(perfstat_cpu_total_t), 1) == -1)
      val.uint16 = 0;
   else
      val.uint16 = c.ncpus;

   return val;
}

#define MAXPROCS 20

#if !defined(_AIX61)
/*
** These missing prototypes have caused me an afternoon of real grief !!!
*/
int getprocs64 (struct procentry64 *ProcessBuffer, int ProcessSize, 
                struct fdsinfo64 *FileBuffer, int FileSize,
                pid_t *IndexPointer, int Count);
int getthrds64 (pid_t ProcessIdentifier, struct thrdentry64 *ThreadBuffer,
            int  ThreadSize, tid64_t *IndexPointer, int Count);
#endif

/*
** count_threads(pid) finds all runnable threads belonging to
** process==pid. We do not count threads with the TFUNNELLED
** flag set, as they do not seem to count against the load
** averages (pure WOODOO, also known as "heuristics" :-)
*/
int count_threads(pid_t pid) {

  struct thrdentry64 ThreadsBuffer[MAXPROCS];
  tid64_t IndexPointer = 0;
  int stat_val;
  int nth = 0; 
  int i;
  
  while ((stat_val = getthrds64(pid,
			      ThreadsBuffer,
			      sizeof(struct thrdentry64),
			      &IndexPointer,
			      MAXPROCS )) > 0 ) 
    
    {
      for ( i=0; i<stat_val; i++) {
        /*
        ** Do not count FUNNELED threads, as they do not seem to
        ** be counted in loadavg.
        */
        if (ThreadsBuffer[i].ti_flag & TFUNNELLED) continue;
	if(ThreadsBuffer[i].ti_state == TSRUN) {
          /*fprintf(stderr,"i=%d pid=%d tid=%lld state=%x flags=%x\n",i,ThreadsBuffer[i].ti_pid,
			ThreadsBuffer[i].ti_tid,ThreadsBuffer[i].ti_state,
			ThreadsBuffer[i].ti_flag);*/
          nth++;
        }
      }
      if (stat_val < MAXPROCS) break;
    }       
  return nth;
}

/*
** count_procs() computes the number of processes as shown in "ps -A".
**    Pass 0 as flag if you want to get a list of all processes.
**    Pass 1 if you want to get a list of all processes with
**    runnable threads.
*/
int count_procs(int flag) {

  struct procentry64 ProcessBuffer[MAXPROCS];
  pid_t IndexPointer = 0;
  int np = 0; 
  int stat_val;
  int i;
  
  while ((stat_val = getprocs64(
			      &ProcessBuffer[0],
			      sizeof(struct procentry64),
			      NULL,
			      sizeof(struct fdsinfo64),
			      &IndexPointer,
			      MAXPROCS )) > 0 ) 
    
    {
      for ( i=0; i<stat_val; i++) {
        if(flag != 0) {
          /*fprintf(stderr,"i=%d pid=%d state=%x flags=%x flags2=%x thcount=%d\n",i,
				ProcessBuffer[i].pi_pid,ProcessBuffer[i].pi_state,
				ProcessBuffer[i].pi_flags,ProcessBuffer[i].pi_flags2,
				ProcessBuffer[i].pi_thcount);*/
	  np += count_threads(ProcessBuffer[i].pi_pid);
        }
        else {
          np++;
        }
      }
      if (stat_val < MAXPROCS) break;
    }       
	
/*
** Reduce by one to make proc_run more Linux "compliant".
*/
  if ((flag != 0) && (np > 0)) np--;

  return np;
}


g_val_t
proc_total_func ( void )
{
  g_val_t foo;
 
  foo.uint32 = count_procs(0);
 
  return foo;
}


g_val_t
proc_run_func( void )
{
  g_val_t val;

  val.uint32 = count_procs(1);
 
  return val;
}

g_val_t
mem_total_func ( void )
{
   g_val_t val;
   perfstat_memory_total_t m;
  
   if (perfstat_memory_total(NULL, &m, sizeof(perfstat_memory_total_t), 1) == -1)
      val.f = 0;
   else
      val.f = m.real_total * MEM_KB_PER_PAGE;
   
   return val;
}

g_val_t
mem_free_func ( void )
{
   g_val_t val;
   perfstat_memory_total_t m;
  
   if (perfstat_memory_total(NULL, &m, sizeof(perfstat_memory_total_t), 1) == -1)
      val.f = 0;
   else
      val.f = m.real_free * MEM_KB_PER_PAGE;
   
   return val;
}

/* FIXME? */
g_val_t
mem_shared_func ( void )
{
   g_val_t val;
   
   val.f = 0;

   return val;
}

/* FIXME? */
g_val_t
mem_buffers_func ( void )
{
   g_val_t val;
   
   val.f = 0;

   return val;
}

g_val_t
mem_cached_func ( void )
{
   g_val_t val;
   perfstat_memory_total_t m;

   if (perfstat_memory_total(NULL, &m, sizeof(perfstat_memory_total_t), 1) == -1)
      val.f = 0;
   else
      val.f = m.numperm * MEM_KB_PER_PAGE;

   return val;
}

g_val_t
swap_total_func ( void )
{
   g_val_t val;
   perfstat_memory_total_t m;

   if (perfstat_memory_total(NULL, &m, sizeof(perfstat_memory_total_t), 1) == -1)
      val.f = 0;
   else
      val.f = m.pgsp_total * MEM_KB_PER_PAGE;
   
   return val;
   
}

g_val_t
swap_free_func ( void )
{
   g_val_t val;
   perfstat_memory_total_t m;

   if (perfstat_memory_total(NULL, &m, sizeof(perfstat_memory_total_t), 1) == -1)
      val.f = 0;
   else
      val.f =m.pgsp_free * MEM_KB_PER_PAGE;

   return val;
}

g_val_t
mtu_func ( void )
{
   /* We want to find the minimum MTU (Max packet size) over all UP interfaces.
*/
   unsigned int min=0;
   g_val_t val;
   val.uint32 = get_min_mtu();
   /* A val of 0 means there are no UP interfaces. Shouldn't happen. */
   return val;
}








void get_cpuinfo() 
{
  u_longlong_t cpu_total;
  time_t new_time;


  new_time = time(NULL);

  if (new_time - CPU_INFO_TIMEOUT > cur_cpu_info->timestamp ) 
    {

      perfstat_cpu_total(NULL,  &cpu_total_buffer, sizeof(perfstat_cpu_total_t), 1);


      cpu_total = cpu_total_buffer.user +  cpu_total_buffer.sys  
	+  cpu_total_buffer.idle +  cpu_total_buffer.wait;

  
      last_cpu_info=&cpu_info[ci_flag];
      ci_flag^=1; 
      cur_cpu_info=&cpu_info[ci_flag];

      cur_cpu_info->timestamp   = new_time;
      cur_cpu_info->total_ticks = cpu_total;
      cur_cpu_info->user        = cpu_total_buffer.user;
      cur_cpu_info->sys         = cpu_total_buffer.sys;
      cur_cpu_info->idle        = cpu_total_buffer.idle;
      cur_cpu_info->wait        = cpu_total_buffer.wait;
    }
} /*      get_cpuinfo  */
  


/* int bos_level(int *aix_version, int *aix_release, int *aix_level, int *aix_fix)
 *  is copied form 
 *
 *  First stab at support for metrics in AIX
 *  by Preston Smith <psmith@physics.purdue.edu>
 *  Wed Feb 27 14:55:33 EST 2002
 *
 *  AIX V5 support, bugfixes added by Davide Tacchella <tack@cscs.ch>
 *  May 10, 2002
 *
 */

int bos_level(int *aix_version, int *aix_release, int *aix_level, int *aix_fix)
{
    struct Class *my_cl;   /* customized devices class ptr */
    struct product  productobj;     /* customized device object storage */
    int rc, getit, found = 0;
    char *path;

    /*
     * start up odm
     */
    if (odm_initialize() == -1)
        return E_ODMINIT; 

    /*
     * Make sure we take the right database
     */
    if ((path = odm_set_path("/usr/lib/objrepos")) == (char *) -1)
        return odmerrno;
 
    /*
     * Mount the lpp class
     */
    if ((My_CLASS = odm_mount_class("product")) == (CLASS_SYMBOL) -1)
        return odmerrno;

    /*
     * open customized devices object class
     */
    if ((int)(my_cl = odm_open_class(My_CLASS)) == -1)
        return E_ODMOPEN;

    /*
     * Loop trough all entries for the lpp name, ASSUMING the last
     * one denotes the up to date number!!!
     */
    /*
     * AIX > 4.2 uses bos.mp or bos.up
     * AIX >= 6.1 uses bos.mp64
     */
    getit = ODM_FIRST;
    while ((rc = (int)odm_get_obj(my_cl, "name like bos.?p*",
                                  &productobj, getit)) != 0) {
        getit = ODM_NEXT;
        if (rc == -1) {
            /* ODM failure */
            break;
        }
        else {
            *aix_version = productobj.ver;
            *aix_release = productobj.rel;
            *aix_level   = productobj.mod;
            *aix_fix     = productobj.fix;
            found++;
        }
    }
    /*
     * AIX < 4.2 uses bos.rte.mp or bos.rte.up
     */
    if (!found) {
        getit = ODM_FIRST;
        while ((rc = (int)odm_get_obj(my_cl, "name like bos.rte.?p",
                                      &productobj, getit)) != 0) {
            getit = ODM_NEXT;
            if (rc == -1) {
                /* ODM failure */
                break;
            }
            else {
                *aix_version = productobj.ver;
                *aix_release = productobj.rel;
                *aix_level   = productobj.mod;
                *aix_fix     = productobj.fix;
                found++;
            }
        }
    }



    /*
     * close lpp object class
     */
    odm_close_class(my_cl);

    odm_terminate();

    free(path);
    return (found ? 0 : -1);

} /* bos_level */




#define CALC_NETSTAT(type) (double) ((cur_ninfo->type<last_ninfo->type)?-1:(cur_ninfo->type - last_ninfo->type)/timediff)
void
update_ifdata(void){

   static int init_done = 0;
   static struct timeval lasttime={0,0};
   struct timeval thistime;
   double timediff;


   /*
   ** Compute time between calls
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
   ** Do nothing if we are called to soon after the last call
   */
   if (init_done && (timediff < INFO_TIMEOUT)) return;

   lasttime = thistime;

   last_ninfo = &ninfo[ni_flag];

   ni_flag^=1;

   cur_ninfo = &ninfo[ni_flag];

   perfstat_netinterface_total(NULL, cur_ninfo, sizeof(perfstat_netinterface_total_t), 1);

   if (init_done) {
      cur_net_stat.ipackets = (CALC_NETSTAT(ipackets)<0)?cur_net_stat.ipackets:CALC_NETSTAT(ipackets);
      cur_net_stat.opackets = (CALC_NETSTAT(opackets)<0)?cur_net_stat.opackets:CALC_NETSTAT(opackets);
      cur_net_stat.ibytes   = (CALC_NETSTAT(ibytes ) <0)?cur_net_stat.ibytes:CALC_NETSTAT(ibytes );
      cur_net_stat.obytes   = (CALC_NETSTAT(obytes  )<0)?cur_net_stat.obytes:CALC_NETSTAT(obytes  );
   }
   else
     {
       init_done = 1;

       cur_net_stat.ipackets = 0;
       cur_net_stat.opackets = 0;
       cur_net_stat.ibytes   = 0;
       cur_net_stat.obytes   = 0;
     }

}  /* update_ifdata */


