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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef AIX
  void *malloc(size_t size);
 
 char *
 rpl_malloc(size_t n)
 {
     if (n == 0)
         n = 1;
     return malloc (n);
 }
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gm_value.h>
#include <gm_msg.h>

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
 g_val_t cpu_intr_func(void);
 g_val_t cpu_sintr_func(void);
 g_val_t cpu_steal_func(void);
 g_val_t bytes_in_func(void);
 g_val_t bytes_out_func(void);
 g_val_t pkts_in_func(void);
 g_val_t pkts_out_func(void);
 g_val_t disk_total_func(void);
 g_val_t disk_free_func(void);
 g_val_t part_max_used_func(void);
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

#ifdef LINUX
 g_val_t mem_sreclaimable_func (void);
#endif

/* the following are additional internal metrics added by swagner
 * what for the monitoring of buffer/linear read/writes on Solaris boxen.
 * these are only valid on the solaris version of gmond v2.3.1b1,
 * all others are untested.  caveat haxor. :P
 */     
        
#ifdef SOLARIS
        
 g_val_t bread_sec_func(void);
 g_val_t bwrite_sec_func(void);
 g_val_t lread_sec_func(void);
 g_val_t lwrite_sec_func(void);
 g_val_t phread_sec_func(void);
 g_val_t phwrite_sec_func(void);
 g_val_t rcache_func(void);
 g_val_t wcache_func(void);
        
#endif  

#ifdef HPUX

 g_val_t mem_rm_func(void);
 g_val_t mem_arm_func(void);
 g_val_t mem_vm_func(void);
 g_val_t mem_avm_func(void);

#endif

#endif /* LIBMETRICS_H */
