/*
 *  First stab at support for metrics in AIX
 *  by Preston Smith <psmith@physics.purdue.edu>
 *  Wed Feb 27 14:55:33 EST 2002
 *
 *  AIX V5 support, bugfixes added by Davide Tacchella <tack@cscs.ch>
 *  May 10, 2002
 *  
 */
#include "interface.h"
#include <stdlib.h>
#include <unistd.h>
#include <utmp.h>
#include <stdio.h>
#include <procinfo.h>
#include <strings.h>
#include <signal.h>
#include <fcntl.h>
#include <nlist.h>
#include <odmi.h>
#include <dlfcn.h>
#include <sys/cfgodm.h>
#include <sys/types.h>
#include <cf.h>
#include <sys/sysinfo.h>
#include <sys/systemcfg.h>
#include <sys/sysconfig.h>
#include <sys/utsname.h>
#include <sys/vminfo.h>
#include <sys/errno.h>
#ifdef HAVE_PMAPI
#include <pmapi.h>
#endif
/*#include <sys/proc.h> */

#include "libmetrics.h"

#define MAX_CPUS  64
#define N_VALUE(index) ((caddr_t)kernelnames[index].n_value)
#define KNSTRUCTS_CNT 3
#define NLIST_VMKER 0
#define NLIST_SYSINFO 0
#define MAXPROCS 5
#define NLIST_VMINFO 2

/* vmker structs taken from AIX monitor */

/* -- AIX/6000 System monitor 
**
**     get_sysvminfo.h
**
** Copyright (c) 1991-1995 Jussi Maki, All Rights Reserved.
** Copyright (c) 1993-2001 Marcel Mol, All Rights Reserved.
** NON-COMMERCIAL USE ALLOWED. YOU ARE FREE TO DISTRIBUTE
** THIS PROGRAM AND MODIFY IT AS LONG AS YOU KEEP ORIGINAL
** COPYRIGHTS.
*/

struct vmker433 {
    uint n0, n1, n2, n3, n4, n5, n6;
    uint pageresv; /* reserved paging space blocks */
    uint totalmem; /* total number of pages memory */
    uint badmem;   /* this is used in RS/6000 model 220 and C10 */
    uint totalvmem;
    uint virtacc;
    uint freevmem;
    uint n13;
    uint nonpinnable; /* number of reserved (non-pinable) memory pages */
    uint maxperm;  /* max number of frames no working */
    uint numperm;  /* seems to keep other than text and data segment usage */
                   /* the name is taken from /usr/lpp/bos/samples/vmtune.c */
    uint n17;
    uint n18;
    uint numclient;/* number of client frames */
    uint maxclient;/* max number of client frames */
};



struct vmker {
    uint n0, n1, n2, n3, n4, n5, n6, n7;
    uint pageresv; /* reserved paging space blocks */
    uint totalmem; /* total number of pages memory */
    uint badmem; /* this is used in RS/6000 model 220 and C10 */
    uint freemem; /* number of pages on the free list */
    uint maxperm; /* max number of frames no working */
    uint numperm; /* seems to keep other than text and data segment usage */
                  /* the name is taken from /usr/lpp/bos/samples/vmtune.c */
    uint totalvmem, freevmem;
    uint n16;
    uint nonpinnable; /* number of reserved (non-pinable) memory pages */
    uint n18, n19;
    uint numclient; /* number of client frames */
    uint maxclient; /* max number of client frames */
    uint n22, n23;
    uint n24, n25;
    uint n26, n27;
    uint n28, n29;
    uint n30, n31;
    uint n32, n33;
    uint n34, n35;
    uint n36, n37;
    uint n38, n39;
    uint n40, n41;
    uint n42, n43;
    uint n44, n45;
    uint n46, n47;
};

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

static struct nlist kernelnames[] = {  
    {"sysinfo", 0, 0, 0, 0, 0},
    {"cpuinfo", 0, 0, 0, 0, 0},
    {"vmker", 0, 0, 0, 0, 0},
    {NULL,      0, 0, 0, 0, 0},
}; 

