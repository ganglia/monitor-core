#include <metric.h>
#include <gm_mmn.h>

#include <libmetrics.h>


mmodule mem_module;


static int mem_metric_init ( apr_pool_t *p )
{
    int i;

	libmetrics_init();

	for (i = 0; mem_module.metrics_info[i].name != NULL; i++) {
        /* Initialize the metadata storage for each of the metrics and then
         *  store one or more key/value pairs.  The define MGROUPS defines
         *  the key for the grouping attribute. */
        MMETRIC_INIT_METADATA(&(mem_module.metrics_info[i]),p);
        MMETRIC_ADD_METADATA(&(mem_module.metrics_info[i]),MGROUP,"memory");
    }

    return 0;
}

static void mem_metric_cleanup ( void )
{
}

static g_val_t mem_metric_handler ( int metric_index )
{
    g_val_t val;

    /* The metric_index corresponds to the order in which
       the metrics appear in the metric_info array
    */
    switch (metric_index) {
    case 0:
        return mem_total_func();
    case 1:
        return mem_free_func();
    case 2:
        return mem_shared_func();
    case 3:
        return mem_buffers_func();
    case 4:
        return mem_cached_func();
    case 5:
        return swap_free_func();
    case 6:
        return swap_total_func();
    }

    /* default case */
    val.int32 = 0;
    return val;
}

static const Ganglia_25metric mem_metric_info[] = 
{
    {0, "mmem_total",  1200, GANGLIA_VALUE_UNSIGNED_INT, "KB", "zero", "%u", UDP_HEADER_SIZE+8, "Total amount of memory displayed in KBs"},
    {0, "mmem_free",    180, GANGLIA_VALUE_UNSIGNED_INT, "KB", "both", "%u", UDP_HEADER_SIZE+8, "Amount of available memory"},
    {0, "mmem_shared",  180, GANGLIA_VALUE_UNSIGNED_INT, "KB", "both", "%u", UDP_HEADER_SIZE+8, "Amount of shared memory"},
    {0, "mmem_buffers", 180, GANGLIA_VALUE_UNSIGNED_INT, "KB", "both", "%u", UDP_HEADER_SIZE+8, "Amount of buffered memory"},
    {0, "mmem_cached",  180, GANGLIA_VALUE_UNSIGNED_INT, "KB", "both", "%u", UDP_HEADER_SIZE+8, "Amount of cached memory"},
    {0, "mswap_free",   180, GANGLIA_VALUE_UNSIGNED_INT, "KB", "both", "%u", UDP_HEADER_SIZE+8, "Amount of available swap memory"},
    {0, "mswap_total", 1200, GANGLIA_VALUE_UNSIGNED_INT, "KB", "zero", "%u", UDP_HEADER_SIZE+8, "Total amount of swap space displayed in KBs"},
    {0, NULL}

};

mmodule mem_module =
{
    STD_MMODULE_STUFF,
    mem_metric_init,
    mem_metric_cleanup,
    mem_metric_info,
    mem_metric_handler,
};

