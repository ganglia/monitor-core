#include <gm_metric.h>
#include <libmetrics.h>

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
#ifdef SOLARIS
    case 3:
        return bread_sec_func();
    case 4:
        return bwrite_sec_func();
    case 5:
        return lread_sec_func();
    case 6:
        return lwrite_sec_func();
    case 7:
        return phread_sec_func();
    case 8:
        return phwrite_sec_func();
    case 9:
        return rcache_func();
    case 10:
        return wcache_func();
#endif
    }

    /* default case */
    val.f = 0;
    return val;
}

static Ganglia_25metric disk_metric_info[] = 
{
    {0, "disk_total",    1200, GANGLIA_VALUE_DOUBLE, "GB", "both", "%.3f", UDP_HEADER_SIZE+16, "Total available disk space"},
    {0, "disk_free",      180, GANGLIA_VALUE_DOUBLE, "GB", "both", "%.3f", UDP_HEADER_SIZE+16, "Total free disk space"},
    {0, "part_max_used",  180, GANGLIA_VALUE_FLOAT,  "%",  "both", "%.1f", UDP_HEADER_SIZE+8,  "Maximum percent used for all partitions"},
#ifdef SOLARIS
    {0, "bread_sec",       90, GANGLIA_VALUE_FLOAT,  "1/sec", "both", "%.2f",UDP_HEADER_SIZE+8, "bread_sec"},
    {0, "bwrite_sec",      90, GANGLIA_VALUE_FLOAT,  "1/sec", "both", "%.2f",UDP_HEADER_SIZE+8, "bwrite_sec"}, 
    {0, "lread_sec",       90, GANGLIA_VALUE_FLOAT,  "1/sec", "both", "%.2f",UDP_HEADER_SIZE+8, "lread_sec"},
    {0, "lwrite_sec",      90, GANGLIA_VALUE_FLOAT,  "1/sec", "both", "%.2f",UDP_HEADER_SIZE+8, "lwrite_sec"},
    {0, "phread_sec",      90, GANGLIA_VALUE_FLOAT,  "1/sec", "both", "%.2f",UDP_HEADER_SIZE+8, "phread_sec"},
    {0, "phwrite_sec",     90, GANGLIA_VALUE_FLOAT,  "1/sec", "both", "%.2f",UDP_HEADER_SIZE+8, "phwrite_sec"},
    {0, "rcache",          90, GANGLIA_VALUE_FLOAT,  "%",     "both", "%.1f",UDP_HEADER_SIZE+8, "rcache"},
    {0, "wcache",          90, GANGLIA_VALUE_FLOAT,  "%",     "both", "%.1f",UDP_HEADER_SIZE+8, "wcache"},
#endif
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

