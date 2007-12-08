#include <metric.h>
#include <gm_mmn.h>

#include <libmetrics.h>

//#include "utils.h"

mmodule disk_module;


static int disk_metric_init ( apr_pool_t *p )
{
    int i;

	libmetrics_init();

    for (i = 0; disk_module.metrics_info[i].name != NULL; i++) {
        /* Initialize the metadata storage for each of the metrics and then
         *  store one or more key/value pairs.  The define MGROUPS defines
         *  the key for the grouping attribute. */
        MMETRIC_INIT_METADATA(&(disk_module.metrics_info[i]),p);
        MMETRIC_ADD_METADATA(&(disk_module.metrics_info[i]),MGROUP,"disk");
    }

    return 0;
}

static void disk_metric_cleanup ( void )
{
}

static g_val_t disk_metric_handler ( int metric_index )
{
    g_val_t val;

    /* The metric_index corresponds to the order in which
       the metrics appear in the metric_info array
    */
    switch (metric_index) {
    case 0:
        return disk_total_func();
    case 1:
        return disk_free_func();
    case 2:
        return part_max_used_func();
    }

    /* default case */
    val.int32 = 0;
    return val;
}

static const Ganglia_25metric disk_metric_info[] = 
{
    {0, "mdisk_total",    1200, GANGLIA_VALUE_DOUBLE, "GB", "both", "%.3f", UDP_HEADER_SIZE+16, "Total available disk space"},
    {0, "mdisk_free",      180, GANGLIA_VALUE_DOUBLE, "GB", "both", "%.3f", UDP_HEADER_SIZE+16, "Total free disk space"},
    {0, "mpart_max_used",  180, GANGLIA_VALUE_FLOAT,  "%",  "both", "%.1f", UDP_HEADER_SIZE+8,  "Maximum percent used for all partitions"},
    {0, NULL}

};

mmodule disk_module =
{
    STD_MMODULE_STUFF,
    disk_metric_init,
    disk_metric_cleanup,
    disk_metric_info,
    disk_metric_handler,
};

