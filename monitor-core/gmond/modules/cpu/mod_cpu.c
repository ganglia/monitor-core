#include <gm_metric.h>
#include <libmetrics.h>

mmodule cpu_module;

/*
** A helper function to determine the number of cpustates in /proc/stat (MKN)
*/

static int cpu_metric_init ( apr_pool_t *p )
{
    int i;

    libmetrics_init();

    for (i = 0; cpu_module.metrics_info[i].name != NULL; i++) {
        /* Initialize the metadata storage for each of the metrics and then
         *  store one or more key/value pairs.  The define MGROUPS defines
         *  the key for the grouping attribute. */
        MMETRIC_INIT_METADATA(&(cpu_module.metrics_info[i]),p);
        MMETRIC_ADD_METADATA(&(cpu_module.metrics_info[i]),MGROUP,"cpu");
    }

    return 0;
}

static void cpu_metric_cleanup ( void )
{
}

static g_val_t cpu_metric_handler ( int metric_index )
{
    g_val_t val;

    /* The metric_index corresponds to the order in which
       the metrics appear in the metric_info array
    */
    switch (metric_index) {
    case 0:
        return cpu_num_func();
    case 1:
        return cpu_speed_func();
    case 2:
        return cpu_user_func();
    case 3:
        return cpu_nice_func();
    case 4:
        return cpu_system_func();
    case 5:
        return cpu_idle_func();
    case 6:
        return cpu_aidle_func();
    case 7:
        return cpu_wio_func();
    case 8:
        return cpu_intr_func();
    case 9:
        return cpu_sintr_func();
    }

    /* default case */
    val.int32 = 0;
    return val;
}

static Ganglia_25metric cpu_metric_info[] = 
{
    {0, "cpu_num",    1200, GANGLIA_VALUE_UNSIGNED_SHORT, "CPUs", "zero", "%hu",  UDP_HEADER_SIZE+8, "Total number of CPUs"},
    {0, "cpu_speed",  1200, GANGLIA_VALUE_UNSIGNED_INT,   "MHz",  "zero", "%u",  UDP_HEADER_SIZE+8, "CPU Speed in terms of MHz"},
    {0, "cpu_user",     90, GANGLIA_VALUE_FLOAT,          "%",    "both", "%.1f", UDP_HEADER_SIZE+8, "Percentage of CPU utilization that occurred while executing at the user level"},
    {0, "cpu_nice",     90, GANGLIA_VALUE_FLOAT,          "%",    "both", "%.1f", UDP_HEADER_SIZE+8, "Percentage of CPU utilization that occurred while executing at the user level with nice priority"},
    {0, "cpu_system",   90, GANGLIA_VALUE_FLOAT,          "%",    "both", "%.1f", UDP_HEADER_SIZE+8, "Percentage of CPU utilization that occurred while executing at the system level"},
    {0, "cpu_idle",     90, GANGLIA_VALUE_FLOAT,          "%",    "both", "%.1f", UDP_HEADER_SIZE+8, "Percentage of time that the CPU or CPUs were idle and the system did not have an outstanding disk I/O request"},
    {0, "cpu_aidle",  3800, GANGLIA_VALUE_FLOAT,          "%",    "both", "%.1f", UDP_HEADER_SIZE+8, "Percent of time since boot idle CPU"},
    {0, "cpu_wio",      90, GANGLIA_VALUE_FLOAT,          "%",    "both", "%.1f", UDP_HEADER_SIZE+8, "Percentage of time that the CPU or CPUs were idle during which the system had an outstanding disk I/O request"},
    {0, "cpu_intr",     90, GANGLIA_VALUE_FLOAT,          "%",    "both", "%.1f", UDP_HEADER_SIZE+8, "cpu_intr"},
    {0, "cpu_sintr",    90, GANGLIA_VALUE_FLOAT,          "%",    "both", "%.1f", UDP_HEADER_SIZE+8, "cpu_sintr"},
    {0, NULL}

};

mmodule cpu_module =
{
    STD_MMODULE_STUFF,
    cpu_metric_init,
    cpu_metric_cleanup,
    cpu_metric_info,
    cpu_metric_handler,
};

