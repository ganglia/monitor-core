#ifndef METRIC_H
#define METRIC_H 1

#include "libmetrics.h"

typedef g_val_t (*metric_func)(void);

/*
 * Each host parameter is assigned it's own metric_t structure.  This
 * structure is used by both the monitoring and multicasting threads.
 */
typedef struct
{
   const char name[16];          /* the name of the metric */
   metric_func func; 
   int check_threshold;
   int mcast_threshold;
   g_val_t now;
   g_val_t last_mcast;     /* the value at last multicast */
   int value_threshold; 
   int check_min;          /* check_min and check_max define a range of secs */
   int check_max;          /*    which pass between each /proc check for the metric */
   int mcast_min;          /* msg_min and msg_max define a range of secs */
   int mcast_max;          /*    which a mcast of the metric is forced */
   g_type_t type;          /* type of data in our union */
   char units[32];         /* units the value are in */
   char fmt[16];           /* how to format the binary to a human-readable string */
   int key;                /* the unique key for this metric */
}
metric_t;          

/* SEE key_metrics.h for list of key metrics */

/* If you add a value to the key_metrics enum then you need to add
 * its func here as well.  They need to be of type "metric_func" defined
 * above.  These functions are all added to the metric_<os>.c file along
 * with the metric_init() function.  The metric_init() function is only
 * called once.
 */
extern g_val_t metric_init(void);
 
extern g_val_t cpu_num_func(void);
extern g_val_t cpu_speed_func(void);
extern g_val_t mem_total_func(void);
extern g_val_t swap_total_func(void);
extern g_val_t boottime_func(void);
extern g_val_t sys_clock_func(void);
extern g_val_t machine_type_func(void);
extern g_val_t os_name_func(void);
extern g_val_t os_release_func(void);
extern g_val_t mtu_func(void);
extern g_val_t cpu_user_func(void);
extern g_val_t cpu_nice_func(void);
extern g_val_t cpu_system_func(void);
extern g_val_t cpu_idle_func(void);
extern g_val_t cpu_aidle_func(void);
extern g_val_t load_one_func(void);
extern g_val_t load_five_func(void);
extern g_val_t load_fifteen_func(void);
extern g_val_t proc_run_func(void);
extern g_val_t proc_total_func(void);
extern g_val_t mem_free_func(void);
extern g_val_t mem_shared_func(void);
extern g_val_t mem_buffers_func(void);
extern g_val_t mem_cached_func(void);
extern g_val_t swap_free_func(void);
extern g_val_t gexec_func(void);
extern g_val_t heartbeat_func(void);
extern g_val_t mtu_func(void);
extern g_val_t location_func(void);

/* the following are additional internal metrics added by swagner
 * what for the monitoring of buffer/linear read/writes on Solaris boxen.
 * these are only valid on the solaris version of gmond v2.3.1b1,
 * all others are untested.  caveat haxor. :P
 */     
        
#ifdef SOLARIS
        
extern g_val_t bread_sec_func(void);
extern g_val_t bwrite_sec_func(void);
extern g_val_t lread_sec_func(void);
extern g_val_t lwrite_sec_func(void);
extern g_val_t phread_sec_func(void);
extern g_val_t phwrite_sec_func(void);
extern g_val_t rcache_func(void);
extern g_val_t wcache_func(void);
extern g_val_t cpu_wio_func(void);
        
#endif  

#ifdef LINUX

extern g_val_t bytes_in_func(void);
extern g_val_t bytes_out_func(void);
extern g_val_t pkts_in_func(void);
extern g_val_t pkts_out_func(void);
extern g_val_t disk_total_func(void);
extern g_val_t disk_free_func(void);
extern g_val_t part_max_used_func(void);

#endif

#ifdef HPUX

extern g_val_t cpu_wait_func(void);
extern g_val_t cpu_intr_func(void);
extern g_val_t cpu_ssys_func(void);
extern g_val_t mem_rm_func(void);
extern g_val_t mem_arm_func(void);
extern g_val_t mem_vm_func(void);
extern g_val_t mem_avm_func(void);

#endif

#ifdef FREEBSD

extern g_val_t bytes_in_func(void);
extern g_val_t bytes_out_func(void);
extern g_val_t pkts_in_func(void);
extern g_val_t pkts_out_func(void);
extern g_val_t disk_total_func(void);
extern g_val_t disk_free_func(void);
extern g_val_t part_max_used_func(void);

#endif

#endif  /* METRIC_H */
