#ifndef METRIC_H
#define METRIC_H 1
#include <ganglia.h>
#include "metric_typedefs.h"
#include "key_metrics.h"

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
extern g_val_t cpu_user_func(void);
extern g_val_t cpu_nice_func(void);
extern g_val_t cpu_system_func(void);
extern g_val_t cpu_idle_func(void);
extern g_val_t cpu_wio_func(void);
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
        
#endif  
     
/* end of swagner modifications.
 *      
 */     
   



#define INIT 0, 0, {0}, {0}
#define KEY(NAME) { #NAME, NAME ##_func, INIT

/*
 * All behavior of the gmond is tweak'd by this array.. 
 *
 * Each metric is checked during some random interval between "check_min" and
 * "check_max" (assuming the threshold value is not negative).  If the metric is 
 * "threshold" greater than the previous metric value then the key_metric 
 * (listed on the left) is multicast.  If there has been no multicast 
 * during some random interval from "mcast_min" to "mcast_max" then a multicast 
 * is sent regardless of the metric value.
 *
 * NOTE: it is critical that the key matches the key in the enum key_metrics above
 */
metric_t metric[num_key_metrics] = {
/*             threshold    check_max  mcast_max       units        */
/* key               check_min   mcast_min     type             fmt */
/*------------------------------------------------------------------*/

/* user_defined does nothing */
{"user_defined",metric_init,
                  INIT,  -1,  -1,   -1,   -1,   -1, g_int32, "",     ""},

/* CONFIGURATION METRICS */
KEY(cpu_num),      -1,  -1,   -1,  900, 1200, g_uint16, "CPUs","%hu"},
KEY(cpu_speed),    -1,  -1,   -1,  900, 1200, g_uint32, "MHz", "%hu"},
KEY(mem_total),    -1,  -1,   -1,  900, 1200, g_uint32, "KBs", "%u" },
KEY(swap_total),   -1,  -1,   -1,  900, 1200, g_uint32, "KBs", "%u" },
KEY(boottime),     -1,  -1,   -1,  900, 1200, g_uint32, "",    "%u" },
KEY(sys_clock),    -1,  -1,   -1,  900, 1200, g_uint32, "",    "%u" },
KEY(machine_type), -1,  -1,   -1,  900, 1200, g_string, "",    "%s" },
KEY(os_name),      -1,  -1,   -1,  900, 1200, g_string, "",    "%s" },
KEY(os_release),   -1,  -1,   -1,  900, 1200, g_string, "",    "%s" },

/* STATE METRICS */
KEY(cpu_user),      1,  15,   20,   60,   90, g_float, "%",   "%.1f"},
KEY(cpu_nice),      1,  15,   20,   60,   90, g_float, "%",   "%.1f"},
KEY(cpu_system),    1,  15,   20,   60,   90, g_float, "%",   "%.1f"},
KEY(cpu_idle),      5,  15,   20,   60,   90, g_float, "%",   "%.1f"},
KEY(cpu_wio),     5, 850,  950, 3400, 3800, g_float, "%",   "%.1f"},
KEY(load_one),      1,  15,   20,   50,   70, g_float, "",    "%.2f"},
KEY(load_five),     1,  30,   40,  275,  325, g_float, "",    "%.2f"},
KEY(load_fifteen),  1,  60,   80,  850,  950, g_float, "",    "%.2f"},
KEY(proc_run),      1,  45,   80,  850,  950, g_uint32, "procs","%u"},
KEY(proc_total),    1,  45,   80,  850,  950, g_uint32, "procs","%u"},
KEY(mem_free),   1024,  30,   40,  120,  180, g_uint32, "KBs", "%u" },
KEY(mem_shared), 1024,  30,   40,  120,  180, g_uint32, "KBs", "%u" },
KEY(mem_buffers),1024,  30,   40,  120,  180, g_uint32, "KBs", "%u" },
KEY(mem_cached), 1024,  30,   40,  120,  180, g_uint32, "KBs", "%u" },
KEY(swap_free),  1024,  30,   40,  120,  180, g_uint32, "KBs", "%u" },

/* gmond internals */
KEY(gexec),        -1,  -1,   -1,  180,  300, g_string, "",    "%s"},

#ifdef SOLARIS

/* buffer reads and writes, adjusted per second */

KEY(bread_sec),   1,  15,   20,  60,  90, g_float, "", "%.2f" },
KEY(bwrite_sec), 1,  15,   20,  60,  90, g_float, "", "%.2f" },

/* linear reads/writes, adjusted per second */

KEY(lread_sec),1,  15,   20,  60,  90, g_float, "", "%.2f" },
KEY(lwrite_sec), 1,  15,   20,  60,  90, g_float, "", "%.2f" },

/* cache hit ratios, rcache = 1 - (bread/lread), wcache = 1 - (bwrite/lwrite) */

KEY(rcache), 1,  15,   20,  60,  90, g_float, "%", "%.1f" },
KEY(wcache), 1,  15,   20,  60,  90, g_float, "%", "%.1f" },

/* physical reads/writes per second. */

KEY(phread_sec), 1,  15,   20,  60,  90, g_float, "", "%.2f" },
KEY(phwrite_sec), 1,  15,   20,  60,  90, g_float, "", "%.2f" },

#endif

KEY(heartbeat),    -1,  -1,   -1,   10,   20, g_uint32, "",    "%u"}

};

#endif  /* METRIC_H */
