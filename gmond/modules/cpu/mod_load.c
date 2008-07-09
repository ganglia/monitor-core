#include <gm_metric.h>
#include <libmetrics.h>

mmodule load_module;


static int load_metric_init ( apr_pool_t *p )
{
    int i;

    libmetrics_init();

    for (i = 0; load_module.metrics_info[i].name != NULL; i++) {
        /* Initialize the metadata storage for each of the metrics and then
         *  store one or more key/value pairs.  The define MGROUPS defines
         *  the key for the grouping attribute. */
        MMETRIC_INIT_METADATA(&(load_module.metrics_info[i]),p);
        MMETRIC_ADD_METADATA(&(load_module.metrics_info[i]),MGROUP,"load");
    }

    return 0;
}

static void load_metric_cleanup ( void )
{
}

static g_val_t load_metric_handler ( int metric_index )
{
    g_val_t val;

    /* The metric_index corresponds to the order in which
       the metrics appear in the metric_info array
    */
    switch (metric_index) {
    case 0:
        return load_one_func();
    case 1:
        return load_five_func();
    case 2:
        return load_fifteen_func();
    }

    /* default case */
    val.f = 0;
    return val;
}

static Ganglia_25metric load_metric_info[] = 
{
    {0, "load_one",      70, GANGLIA_VALUE_FLOAT, " ", "both", "%.2f", UDP_HEADER_SIZE+8, "One minute load average"},
    {0, "load_five",    325, GANGLIA_VALUE_FLOAT, " ", "both", "%.2f", UDP_HEADER_SIZE+8, "Five minute load average"},
    {0, "load_fifteen", 950, GANGLIA_VALUE_FLOAT, " ", "both", "%.2f", UDP_HEADER_SIZE+8, "Fifteen minute load average"},
    {0, NULL}
};

mmodule load_module =
{
    STD_MMODULE_STUFF,
    load_metric_init,
    load_metric_cleanup,
    load_metric_info,
    load_metric_handler,
};

