/*
 *  Stub file so that ganglia will build on Darwin (MacOS X)
 *  by Preston Smith <psmith@physics.purdue.edu>
 *  Mon Oct 14 14:18:01 EST 2002
 *
 */

#include "interface.h"
#include <kvm.h>
#include <sys/sysctl.h>
#include "libmetrics.h"

/* Function prototypes */
 
/*
 * This function is called only once by the gmond.  Use to 
 * initialize data structures, etc or just return SYNAPSE_SUCCESS;
 */
g_val_t
metric_init(void)
{
   g_val_t val;
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
   long cpu_speed;
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
   size_t len;
   int total;
   int mib[2];

   mib[0] = CTL_HW;
   mib[1] = HW_PHYSMEM;
   len = sizeof (total);

   sysctl(mib, 2, &total, &len, NULL, 0);
   total /= 1024;
   val.uint32 = total;

   return val;
}

g_val_t
swap_total_func ( void )
{
   g_val_t val;
/*
   struct kvm_swap swap[1];
   kvm_t *kd;
   int totswap, n;

   kd = kvm_open(NULL, NULL, NULL, O_RDONLY, "kvm_open"); 
   n = kvm_getswapinfo(kd, swap, 1, 0);
   if (n < 0 || swap[0].ksw_total == 0) { 
       val.uint32 = 0;
   }
   totswap = swap[0].ksw_total;
   totswap *= getpagesize() / 1024;
   val.uint32 = totswap;
   kvm_close(kd); */
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

g_val_t 
cpu_aidle_func ( void )
{
   g_val_t val;
   
   val.f = 0.0;
   return val;
}

g_val_t 
cpu_wio_func ( void )
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
   int mib[3];
   size_t len;

   mib[0] = CTL_KERN;
   mib[1] = KERN_PROC;
   mib[2] = KERN_PROC_ALL;

   sysctl(mib, 3, NULL, &len, NULL, 0);

   val.uint32 = (len / sizeof (struct kinfo_proc)); 

   return val;


   return val;
}


/* 
 * Don't know how to do this yet..
 */
g_val_t
proc_run_func( void )
{
   g_val_t val;

   val.uint32 = 0;
   return val;
}

g_val_t
mem_free_func ( void )
{
   g_val_t val;

   val.uint32 = 0;
   return val;
}

g_val_t
mem_shared_func ( void )
{
   g_val_t val;

   val.uint32 = 0;
   return val;
}

g_val_t
mem_buffers_func ( void )
{
   g_val_t val;

   val.uint32 = 0;
   return val;
}

g_val_t
mem_cached_func ( void )
{
   g_val_t val;

   val.uint32 = 0;
   return val;
}

g_val_t
swap_free_func ( void )
{
   g_val_t val;
   val.uint32 = 0;
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

