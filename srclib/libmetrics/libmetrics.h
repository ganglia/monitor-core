#ifndef LIBMETRICS_H
#define LIBMETRICS_H 1

void libmetrics_init( void );

#ifndef SYNAPSE_SUCCESS
#define SYNAPSE_SUCCESS 0
#endif
#ifndef SYNAPSE_FAILURE
#define SYNAPSE_FAILURE -1
#endif

#include <sys/types.h>
#include <rpc/types.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

typedef enum {
   g_string,  /* huh uh.. he said g string */
   g_int8,
   g_uint8,
   g_int16,
   g_uint16,
   g_int32,
   g_uint32,
   g_float,
   g_double,
   g_timestamp    /* a uint32 */
} g_type_t;
 
#define MAX_G_STRING_SIZE 32
 
typedef union {
    int8_t   int8;
   uint8_t  uint8;
   int16_t  int16;
  uint16_t uint16;
   int32_t  int32;
  uint32_t uint32;
   float   f; 
   double  d;
   char str[MAX_G_STRING_SIZE];
} g_val_t;         

 g_val_t metric_init(void);
 g_val_t cpu_num_func(void);
 g_val_t cpu_speed_func(void);
 g_val_t mem_total_func(void);
 g_val_t swap_total_func(void);
 g_val_t boottime_func(void);
 g_val_t sys_clock_func(void);
 g_val_t machine_type_func(void);
 g_val_t os_name_func(void);
 g_val_t os_release_func(void);
 g_val_t mtu_func(void);
 g_val_t cpu_user_func(void);
 g_val_t cpu_nice_func(void);
 g_val_t cpu_system_func(void);
 g_val_t cpu_idle_func(void);
 g_val_t cpu_wio_func(void);
 g_val_t cpu_aidle_func(void);
 g_val_t load_one_func(void);
 g_val_t load_five_func(void);
 g_val_t load_fifteen_func(void);
 g_val_t proc_run_func(void);
 g_val_t proc_total_func(void);
 g_val_t mem_free_func(void);
 g_val_t mem_shared_func(void);
 g_val_t mem_buffers_func(void);
 g_val_t mem_cached_func(void);
 g_val_t swap_free_func(void);
 g_val_t gexec_func(void);
 g_val_t heartbeat_func(void);
 g_val_t location_func(void);

/* the following are additional internal metrics added by swagner
 * what for the monitoring of buffer/linear read/writes on Solaris boxen.
 * these are only valid on the solaris version of gmond v2.3.1b1,
 * all others are untested.  caveat haxor. :P
 */     
        
#ifdef SOLARIS
        
 g_val_t bytes_in_func(void);
 g_val_t bytes_out_func(void);
 g_val_t pkts_in_func(void);
 g_val_t pkts_out_func(void);
 g_val_t bread_sec_func(void);
 g_val_t bwrite_sec_func(void);
 g_val_t lread_sec_func(void);
 g_val_t lwrite_sec_func(void);
 g_val_t phread_sec_func(void);
 g_val_t phwrite_sec_func(void);
 g_val_t rcache_func(void);
 g_val_t wcache_func(void);
        
#endif  

#ifdef LINUX

 g_val_t cpu_intr_func(void);
 g_val_t cpu_sintr_func(void);
 g_val_t bytes_in_func(void);
 g_val_t bytes_out_func(void);
 g_val_t pkts_in_func(void);
 g_val_t pkts_out_func(void);
 g_val_t disk_total_func(void);
 g_val_t disk_free_func(void);
 g_val_t part_max_used_func(void);

#endif

#ifdef CYGWIN

 g_val_t cpu_intr_func(void);
 g_val_t cpu_sintr_func(void);
 g_val_t bytes_in_func(void);
 g_val_t bytes_out_func(void);
 g_val_t pkts_in_func(void);
 g_val_t pkts_out_func(void);

#endif

#ifdef HPUX

 g_val_t cpu_intr_func(void);
 g_val_t cpu_sintr_func(void);
 g_val_t mem_rm_func(void);
 g_val_t mem_arm_func(void);
 g_val_t mem_vm_func(void);
 g_val_t mem_avm_func(void);

#endif

#ifdef FREEBSD

 g_val_t bytes_in_func(void);
 g_val_t bytes_out_func(void);
 g_val_t pkts_in_func(void);
 g_val_t pkts_out_func(void);
 g_val_t disk_total_func(void);
 g_val_t disk_free_func(void);
 g_val_t part_max_used_func(void);

#endif

enum {
   user_defined,
  /*
   * These are my configuration metrics which don't change between reboots
   */
   cpu_num,
   cpu_speed,
   mem_total,
   swap_total,
   boottime,
   sys_clock,
   machine_type,
   os_name,
   os_release,
  /*
   * These are my state metrics which are always a changin' changin'
   */
   cpu_user,
   cpu_nice,
   cpu_system,
   cpu_idle,
   cpu_wio,
   cpu_aidle,
   load_one,
   load_five,
   load_fifteen,
   proc_run,
   proc_total,
   mem_free,
   mem_shared,
   mem_buffers,
   mem_cached,
   swap_free,
   /* internal.. ignore */
   gexec,
   heartbeat,
   mtu,
   location,
   /*
    * added by swagner for extended solaris monitoring.
    */
#ifdef SOLARIS
   bytes_in,
   bytes_out,
   pkts_in,
   pkts_out,
   bread_sec,
   bwrite_sec,
   lread_sec,
   lwrite_sec,
   phread_sec,
   phwrite_sec,
   rcache,
   wcache,
#endif
#ifdef LINUX
   cpu_intr,
   cpu_sintr,
   bytes_in,
   bytes_out,
   pkts_in,
   pkts_out,
   disk_total,
   disk_free,
   part_max_used,
#endif
#ifdef CYGWIN
   cpu_intr,
   cpu_sintr,
   bytes_in,
   bytes_out,
   pkts_in,
   pkgs_out,
#endif
#ifdef HPUX
   cpu_intr,
   cpu_sintr,
   cpu_arm,
   cpu_rm,
   cpu_avm,
   cpu_vm,
#endif
#ifdef FREEBSD
   bytes_in,
   bytes_out,
   pkts_in,
   pkts_out,
   disk_total,
   disk_free,
   part_max_used,
#endif
#ifdef IRIX
   cpu_intr,
#endif
   num_key_metrics
}  key_metrics;

#endif /* LIBMETRICS_H */