void (*my_vmgetinfo) ();

int ci = 0;
struct cpuinfo *cpus[2] = {NULL, NULL};
struct sysinfo si[3];
struct vminfo vm[3];
struct vmker vmk;
struct vmker433 *vmk433;
static int n_cpus;
int aixver, aixrel, aixlev, aixfix;
struct Class *My_CLASS;


long cpu_sys, cpu_wait, cpu_user, cpu_idle, cpu_sum;
struct utsname unames;

/* Prototypes
 */
int get_cpuinfo(struct cpuinfo **cpu);
void getloadavg(double *loadv, int nelem);
int getloadavg_avenrun(double loadv[], int nelem);
void get_sys_info(struct sysinfo *s);
void calc_cpuinfo(struct sysinfo *si1, struct sysinfo *si2);
int swapqry(char *DevName, struct pginfo *pginfo);
int count_procs(int flag);
int getprocs(struct procsinfo *pbuf, int psize, struct fdsinfo *fbuf,
	     int fsize, pid_t iptr, int cnt);
void init_sys_vm();
void get_sys_vm_info(struct sysinfo *si, struct vmker *vmk, struct vminfo *vm);
int bos_level(int *aix_version, int *aix_release, int *aix_level, int *aix_fix);

/*
 * This function is called only once by the gmond.  Use to 
 * initialize data structures, etc or just return SYNAPSE_SUCCESS;
 */
g_val_t
metric_init(void)
{
   g_val_t val;
   ci ^= 1;

   uname(&unames);
   n_cpus = get_cpuinfo(&cpus[ci]); 
   get_sys_info(&si[ci]);
  /* get vm info, for memfrees */
   init_sys_vm();
   get_sys_vm_info(&si[ci], &vmk, &vm[ci]);
 
   calc_cpuinfo(&si[ci], &si[ci^1]);

  /* get oslevel from odm */
   bos_level(&aixver, &aixrel, &aixlev, &aixfix);

   val.int32 = SYNAPSE_SUCCESS;
   return val;
}

g_val_t
cpu_num_func ( void )
{
   g_val_t val;

   int ncpu;
   ncpu = get_cpuinfo(&cpus[ci^1]);
   
   if(ncpu) n_cpus = ncpu;

   val.uint16 = ncpu;
   return val;
}

/*
   With PMAPI CPU speed can be obtained with
   double pm_cycles(void);
*/

g_val_t
cpu_speed_func ( void )
{
   g_val_t val;
#ifdef HAVE_PMAPI
   double pm_cycles(void);
   val.uint32 = pm_cycles()/1000000;
#else
   val.uint16 = 0;
#endif
   return val;
}


