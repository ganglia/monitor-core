/*
 * Libmetrics Native Windows Metrics (through mingw32)
 *
 * Copyright (c) 2008 Carlo Marcelo Arenas Belon <carenas@sajinet.com.pe>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
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
#include <sys/timeb.h>
#include <psapi.h>

/* From old ganglia 2.5.x... */
#include "libmetrics.h"
/* End old ganglia 2.5.x headers */
#undef min
#undef max
#include "interface.h"

const char *sys_osname = "Windows";
char sys_osrelease[MAX_G_STRING_SIZE];

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
  }

  if (in_bytes) *in_bytes = bytes_in;
  if (out_bytes) *out_bytes = bytes_out;
  if (in_pkts) *in_pkts = pkts_in;
  if (out_pkts) *out_pkts = pkts_out;

  return timebuffer.time;
}

/*
 * This function is called only once by the gmond.  Use to 
 * initialize data structures, etc or just return SYNAPSE_SUCCESS;
 */
g_val_t
metric_init(void)
{
   g_val_t rval;
   OSVERSIONINFO osvi;

   ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

   if (GetVersionEx(&osvi)) {
      snprintf(sys_osrelease, MAX_G_STRING_SIZE, "%d.%d",
               osvi.dwMajorVersion, osvi.dwMinorVersion);
   } else {
      strncpy(sys_osrelease, "unknown", MAX_G_STRING_SIZE);
   }
   sys_osrelease[MAX_G_STRING_SIZE - 1] = '\0';
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
   diff = (unsigned long)(out_pkts - last_pkts_out);
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
   diff = (unsigned long)(out_bytes - last_bytes_out);
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
   diff = (unsigned long)(in_bytes - last_bytes_in);
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
   g_val_t val;

   val.uint32 = 0;

   return val;
}

g_val_t
mem_total_func ( void )
{
   MEMORYSTATUSEX stat;
   DWORDLONG size;
   g_val_t val;

   stat.dwLength = sizeof(stat);

   if ( GlobalMemoryStatusEx (&stat)) {
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

   if ( GlobalMemoryStatusEx (&stat)) {
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
   g_val_t val;

   val.uint32 = 0;

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

g_val_t
cpu_user_func ( void )
{
   g_val_t val;
   
   val.f = 0.0;
     
   return val;
}

g_val_t
cpu_nice_func ( void )
{
   g_val_t val;
 
   val.f = 0.0;
 
   return val;
}

g_val_t 
cpu_system_func ( void )
{
   g_val_t val;
 
   val.f = 0.0;
 
   return val;
}

g_val_t 
cpu_idle_func ( void )
{
   g_val_t val;
 
   val.f = 0.0;
     
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

#define MAXPROCESSES 1024

/* FIXME */
g_val_t
proc_run_func( void )
{
   DWORD aProcesses[MAXPROCESSES], cbNeeded, cProcesses;
   unsigned int i, running = 0;
   HANDLE hProcess;
   BOOL bResult;
   g_val_t val;

   if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
      cProcesses = 0;
   } else {
      cProcesses = cbNeeded / sizeof(DWORD);
   }
#if (_WIN32_WINNT >= 0x0501)
   /* Only for XP or newer */
   for (i = 0; i < cProcesses; i++)
      if (aProcesses[i] != 0) {
         hProcess = OpenProcess (PROCESS_QUERY_INFORMATION, 
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

   if ( GlobalMemoryStatusEx (&stat)) {
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

   if ( GlobalMemoryStatusEx (&stat)) {
      size = stat.ullAvailPageFile;
      /* get the value in kB */
      val.f =  size / 1024;
   } else {
      val.f = 0;
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

/* FIXME: hardcoded max number of disks */
#define MAXDRIVES 4

typedef struct {
   double total;
   double avail;
} disk_t;

static float
find_disk_space(double *total, double *avail)
{
   float most_full = 0.0;

   *total = 0.0;
   *avail = 0.0;

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
