#ifndef GM_METRIC_H
#define GM_METRIC_H 1

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <rpc/types.h>
#include <rpc/xdr.h>

#include <gm_mmn.h>
#ifndef GM_PROTOCOL_GUARD
#include <netinet/in.h>
#include <gm_protocol.h>
#endif
#include <gm_value.h>
#include <gm_msg.h>

#include <confuse.h>   /* header for libconfuse */
#include <apr.h>
#include <apr_pools.h>
#include <apr_tables.h>

#define MGROUP "GROUP"

typedef void (*metric_info_func)(Ganglia_25metric *gmi);
typedef g_val_t (*metric_func)(int metric_index);
typedef g_val_t (*metric_func_void)(void);

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
    Ganglia_25metric *metrics_info;

    /** Metric callback function */
    metric_func handler;
};

/* Convenience macros for adding metadata key/value pairs to a metric structure element */
#define MMETRIC_INIT_METADATA(m,p) \
    do {    \
        void **t = (void**)&((m)->metadata);   \
        *t = (void*)apr_table_make(p, 2);    \
    } while (0)

#define MMETRIC_ADD_METADATA(m,k,v) \
    apr_table_add((apr_table_t*)(m)->metadata,k,v)

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

#endif  /* GM_METRIC_H */
