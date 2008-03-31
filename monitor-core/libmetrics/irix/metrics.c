/*
 *  irix.c - Ganglia monitor core gmond module for IRIX
 *
 * Change log:
 *
 *   15apr2002 - unknown ???
 *               Initial version
 *
 *   06may2003 - Martin Knoblauch <martin.knoblauch@mscsoftware.com>
 *               Minimum mtu is now actually computed
 *               CPU speed is taken from "sgikopt", assuming all CPUs in
 *               the system run at the same speed.
 *               bootime is computed correctly
 *               Add wait, interrupt and swap times to "system" time metric
 *               Add stub for standalone testing
 *               Add some FIXME comments, just as reminders :-)
 *               
 *   08may2003 - Martin Knoblauch <martin.knoblauch@mscsoftware.com>
 *               Add "cached" memory metrics. Don't ask why I know :-)
 *               Add "shared" memory metrics. See special comment.
 *               Add "total" and "running" processes.
 *               Add "aidle" cpu percentage. "nice" is the last missing metric.
 *
 */

#include "interface.h"
#include "libmetrics.h"
#include <sys/systeminfo.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/swap.h>
#include <sys/types.h>
#include <sys/times.h>

#include <fcntl.h>

#include <sys/sysinfo.h>
#include <sys/sysmp.h>

#include <sys/sysget.h>
#include <unistd.h>

int multiplier;		/* Pagesize / 1024 (for memory calcs) */

/*  here come swagner's crappy modifications.  all bad code
 *  is mine...
 */
struct sysinfo systeminfostuff;
struct cpuinfo {
   long		idle;
  float		idlepct;
   long		user;
  float		userpct;
   long		kernel;
  float		kernelpct;
   long		wait;
  float		waitpct;
   long		swap;
  float		swappct;
   long		interrupt;
  float		interruptpct;
   long		total;
} cpuinfo;

static struct cpuinfo oldcpuinfo;
static struct cpuinfo diffcpuinfo;

