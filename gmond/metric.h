#ifndef METRIC_H
#define METRIC_H 1

#include "gm_mmn.h"
#include "libmetrics.h"
#include "protocol.h"
#include "confuse.h"   /* header for libconfuse */

#include <apr.h>
#include <apr_pools.h>
#include <apr_tables.h>

typedef void (*metric_info_func)(Ganglia_25metric *gmi);
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

/**
 * Module structures.  
 */
typedef struct mmodule_param mmparam;
struct mmodule_param {
    char *name;
    char *value;
};

typedef struct mmodule_struct mmodule;
struct mmodule_struct {
    /** API version, *not* module version; check that module is 
     * compatible with this version of the server.
     */
    int version;
    /** API minor version. Provides API feature milestones. */
    int minor_version;

    /** The name of the module's C file */
    const char *name;
    /** The handle for the DSO.  Internal use only */
    void *dynamic_load_handle;
    /** The module name */
    char *module_name;
    /** The metric name */
    char *metric_name;
    /** Single string parameter */
    char *module_params;
    /** Multiple name/value pair parameter list */
    apr_array_header_t *module_params_list;
    /** Configuration file handle */
    cfg_t *config_file;

    /** A pointer to the next module in the list
     *  @defvar module_struct *next */
    struct mmodule_struct *next;

    /** Magic Cookie to identify a module structure. */
    unsigned long magic;

    /** Metric init callback function */
    int (*init)(apr_pool_t *p); /* callback function */

    /** Metric cleanup callback function */
    void (*cleanup)(void); /* callback function */

    /** Metric info callback function */
    const Ganglia_25metric *metrics_info;

    /** Metric callback function */
    g_val_t (*handler)(int metric_index); /* callback function */
};

/** Use this in all standard modules */
#define STD_MMODULE_STUFF	MMODULE_MAGIC_NUMBER_MAJOR, \
				MMODULE_MAGIC_NUMBER_MINOR, \
				__FILE__, \
				NULL, \
				NULL, \
				NULL, \
                NULL, \
                NULL, \
                NULL, \
                NULL, \
				MMODULE_MAGIC_COOKIE


#endif  /* METRIC_H */
