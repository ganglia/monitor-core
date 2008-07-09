#include <gm_metric.h>
#include <libmetrics.h>

mmodule proc_module;


static int proc_metric_init ( apr_pool_t *p )
{
    int i;

    libmetrics_init();

    for (i = 0; proc_module.metrics_info[i].name != NULL; i++) {
        /* Initialize the metadata storage for each of the metrics and then
         *  store one or more key/value pairs.  The define MGROUPS defines
         *  the key for the grouping attribute. */
        MMETRIC_INIT_METADATA(&(proc_module.metrics_info[i]),p);
        MMETRIC_ADD_METADATA(&(proc_module.metrics_info[i]),MGROUP,"process");
    }

    return 0;
}

static void proc_metric_cleanup ( void )
{
}

static g_val_t proc_metric_handler ( int metric_index )
{
    g_val_t val;

    /* The metric_index corresponds to the order in which
       the metrics appear in the metric_info array
    */
    switch (metric_index) {
    case 0:
        return proc_run_func();
    case 1:
        return proc_total_func();
    }

    /* default case */
    val.uint32 = 0;
    return val;
}

static Ganglia_25metric proc_metric_info[] = 
{
    {0, "proc_run",   950, GANGLIA_VALUE_UNSIGNED_INT, " ", "both", "%u", UDP_HEADER_SIZE+8, "Total number of running processes"},
    {0, "proc_total", 950, GANGLIA_VALUE_UNSIGNED_INT, " ", "both", "%u", UDP_HEADER_SIZE+8, "Total number of processes"},
    {0, NULL}

};

mmodule proc_module =
{
    STD_MMODULE_STUFF,
    proc_metric_init,
    proc_metric_cleanup,
    proc_metric_info,
    proc_metric_handler,
};

