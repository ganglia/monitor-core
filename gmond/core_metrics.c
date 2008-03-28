#include <gm_metric.h>
#include <libmetrics.h>

mmodule core_metrics;

/*
** A helper function to determine the number of cpustates in /proc/stat (MKN)
*/

static int core_metrics_init ( apr_pool_t *p )
{
    int i;

    for (i = 0; core_metrics.metrics_info[i].name != NULL; i++) {
        /* Initialize the metadata storage for each of the metrics and then
         *  store one or more key/value pairs.  The define MGROUPS defines
         *  the key for the grouping attribute. */
        MMETRIC_INIT_METADATA(&(core_metrics.metrics_info[i]),p);
        MMETRIC_ADD_METADATA(&(core_metrics.metrics_info[i]),MGROUP,"core");
    }

    return 0;
}

static void core_metrics_cleanup ( void )
{
}

static g_val_t core_metrics_handler ( int metric_index )
{
    g_val_t val;

    /* The metric_index corresponds to the order in which
       the metrics appear in the metric_info array
    */
    switch (metric_index) {
    case 0:
        return gexec_func();
    case 1:
        return heartbeat_func();
    case 2:
        return location_func();
    }

    /* default case */
    val.int32 = 0;
    return val;
}

static Ganglia_25metric core_metrics_info[] = 
{
    {0, "gexec",      300, GANGLIA_VALUE_STRING,       "",        "zero", "%s", UDP_HEADER_SIZE+32, "gexec available"},
    {0, "heartbeat",   20, GANGLIA_VALUE_UNSIGNED_INT, "",        "",     "%u", UDP_HEADER_SIZE+8,  "Last heartbeat"},
    {0, "location",  1200, GANGLIA_VALUE_STRING,       "(x,y,z)", "",     "%s", UDP_HEADER_SIZE+12, "Location of the machine"},
    {0, NULL}
};

mmodule core_metrics =
{
    STD_MMODULE_STUFF,
    core_metrics_init,
    core_metrics_cleanup,
    core_metrics_info,
    core_metrics_handler,
};