g_val_t
boottime_func ( void )
{
   g_val_t val;
   int boottime = 0;
   struct utmp buf;
   FILE *utmp = fopen(UTMP_FILE, "r");

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

/* Nice little macros in sys/systemcfg.h to get the processor
   architecture
*/
g_val_t
machine_type_func ( void )
{
   g_val_t val;

   if( __power_rs())
	strncpy( val.str, "POWER", MAX_G_STRING_SIZE );
   if( __power_pc())
	strncpy( val.str, "PowerPC", MAX_G_STRING_SIZE );
#ifdef _AIXVERSION_510
   if( __isia64())
#else
   if( __ia64())
#endif
	strncpy( val.str, "IA/64", MAX_G_STRING_SIZE );
   return val;
}

g_val_t
os_name_func ( void )
{
   g_val_t val;
   strncpy( val.str, unames.sysname, MAX_G_STRING_SIZE );
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


/* AIX 4 defines
   CPU_IDLE, CPU_USER, CPU_KERNEL, CPU_WAIT
   so no metrics for cpu_nice, or cpu_aidle
   Check /usr/include/sys/sysinfo.h for details as to what the CPU stats
   actually mean.
*/
g_val_t
cpu_user_func ( void )
{
   g_val_t val;

   ci ^= 1;
   get_sys_info(&si[ci]);
   calc_cpuinfo(&si[ci], &si[ci^1]);

   val.f = cpu_user;
   if(val.f < 0) val.f = 0.0;
   return val;
}

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
   
   ci ^= 1;
   get_sys_info(&si[ci]);
   calc_cpuinfo(&si[ci], &si[ci^1]);

   val.f = cpu_sys;
   if(val.f < 0) val.f = 0.0;
   return val;
}
g_val_t 

cpu_wio_func ( void )
{
   g_val_t val;
   
   ci ^= 1;
   get_sys_info(&si[ci]);
   calc_cpuinfo(&si[ci], &si[ci^1]);

   val.f = cpu_wait;
   if(val.f < 0) val.f = 0.0;
   return val;
}

g_val_t 
cpu_idle_func ( void )
{
   g_val_t val;

   ci ^= 1;
   get_sys_info(&si[ci]);
   calc_cpuinfo(&si[ci], &si[ci^1]);

   val.f = cpu_idle;
   if(val.f < 0) val.f = 0.0;
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
load_one_func ( void )
{
   g_val_t val;
   double load[3];

   getloadavg_avenrun(load, 3);
   val.f = load[0];
   return val;
}

g_val_t
load_five_func ( void )
{
   g_val_t val;
   double load[3];
   getloadavg_avenrun(load, 3);
 
   val.f = load[1];
   return val;
}

g_val_t
load_fifteen_func ( void )
{
   g_val_t val;
   double load[3];
   getloadavg_avenrun(load, 3);

   val.f = load[2];
   return val;
}

  /* I get a SIGSEGV when I have the code for process counting 
     compiled in. It works when I display count_procs()
     from main() tho. Running in gmond, though, it reliably chokes on
     both proc_total_func and proc_run_func

     It works and gets a process count when run as a standalone program.
     
     So, I'm pretty sure count_procs() works ok. No idea where 
     the issue lies...

     Until I have a revelation, I've #ifdef'd it out.   

     tack notes that this SIGSEGV seems pthread related.
*/

g_val_t
proc_total_func ( void )
{
  g_val_t foo;
 
  int nprocs;

#ifdef COUNT_PROCS
  nprocs = count_procs(0);
#else
  nprocs = 0;
#endif
  foo.uint32 = nprocs;
 
  return foo;
}


g_val_t
proc_run_func( void )
{
  g_val_t val;
  int nprocs;

#ifdef COUNT_PROCS
  nprocs = count_procs(1);  
#else
  nprocs = 0;
#endif
  val.uint32 = nprocs;
 
  return val;
}


/* AIX does funny things with virtual memory.
   Like fill it up with buffered files.
   Don't know if there's an equivalent for shared and cached..
*/
g_val_t
mem_total_func ( void )
{
   g_val_t val;
   struct CuAt                 *at;
   struct objlistinfo          ObjListInfo;

   odm_initialize();
   odm_set_path("/etc/objrepos");

   at = get_CuAt_list(CuAt_CLASS, "name = 'sys0' and attribute = 'realmem'",
        &ObjListInfo, 20, 1);
   if ((int)at == -1) {
       odm_terminate();
   }

   (void) odm_close_class(CuAt_CLASS);
   odm_terminate();
   val.uint32 =  atoi(at->value);
   
   return val;
}

g_val_t
mem_free_func ( void )
{
   g_val_t val;
   uint mem;
#ifdef _AIXVERSION_510
   struct vminfo dtsvm;
   int status;

   memset(&dtsvm, 0, sizeof(struct vminfo));

   status = vmgetinfo (&dtsvm, VMINFO, 0);  
   mem = (int) dtsvm.numfrb; /* numfrb is a 64 bit integer, cast it to int */
#else
   /* Our code only gets memfree for >= 4.3.3 */
   if(aixver >= 4 && aixrel >= 3 && aixlev >= 3) {
     get_sys_vm_info(&si[ci], &vmk, &vm[ci]);
     mem = vmk.freemem;
   } else {
     mem = 0;
   }
#endif
   val.uint32 = mem * (getpagesize() / 1024); 
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

/* There's vmker entries totalvmem and freevm,
   but they don't seem to give anything correct.
 
   Swapqry should give it.. gotta do it for each paging space.
   Get it out of the ODM
*/

g_val_t
swap_total_func ( void )
{
   g_val_t val;
   struct CuAt                 *at;
   struct objlistinfo          ObjListInfo;
   register int                i;
   static char                 DevName[BUFSIZ];
   static struct pginfo        pginfo;
   int                         tot = 0;

   odm_initialize();
   odm_set_path("/etc/objrepos");

   at = get_CuAt_list(CuAt_CLASS, "value = 'paging' and attribute = 'type'", 
	&ObjListInfo, 20, 1);
   if ((int)at == -1) {
       odm_terminate();
       val.uint32 = 0;
       return val;
   }
  /* For each paging space the ODM tells us about, do swapqry on it.
  */
   for (i = 0; i < ObjListInfo.num; ++i, at++) {
       (void) sprintf(DevName, "/dev/%s", at->name);
       if (swapqry(DevName, &pginfo) == -1) {
           continue;
       }
       tot +=  ( (pginfo.size * getpagesize() ) / 1024 );
   }
   odm_terminate();
   val.uint32 = tot;
   return val;
   
}

g_val_t
swap_free_func ( void )
{
   g_val_t val;
   struct CuAt                 *at;
   struct objlistinfo          ObjListInfo;
   register int                i;
   static char                 DevName[BUFSIZ];
   static struct pginfo        pginfo;
   int                         tot = 0;

   odm_initialize();
   odm_set_path("/etc/objrepos");

   at = get_CuAt_list(CuAt_CLASS, "value = 'paging' and attribute = 'type'",
        &ObjListInfo, 20, 1);
   if ((int)at == -1) {
       odm_terminate();
       val.uint32 = 0;
       return val;
   }

  /* For each paging space the ODM tells us about, do swapqry on it.
  */
   for (i = 0; i < ObjListInfo.num; ++i, at++) {
       (void) sprintf(DevName, "/dev/%s", at->name);
       if (swapqry(DevName, &pginfo) == -1) {
           continue;
       }
       tot +=  ( (pginfo.free * getpagesize() ) / 1024 );
   }
   odm_terminate();
   val.uint32 = tot;
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




/* Lifted from AIX 'monitor'
 * http://www.mesa.nl/pub/monitor
 *
** -- AIX/6000 System monitor 
**
**     getkmemdata.c
**
** Copyright (c) 1991-1995 Jussi Maki, All Rights Reserved.
** Copyright (c) 1993-1998 Marcel Mol, All Rights Reserved.
** NON-COMMERCIAL USE ALLOWED. YOU ARE FREE TO DISTRIBUTE
** THIS PROGRAM AND MODIFY IT AS LONG AS YOU KEEP ORIGINAL
** COPYRIGHTS.
*/
int kmemfd = -1;

/*********************************************************************/

int getkmemdata(void *buf, int bufsize, caddr_t address) {
    int n;

    /*
     * Do stuff we only need to do once per invocation, like opening
     * the kmem file and fetching the parts of the symbol table.
     */
    if (kmemfd < 0) {
        if ((kmemfd = open("/dev/kmem", O_RDONLY)) < 0) {
            perror("kmem");
            return 0;
        }
        /*
         * We only need to be root for getting access to kmem, so give up
         * root permissions now!
         */
        setuid(getuid());
        setgid(getgid());
    }
    /*
     * Get the structure from the running kernel.
     */
    lseek(kmemfd, (off_t) address, SEEK_SET);
    n = read(kmemfd, buf, bufsize);

    return(n);

} /* getkmemdata */


/* -- AIX/6000 System monitor 
**
**     getloadavg.c
**
** Copyright (c) 1991-1995 Jussi Maki, All Rights Reserved.
** Copyright (c) 1993-1998 Marcel Mol, All Rights Reserved.
** NON-COMMERCIAL USE ALLOWED. YOU ARE FREE TO DISTRIBUTE
** THIS PROGRAM AND MODIFY IT AS LONG AS YOU KEEP ORIGINAL
** COPYRIGHTS.
*/


typedef struct {
    long l1, l5, l15;
} avenrun_t;

int getloadavg_avenrun(double *loadv, int nelem)
{
    static int initted=0;
    int avenrun[3];
    static struct nlist knames[] = {
          {"avenrun", 0, 0, 0, 0, 0},
          {NULL, 0, 0, 0, 0, 0}
      };

    static int no_avenrun_here = 0;

    if (no_avenrun_here) 
        return -1;

    if (!initted) {
        initted = 1;
        if (knlist(knames, 1, sizeof(struct nlist)) == -1){
            no_avenrun_here = 1;
            return -1;
        }
    }
    getkmemdata(&avenrun, sizeof(avenrun), (caddr_t) knames->n_value);
    if (nelem > 0)
       loadv[0] = avenrun[0] / 65536.0;
    if (nelem > 1)
       loadv[1] = avenrun[1] / 65536.0;
    if (nelem > 2)
       loadv[2] = avenrun[2] / 65536.0;

    return(0);

} /* getloadavg_avenrun */

/* -- AIX/6000 System monitor 
**
**     vmker.c
**
** Copyright (c) 1998 Marcel Mol, All Rights Reserved.
** NON-COMMERCIAL USE ALLOWED. YOU ARE FREE TO DISTRIBUTE
** THIS PROGRAM AND MODIFY IT AS LONG AS YOU KEEP ORIGINAL
** COPYRIGHTS.
*/

/* get_sys_info adapted from AIX monitor get_sys_vm_info.c
 */
void get_sys_info(struct sysinfo *s) {

    int nlistdone = 0;
    getkmemdata(s,  sizeof(struct sysinfo), N_VALUE(NLIST_SYSINFO));

    if (!nlistdone) {
        if (knlist(kernelnames, KNSTRUCTS_CNT, sizeof(struct nlist)) == -1)
            perror("knlist, entry not found");
        nlistdone = 1;
    }
    return;
} /* get_sys_info */


/* -- AIX/6000 System monitor 
**
**     get_cpuinfo.c
**
** Copyright (c) 1991-1995 Jussi Maki, All Rights Reserved.
** Copyright (c) 1993-1998 Marcel Mol, All Rights Reserved.
** NON-COMMERCIAL USE ALLOWED. YOU ARE FREE TO DISTRIBUTE
** THIS PROGRAM AND MODIFY IT AS LONG AS YOU KEEP ORIGINAL
** COPYRIGHTS.
*/


int get_cpuinfo(struct cpuinfo **cpu) {

    int ncpus;  
    ncpus = _system_configuration.ncpus;

    if (!kernelnames[0].n_value) {
        if (knlist(kernelnames, 1, sizeof(struct nlist)) == -1)
            perror("knlist, entry not found");
    }
    if (!*cpu)
      *cpu = (struct cpuinfo *)calloc(n_cpus, sizeof(struct cpuinfo));


    getkmemdata((char *) *cpu, sizeof(struct cpuinfo) * ncpus, 
                (caddr_t) kernelnames[0].n_value); 
    return ncpus;
} /* get_cpuinfo */


#define SIDELTA(a) (si1->a - si2->a)

/*
 * Compute the cpu percentages for CPU_IDLE, CPU_KERNEL, CPU_USER, CPU_WAIT
 * Get them from a sysinfo struct, which contains the cumulative CPU stats.
 * cpuinfo struct keeps the same info on a per-processor basis. Much easier
 * This way
 */
void
calc_cpuinfo(struct sysinfo *si1, struct sysinfo *si2) {

    
    cpu_sum = (SIDELTA(cpu[CPU_IDLE])   + SIDELTA(cpu[CPU_USER]) +
               SIDELTA(cpu[CPU_KERNEL]) + SIDELTA(cpu[CPU_WAIT])) / 100.0;
    cpu_sys  = SIDELTA(cpu[CPU_KERNEL]) / cpu_sum;
    cpu_wait = SIDELTA(cpu[CPU_WAIT])   / cpu_sum;
    cpu_user = SIDELTA(cpu[CPU_USER])   / cpu_sum;
    cpu_idle = SIDELTA(cpu[CPU_IDLE])   / cpu_sum;
 
    return;
}
  

/* Pass 0 as flag if you want to get a list of all processes.
   Pass 1 if you want to get a list of all SACTIVE processes */

int count_procs(int flag) {

  struct procsinfo ProcessBuffer[MAXPROCS];
  int ProcessSize = sizeof( ProcessBuffer[0] );
  struct fdsinfo FileBuffer[MAXPROCS];
  int FileSize = sizeof( FileBuffer[0] );
  pid_t IndexPointer = 0;
  int Count = MAXPROCS;
  int stat_val;
  int i;
  /* int state = -1; */
  int np = 0; 
  
  while ((stat_val = getprocs(
			      &ProcessBuffer[0],
			      ProcessSize,
			      NULL,
			      FileSize,
			      (pid_t)&IndexPointer,
			      Count )) == MAXPROCS  ) 
    
    {
      if(flag != 0) {
	for ( i=0; i<stat_val; i++) {
	  if(ProcessBuffer[i].pi_state == SACTIVE) np++;
	}
      } else {
	np += stat_val;
      }
    }       
	
  return np;
}

/* -- AIX/6000 System monitor 
**
**     get_sysvminfo.c
**
** Copyright (c) 1991-1995 Jussi Maki, All Rights Reserved.
** Copyright (c) 1993-2001 Marcel Mol, All Rights Reserved.
** NON-COMMERCIAL USE ALLOWED. YOU ARE FREE TO DISTRIBUTE
** THIS PROGRAM AND MODIFY IT AS LONG AS YOU KEEP ORIGINAL
** COPYRIGHTS.
*/


/* init_sys_vm and  get_sys_vm_info adapted from get_sysvminfo.c */

void init_sys_vm() { 
    void * handle;
    char *progname;

    if (knlist(kernelnames, KNSTRUCTS_CNT, sizeof(struct nlist)) == -1)
        perror("knlist, entry not found");

    vmk433 = (struct vmker433 *) malloc(sizeof(struct vmker433));
    /*
     * Get the call address for the vmgetinfo system call
     */
    if ((handle = dlopen("/unix", RTLD_NOW | RTLD_GLOBAL)) == NULL) {
        fprintf(stderr, "%s: init_sys_vm: dlopen: can't load %s (%s)\n",
                        progname, "/unix", dlerror());
        perror("dlopen");
        exit(1);
    }
    my_vmgetinfo = dlsym( handle, "vmgetinfo");
    dlclose(handle);
    if (my_vmgetinfo == NULL) {
        fprintf(stderr, "%s: init_sys_vm: dlsym: can't find %s (err %s)\n",
                        progname, "vmgetinfo", dlerror());
        perror("dlsym");
    }

   return;
}

void
get_sys_vm_info(struct sysinfo *si, struct vmker *vmk, struct vminfo *vm)
{

    getkmemdata(si,  sizeof(struct sysinfo), N_VALUE(NLIST_SYSINFO));
    /*
     * Get the system info structure from the running kernel.
     * Get the kernel virtual memory vmker structure
     * Get the kernel virtual memory info structure
     */
    my_vmgetinfo(vm, VMINFO, 1);

     getkmemdata(vmk433, sizeof(struct vmker433),   N_VALUE(NLIST_VMKER));
     vmk->totalmem    = vmk433->totalmem;
     vmk->badmem      = vmk433->badmem;
     vmk->freemem     = vm->numfrb;
     vmk->maxperm     = vm->maxperm;
     vmk->numperm     = vm->numperm;
     vmk->totalvmem   = vmk433->totalvmem;
     vmk->freevmem    = vmk433->freevmem;
     vmk->nonpinnable = 0;
     vmk->numclient   = vm->numclient;
     vmk->maxclient   = vm->maxclient;

    return;
}


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
     */
    getit = ODM_FIRST;
    while ((rc = (int)odm_get_obj(my_cl, "name like bos.?p",
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
     * AIX < 4.2 uses bos.mp or bos.up
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
