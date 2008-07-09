#include <gm_metric.h>
#include <libmetrics.h>

mmodule net_module;



static int net_metric_init ( apr_pool_t *p )
{
    int i;

    libmetrics_init();

    for (i = 0; net_module.metrics_info[i].name != NULL; i++) {
        /* Initialize the metadata storage for each of the metrics and then
         *  store one or more key/value pairs.  The define MGROUPS defines
         *  the key for the grouping attribute. */
        MMETRIC_INIT_METADATA(&(net_module.metrics_info[i]),p);
        MMETRIC_ADD_METADATA(&(net_module.metrics_info[i]),MGROUP,"network");
    }

    return 0;
}

static void net_metric_cleanup ( void )
{
}

static g_val_t net_metric_handler ( int metric_index )
{
    g_val_t val;

    /* The metric_index corresponds to the order in which
       the metrics appear in the metric_info array
    */
    switch (metric_index) {
    case 0:
        return bytes_out_func();
    case 1:
        return bytes_in_func();
    case 2:
        return pkts_in_func();
    case 3:
        return pkts_out_func();
    }

    /* default case */
    val.f = 0;
    return val;
}

static Ganglia_25metric net_metric_info[] = 
{
    {0, "bytes_out",  300, GANGLIA_VALUE_FLOAT,  "bytes/sec",   "both", "%.2f", UDP_HEADER_SIZE+8,  "Number of bytes out per second"},
    {0, "bytes_in",   300, GANGLIA_VALUE_FLOAT,  "bytes/sec",   "both", "%.2f", UDP_HEADER_SIZE+8,  "Number of bytes in per second"},
    {0, "pkts_in",    300, GANGLIA_VALUE_FLOAT,  "packets/sec", "both", "%.2f", UDP_HEADER_SIZE+8,  "Packets in per second"},
    {0, "pkts_out",   300, GANGLIA_VALUE_FLOAT,  "packets/sec", "both", "%.2f", UDP_HEADER_SIZE+8,  "Packets out per second"},
    {0, NULL}
};

mmodule net_module =
{
    STD_MMODULE_STUFF,
    net_metric_init,
    net_metric_cleanup,
    net_metric_info,
    net_metric_handler,
};

