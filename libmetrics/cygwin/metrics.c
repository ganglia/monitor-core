/*
 * This file contains all the metrics gathering code from Windows using cygwin
 * or native Windows calls when possible.
 *
 * Tested in Windows XP Home SP3 (i386)
 * Tested in Windows Vista Home SP1 (i386)
 * Tested in Windows Advanced Server 2000 (i386)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <iphlpapi.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <mntent.h>
#include <sys/vfs.h>
#include <psapi.h>

/* From old ganglia 2.5.x... */
#include "gm_file.h"
#include "libmetrics.h"
/* End old ganglia 2.5.x headers */
#undef min
#undef max
#include "interface.h"

char *proc_cpuinfo = NULL;

char sys_osname[MAX_G_STRING_SIZE];
char sys_osrelease[MAX_G_STRING_SIZE];

timely_file proc_stat = { {0, 0}, 1., "/proc/stat", NULL, BUFFSIZE };

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
  while ((ret = GetIfTable(iftable, &dwSize, 1)) == ERROR_INSUFFICIENT_BUFFER) {
     iftable = (PMIB_IFTABLE) realloc (iftable, dwSize);
  }

  if (ret == NO_ERROR) {

    ftime ( &timebuffer );

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

/*
 * A helper function to determine the number of cpustates in /proc/stat (MKN)
 */
#define NUM_CPUSTATES_24X 4
#define NUM_CPUSTATES_26X 7
static unsigned int num_cpustates;

unsigned int
num_cpustates_func ( void )
{
   char *p;
   unsigned int i=0;

   proc_stat.last_read.tv_sec = 0;
   proc_stat.last_read.tv_usec = 0;
   p = update_file(&proc_stat);
   proc_stat.last_read.tv_sec = 0;
   proc_stat.last_read.tv_usec = 0;

/*
** Skip initial "cpu" token
*/
   p = skip_token(p);
   p = skip_whitespace(p);
/*
** Loop over file until next "cpu" token is found.
** i=4 : Cygwin
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
   struct utsname u;
   g_val_t rval;
   char *bp;

   num_cpustates = num_cpustates_func();

   bp = proc_cpuinfo;
   rval.int32 = slurpfile("/proc/cpuinfo", &bp, BUFFSIZE);
   if (proc_cpuinfo == NULL)
      proc_cpuinfo = bp;

   if ( rval.int32 == SLURP_FAILURE ) {
         err_msg("metric_init() got an error from slurpfile() /proc/cpuinfo");
         rval.int32 = SYNAPSE_FAILURE;
         return rval;
   }

   if (uname(&u) == -1) {
      strncpy(sys_osname, "unknown", MAX_G_STRING_SIZE);
      strncpy(sys_osrelease, "unknown", MAX_G_STRING_SIZE);
   } else {
      strncpy(sys_osname, u.sysname, MAX_G_STRING_SIZE);
      sys_osname[MAX_G_STRING_SIZE - 1] = '\0';
      strncpy(sys_osrelease, u.release, MAX_G_STRING_SIZE);
      sys_osrelease[MAX_G_STRING_SIZE - 1] = '\0';
   }
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
   diff = (unsigned long)(in_pkts - last_pkts_in);
   if ( diff && last_stamp ) {
     t = stamp - last_stamp;
     t = diff / t;
     debug_msg("Returning value: %f\n", t);
   } else
     t = 0;

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
   diff = (unsigned long)(out_pkts - last_pkts_out);
   if ( diff && last_stamp ) {
     t = stamp - last_stamp;
     t = diff / t;
     debug_msg("Returning value: %f\n", t);
   } else
     t = 0;

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
   diff = (unsigned long)(out_bytes - last_bytes_out);
   if ( diff && last_stamp ) {
     t = stamp - last_stamp;
     t = diff / t;
     debug_msg("Returning value: %f\n", t);
   } else
     t = 0;

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
   diff = (unsigned long)(in_bytes - last_bytes_in);
   if ( diff && last_stamp ) {
     t = stamp - last_stamp;
     t = diff / t;
     debug_msg("Returning value: %f\n", t);
   } else
     t = 0;

   val.f = t;
   last_bytes_in = in_bytes;
   last_stamp = stamp;

   return val;
}

g_val_t
cpu_num_func ( void )
{
   static DWORD cpu_num = 0;
   SYSTEM_INFO siSysInfo;
   g_val_t val;

   /* Only need to do this once */
   if (!cpu_num) {
      GetSystemInfo(&siSysInfo);
      cpu_num = siSysInfo.dwNumberOfProcessors;
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
         if (p) {
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
         if (p) {
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
         if (p) {
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
   MEMORYSTATUSEX stat;
   DWORDLONG size;
   g_val_t val;

   stat.dwLength = sizeof(stat);

   if (GlobalMemoryStatusEx(&stat)) {
      size = stat.ullTotalPhys;
      /* get the value in kB */
      val.f =  size / 1024;
   } else {
      val.f = 0;
   }

   return val;
}

/* FIXME?: should be using PERFORMANCE_INFORMATION.CommitLimit */
g_val_t
swap_total_func ( void )
{
   MEMORYSTATUSEX stat;
   DWORDLONG size;
   g_val_t val;

   stat.dwLength = sizeof(stat);

   if (GlobalMemoryStatusEx(&stat)) {
      size = stat.ullTotalPageFile;
      /* get the value in kB */
      val.f =  size / 1024;
   } else {
      val.f = 0;
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
   if (p) {
     p = skip_token ( p );
     val.uint32 = atoi ( p );
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
   SYSTEM_INFO siSysInfo;
   g_val_t val;

   GetSystemInfo(&siSysInfo);

   switch (siSysInfo.wProcessorArchitecture) {
      case PROCESSOR_ARCHITECTURE_AMD64:
         snprintf(val.str, MAX_G_STRING_SIZE, "x86_64");
         break;
      case PROCESSOR_ARCHITECTURE_IA64:
         snprintf(val.str, MAX_G_STRING_SIZE, "ia64");
         break;
      case PROCESSOR_ARCHITECTURE_INTEL:
         snprintf(val.str, MAX_G_STRING_SIZE, "x86");
         break;
      case PROCESSOR_ARCHITECTURE_ALPHA:
      case PROCESSOR_ARCHITECTURE_ALPHA64:
         snprintf(val.str, MAX_G_STRING_SIZE, "alpha");
         break;
      case PROCESSOR_ARCHITECTURE_PPC:
         snprintf(val.str, MAX_G_STRING_SIZE, "powerpc");
         break;
      case PROCESSOR_ARCHITECTURE_MIPS:
         snprintf(val.str, MAX_G_STRING_SIZE, "mips");
         break;
      case PROCESSOR_ARCHITECTURE_ARM:
         snprintf(val.str, MAX_G_STRING_SIZE, "arm");
         break;
      default:
         snprintf(val.str, MAX_G_STRING_SIZE, "unknown");
   }

   return val;
}

g_val_t
os_name_func ( void )
{
   g_val_t val;

   snprintf(val.str, MAX_G_STRING_SIZE, "%s", sys_osname);

   return val;
}

g_val_t
os_release_func ( void )
{
   g_val_t val;

   snprintf(val.str, MAX_G_STRING_SIZE, "%s", sys_osrelease);

   return val;
}

/*
 * A helper function to return the total number of cpu jiffies
 */
unsigned long
total_jiffies_func ( void )
{
   char *p;
   unsigned long user_jiffies, nice_jiffies, system_jiffies, idle_jiffies;

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

   return (user_jiffies + nice_jiffies + system_jiffies + idle_jiffies);
}

g_val_t
cpu_user_func ( void )
{
   char *p;
   static g_val_t val;
   static struct timeval stamp = {0, 0};
   static double last_user_jiffies,  user_jiffies,
                 last_total_jiffies, total_jiffies, diff;

   p = update_file(&proc_stat);
   if ((proc_stat.last_read.tv_sec != stamp.tv_sec) &&
       (proc_stat.last_read.tv_usec != stamp.tv_usec)) {
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
   static struct timeval stamp = {0, 0};
   static double last_nice_jiffies,  nice_jiffies,
                 last_total_jiffies, total_jiffies, diff;

   p = update_file(&proc_stat);
   if ((proc_stat.last_read.tv_sec != stamp.tv_sec) &&
       (proc_stat.last_read.tv_usec != stamp.tv_usec)) {
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
   static struct timeval stamp = {0, 0};
   static double last_system_jiffies,  system_jiffies,
                 last_total_jiffies, total_jiffies, diff;

   p = update_file(&proc_stat);
   if ((proc_stat.last_read.tv_sec != stamp.tv_sec) &&
       (proc_stat.last_read.tv_usec != stamp.tv_usec)) {
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
   static struct timeval stamp = {0, 0};
   static double last_idle_jiffies,  idle_jiffies,
                 last_total_jiffies, total_jiffies, diff;

   p = update_file(&proc_stat);
   if ((proc_stat.last_read.tv_sec != stamp.tv_sec) &&
       (proc_stat.last_read.tv_usec != stamp.tv_usec)) {
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

/* FIXME? */
g_val_t
cpu_aidle_func ( void )
{
   g_val_t val;

   val.f = 0.0;

   return val;
}

/* FIXME? */
g_val_t
cpu_wio_func ( void )
{
   g_val_t val;

   val.f = 0.0;

   return val;
}

/* FIXME? */
g_val_t
cpu_intr_func ( void )
{
   g_val_t val;

   val.f = 0.0;

   return val;
}

/* FIXME? */
g_val_t
cpu_sintr_func ( void )
{
   g_val_t val;

   val.f = 0.0;

   return val;
}

/* FIXME? */
g_val_t
load_one_func ( void )
{
   g_val_t val;

   val.f = 0.0;

   return val;
}

/* FIXME? */
g_val_t
load_five_func ( void )
{
   g_val_t val;

   val.f = 0.0;

   return val;
}

/* FIXME? */
g_val_t
load_fifteen_func ( void )
{
   g_val_t val;

   val.f = 0.0;

   return val;
}

/* FIXME: fixed number of processes */
#define MAXPROCESSES 1024

/* FIXME */
g_val_t
proc_run_func( void )
{
   DWORD aProcesses[MAXPROCESSES], cbNeeded, cProcesses;
   unsigned int running = 0;
   g_val_t val;

   if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
      cProcesses = 0;
   } else {
      cProcesses = cbNeeded / sizeof(DWORD);
   }
#if (_WIN32_WINNT >= 0x0501)
   /* Only for XP or newer */
   unsigned int i;
   HANDLE hProcess;
   BOOL bResult;

   for (i = 0; i < cProcesses; i++)
      if (aProcesses[i] != 0) {
         hProcess = OpenProcess(PROCESS_QUERY_INFORMATION,
                                FALSE, aProcesses[i]);
         if (hProcess != NULL) {
            if (IsProcessInJob(hProcess, NULL, &bResult)) {
               if (bResult)
                  running++;
            }
            CloseHandle(hProcess);
         }
      }
#endif

   val.uint32 = running;

   return val;
}

/* FIXME */
g_val_t
proc_total_func ( void )
{
   DWORD aProcesses[MAXPROCESSES], cbNeeded, cProcesses;
   g_val_t val;

   if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
      cProcesses = 0;
   } else {
      cProcesses = cbNeeded / sizeof(DWORD);
   }
   val.uint32 = cProcesses;

   return val;
}

g_val_t
mem_free_func ( void )
{
   MEMORYSTATUSEX stat;
   DWORDLONG size;
   g_val_t val;

   stat.dwLength = sizeof(stat);

   if (GlobalMemoryStatusEx(&stat)) {
      size = stat.ullAvailPhys;
      /* get the value in kB */
      val.f =  size / 1024;
   } else {
      val.f = 0;
   }

   return val;
}

/* FIXME */
g_val_t
mem_shared_func ( void )
{
   g_val_t val;

   val.f = 0;

   return val;
}

/* FIXME */
g_val_t
mem_buffers_func ( void )
{
   g_val_t val;

   val.f = 0;

   return val;
}

/* FIXME */
g_val_t
mem_cached_func ( void )
{
   g_val_t val;

   val.f = 0;

   return val;
}

/* FIXME: should be using PERFORMANCE_INFORMATION.CommitTotal */
g_val_t
swap_free_func ( void )
{
   MEMORYSTATUSEX stat;
   DWORDLONG size;
   g_val_t val;

   stat.dwLength = sizeof(stat);

   if (GlobalMemoryStatusEx(&stat)) {
      size = stat.ullAvailPageFile;
      /* get the value in kB */
      val.f =  size / 1024;
   } else {
      val.f = 0;
   }

   return val;
}

g_val_t
mtu_func ( void )
{
   /*
    * We want to find the minimum MTU (Max packet size) over all UP
    * interfaces.
    */
   g_val_t val;

   val.uint32 = get_min_mtu();

   /* A val of 0 means there are no UP interfaces. Shouldn't happen. */
   return val;
}

/* FIXME: hardcoded max number of disks */
#define MAXDRIVES 4

typedef struct {
   double total;
   double avail;
} disk_t;

static float
find_disk_space(double *total, double *avail)
{
   FILE *mnttab;
   disk_t drives[MAXDRIVES];
   struct mntent *ent;
   struct statfs fs;
   const double reported_units = 1e9;
   int drive;
   float pct;
   float most_full = 0.0;

   *total = 0.0;
   *avail = 0.0;
   bzero(drives, sizeof(disk_t) * MAXDRIVES);

   mnttab = setmntent(MOUNTED, "r");
   while ((ent = getmntent(mnttab)) != NULL) {
      if (islower((int)ent->mnt_fsname[0])) {
         drive = ent->mnt_fsname[0] - 'a';
         if (drives[drive].total == 0.0) {
            statfs(ent->mnt_fsname, &fs);

            drives[drive].total = (double)fs.f_blocks * fs.f_bsize;
            drives[drive].avail = (double)fs.f_bavail * fs.f_bsize;

            pct = (drives[drive].avail == 0) ? 100.0 :
               ((drives[drive].total -
               drives[drive].avail)/drives[drive].total) * 100.0;

            if (pct > most_full)
               most_full = pct;

            *total += (drives[drive].total / reported_units);
            *avail += (drives[drive].avail / reported_units);
         }
      }
   }
   endmntent(mnttab);

   return most_full;
}

g_val_t
disk_free_func( void )
{
   double total_free = 0.0;
   double total_size = 0.0;
   g_val_t val;

   find_disk_space(&total_size, &total_free);

   val.d = total_free;
   return val;
}

g_val_t
disk_total_func( void )
{
   double total_free = 0.0;
   double total_size = 0.0;
   g_val_t val;

   find_disk_space(&total_size, &total_free);

   val.d = total_size;
   return val;
}

g_val_t
part_max_used_func( void )
{
   double total_free = 0.0;
   double total_size = 0.0;
   float most_full;
   g_val_t val;

   most_full = find_disk_space(&total_size, &total_free);

   val.f = most_full;
   return val;
}