int
recalculate_cpu_percentages( void )
{
    static long last_update;
/*    if ( (time ( NULL ) - last_update) < 30 )
       {
       return 0;
       }
*/
    if (sysmp(MP_SAGET, MPSA_SINFO, &systeminfostuff, sizeof(struct sysinfo)) == -1) 
    {
      perror("sysmp");
      return -1;
    }
    cpuinfo.idle = systeminfostuff.cpu[0];
    cpuinfo.user = systeminfostuff.cpu[1];
    cpuinfo.kernel = systeminfostuff.cpu[2];
    cpuinfo.wait = systeminfostuff.cpu[3];
    cpuinfo.swap = systeminfostuff.cpu[4];
    cpuinfo.interrupt = systeminfostuff.cpu[5];
    cpuinfo.total = cpuinfo.idle + cpuinfo.user + cpuinfo.kernel +
		    cpuinfo.wait + cpuinfo.swap + cpuinfo.interrupt;
    if (cpuinfo.total < cpuinfo.idle)
       {
       return 0;
       }

    diffcpuinfo.idle = cpuinfo.idle - oldcpuinfo.idle;
    diffcpuinfo.user = cpuinfo.user - oldcpuinfo.user;
    diffcpuinfo.kernel = cpuinfo.kernel - oldcpuinfo.kernel;
    diffcpuinfo.wait = cpuinfo.wait - oldcpuinfo.wait;
    diffcpuinfo.swap = cpuinfo.swap - oldcpuinfo.swap;
    diffcpuinfo.interrupt = cpuinfo.interrupt - oldcpuinfo.interrupt;
    diffcpuinfo.total = cpuinfo.total - oldcpuinfo.total;

    oldcpuinfo.idle = cpuinfo.idle;
    oldcpuinfo.user = cpuinfo.user;
    oldcpuinfo.kernel = cpuinfo.kernel;
    oldcpuinfo.wait = cpuinfo.wait;
    oldcpuinfo.swap = cpuinfo.swap;
    oldcpuinfo.interrupt = cpuinfo.interrupt;
    oldcpuinfo.total = cpuinfo.total;


    if (diffcpuinfo.total == 0)
       {
       /* head off that divide by zero */
       return 0;
       }
    cpuinfo.idlepct = (float)diffcpuinfo.idle / diffcpuinfo.total * 100;
    cpuinfo.userpct = (float)diffcpuinfo.user / diffcpuinfo.total * 100;
    cpuinfo.kernelpct = (float)diffcpuinfo.kernel / diffcpuinfo.total * 100;
    cpuinfo.waitpct = (float)diffcpuinfo.wait / diffcpuinfo.total * 100;
    cpuinfo.swappct = (float)diffcpuinfo.swap / diffcpuinfo.total * 100;
    cpuinfo.interruptpct = (float)diffcpuinfo.interrupt / diffcpuinfo.total;
    last_update = time ( NULL );
    return 1;
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


/*
 * This function is called only once by the gmond.  Use to 
 * initialize data structures, etc or just return SYNAPSE_SUCCESS;
 */
g_val_t
metric_init(void)
{
   g_val_t val;

   (void) recalculate_cpu_percentages();
   multiplier = getpagesize() / 1024;
   val.int32 = SYNAPSE_SUCCESS;
   return val;
}

/*
 * 
 */

g_val_t
cpu_num_func ( void )
{
   g_val_t val;

   val.uint16 = sysconf(_SC_NPROC_ONLN);
   return val;
}

g_val_t
cpu_speed_func ( void )
{
   char cpufreq[16];
   g_val_t val;

   if (sgikopt("cpufreq",cpufreq,16) == 0)
     val.uint32 = atoi(cpufreq);
   else
     val.uint32 = 0;
   return val;
}

g_val_t
swap_total_func ( void )
{
   g_val_t val;
   off_t swaptotal;

   swapctl(SC_GETSWAPTOT, &swaptotal);
   val.f = swaptotal / 2;

   return val;
}

g_val_t
boottime_func ( void )
{
   struct tms bootime;
   g_val_t val;
  
   val.uint32 = time(NULL) - (times(&bootime) / CLK_TCK) ; 
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
   
   size = sysinfo(SI_MACHINE, val.str, MAX_G_STRING_SIZE);

   return val;
}

g_val_t
os_name_func ( void )
{
   g_val_t val;
   long size;
   
   size = sysinfo(SI_SYSNAME, val.str, MAX_G_STRING_SIZE);

   return val;
}        

g_val_t
os_release_func ( void )
{
   g_val_t val;
   long size;
   
   size = sysinfo(SI_RELEASE, val.str, MAX_G_STRING_SIZE);
   /* strncpy( val.str, "unknown", MAX_G_STRING_SIZE ); */
   return val;
}        

g_val_t
cpu_user_func ( void )
{
   g_val_t val;

   recalculate_cpu_percentages();
   val.f = cpuinfo.userpct;
   return val;
}

g_val_t
cpu_nice_func ( void )
{
   g_val_t val;

   /* FIXME ?? IRIX */
   val.f = 0.0;
   return val;
}

g_val_t 
cpu_system_func ( void )
{
   g_val_t val;

   recalculate_cpu_percentages();
/*
 * System time consists of kernel+interrupt+swap
 */
   val.f = cpuinfo.kernelpct 
         + cpuinfo.interruptpct + cpuinfo.swappct;
   return val;
}

g_val_t 
cpu_idle_func ( void )
{
   g_val_t val;

   recalculate_cpu_percentages();
   val.f = cpuinfo.idlepct;
   return val;
}

g_val_t 
cpu_aidle_func ( void )
{
   g_val_t val;
   
   recalculate_cpu_percentages();
   val.f = 0.0;
   if (cpuinfo.total != 0)
    val.f = ((float)cpuinfo.idle / (float)cpuinfo.total)*100.;
   return val;
}

g_val_t 
cpu_wio_func ( void )
{
   g_val_t val;

   recalculate_cpu_percentages();
   val.f = cpuinfo.waitpct;
   return val;
}

g_val_t 
cpu_intr_func ( void )
{
   g_val_t val;

   recalculate_cpu_percentages();
   val.f = cpuinfo.interruptpct;
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

/*
 * Implement helper function for load values.
 * Add error output
 */
enum {LOAD_1, LOAD_5, LOAD_15};

g_val_t
load_func ( int which )
{
   g_val_t val;
   sgt_cookie_t cookie;
   int avenrun[3];

   SGT_COOKIE_INIT(&cookie);
   SGT_COOKIE_SET_KSYM(&cookie, "avenrun");
   if (sysget(SGT_KSYM, (char *)avenrun, sizeof(avenrun),
          SGT_READ, &cookie) == -1)
   {
     perror("sysget AVENRUN");
     val.f = 0.0;
   }
   else
     val.f = ( (float) avenrun[which] ) / 1024.0;

   return val;
}

g_val_t
load_one_func ( void )
{
   return load_func( LOAD_1 );
}

g_val_t
load_five_func ( void )
{
   return load_func( LOAD_5 );
}

g_val_t
load_fifteen_func ( void )
{
   return load_func( LOAD_15 );
}

#include <sys/dirent.h>
#include <sys/procfs.h>

static int dbuf[4096];
static char pfilename[512];
enum {PROC_TOTAL, PROC_RUNNING};
/*
 * Helper function for processes.
 */
g_val_t
proc_func( int which )
{
g_val_t val;
int pfd,pffd,nlen,deof,i;
unsigned int totp,runp;
struct dirent *sdirp;
struct prpsinfo prpsi;
char *cdirp;

 val.uint32 = 0;
/*
 * On IRIX, "/proc/pinfo" contains non-priviledged readable process entries.
 * We try to open this directory now.
 */
 if ((pfd = open("/proc/pinfo", O_RDONLY|O_NONBLOCK)) == -1)
 {
   perror("proc_func: open pinfo directory failed");
 }
 else
 {
/*
 * Now we try to loop over all pseudo files in "/proc/pinfo".
 * First we need to get a list of all "dirent" entries.
 */
   totp = runp = 0;
   do
   {
/*
 * Try to get 16384 bytes worth of "dirent" data. If "deof" is 1, we got all of them
 */
     if ((nlen = ngetdents(pfd, (struct dirent *)&dbuf[0],  16384, &deof)) == -1)
     {
       perror("proc_func: ngetdents failed");
       close(pfd);
       return val;
     }
     else
     {
/*
 * Here we loop over the set of dirent structures returned by "ngetdents". As the
 * size of the actual entries is variable, we need to do some pointer math :-(
 */
       for (i = 0; i < nlen; )
       {
/*
 * Calculate address of next "dirent" structure
 */
         cdirp = ((char *)&dbuf[0]) + i;
         sdirp = (struct dirent *)cdirp;
	 i += sdirp->d_reclen;
/*
 * Discard "." and ".." entries
 */
         if(strpbrk(sdirp->d_name,".")) continue;
/*
 * Every remaining file points to a process. Just increment number of total processes here.
 */
         totp++;
         if (which == PROC_TOTAL) continue;
/*
 * Lets try to open the process file for reading
 */
         strcpy(pfilename,"/proc/pinfo/");
	 strcat(pfilename,sdirp->d_name);
         if ((pffd = open(pfilename, O_RDONLY)) == -1)
         {
           perror("proc_func: opening process file failed");
           printf("pfile: %s\n",pfilename);
         }
         else
         {
/*
 * Now do an ioctl call to get to the "prpsinfo" data
 */
           if(ioctl(pffd, PIOCPSINFO, &prpsi) == -1)
           {
             perror("proc_func: ioctl on process file failed");
           }
           else
           {
	     if(prpsi.pr_sname == 'R') 
             {
//               printf("pid = %d %d %d (%c) %s\n",
//		 prpsi.pr_pid,prpsi.pr_flag,prpsi.pr_state,prpsi.pr_sname,prpsi.pr_fname);
               runp++;
             }
           }
           close(pffd);
         }
       }
     }
   } while ( deof == 0) ;
   close (pfd);
   switch (which) {
     case PROC_TOTAL: val.uint32 = totp; break;
     case PROC_RUNNING: val.uint32 = runp; break;
     default: break;
   }
 }

 return val;
}

g_val_t
proc_run_func( void )
{
   return proc_func( PROC_RUNNING );
}

g_val_t
proc_total_func ( void )
{
   return proc_func( PROC_TOTAL );
}

/*
 * Implement helper function for various memory metrics.
 */
enum {MEM_TOTAL, MEM_FREE, MEM_SHARED, MEM_BUFFERS, MEM_CACHED};

g_val_t
mem_func( int which )
{
  g_val_t val;
  struct rminfo rmi;

  val.f = 0;
  if (sysmp(MP_SAGET, MPSA_RMINFO, &rmi, sizeof(struct rminfo)) == -1)
  {
     perror("sysmp failed in mem_func");
  }
  else
  {
    switch (which)
    {
      case MEM_TOTAL:   val.f = rmi.physmem*multiplier; break;
      case MEM_FREE:    val.f = rmi.freemem*multiplier; break;
      case MEM_SHARED:  val.f = (rmi.dchunkpages+rmi.dpages)*multiplier; break;
      case MEM_BUFFERS: val.f = rmi.bufmem*multiplier; break;
      case MEM_CACHED:  val.f = (rmi.chunkpages-rmi.dchunkpages)*multiplier; break;
      default:
        err_msg("mem_func() - invalid value for which = %d", which);
    }
  }
  return val;
}

g_val_t
mem_total_func ( void )
{
   return mem_func( MEM_TOTAL );
}

g_val_t
mem_free_func ( void )
{
   return mem_func( MEM_FREE );
}

/*
 * MKN: "shared" is the amount of dirty cached memory. I did this, because
 * I couldn't find any other useful deployment of this base metric.
 *
 */
g_val_t
mem_shared_func ( void )
{
   return mem_func( MEM_SHARED );
}

g_val_t
mem_buffers_func ( void )
{
   return mem_func( MEM_BUFFERS );
}

/*
 * MKN: This shows the amount of "clean" cached memory. 
 */
g_val_t
mem_cached_func ( void )
{
   return mem_func( MEM_CACHED );
}

g_val_t
swap_free_func ( void )
{
   g_val_t val;
   off_t swapfree;

   swapctl(SC_GETFREESWAP, &swapfree);
   val.f = swapfree / 2;
   return val;
}
