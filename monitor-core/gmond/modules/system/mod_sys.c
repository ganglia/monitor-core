#include <gm_metric.h>
#include <libmetrics.h>

mmodule sys_module;

static int sys_metric_init ( apr_pool_t *p )
{
    int i;

    libmetrics_init();

    for (i = 0; sys_module.metrics_info[i].name != NULL; i++) {
        /* Initialize the metadata storage for each of the metrics and then
         *  store one or more key/value pairs.  The define MGROUPS defines
         *  the key for the grouping attribute. */
        MMETRIC_INIT_METADATA(&(sys_module.metrics_info[i]),p);
        MMETRIC_ADD_METADATA(&(sys_module.metrics_info[i]),MGROUP,"system");
    }

    return 0;
}

static void sys_metric_cleanup ( void )
{
}

static g_val_t sys_metric_handler ( int metric_index )
{
    g_val_t val;

    /* The metric_index corresponds to the order in which
       the metrics appear in the metric_info array
    */
    switch (metric_index) {
    case 0:
        return boottime_func();
    case 1:
        return sys_clock_func();
    case 2:
        return machine_type_func();
    case 3:
        return os_name_func();
    case 4:
        return os_release_func();
    case 5:
        return mtu_func();
    }

    /* default case */
    val.uint32 = 0;
    return val;
}

static Ganglia_25metric sys_metric_info[] = 
{
    {0, "boottime",     1200, GANGLIA_VALUE_UNSIGNED_INT, "s", "zero", "%u", UDP_HEADER_SIZE+8,  "The last time that the system was started"},
    {0, "sys_clock",    1200, GANGLIA_VALUE_UNSIGNED_INT, "s", "zero", "%u", UDP_HEADER_SIZE+8,  "Time as reported by the system clock"},
    {0, "machine_type", 1200, GANGLIA_VALUE_STRING,       "",  "zero", "%s", UDP_HEADER_SIZE+32, "System architecture"},
    {0, "os_name",      1200, GANGLIA_VALUE_STRING,       "",  "zero", "%s", UDP_HEADER_SIZE+32, "Operating system name"},
    {0, "os_release",   1200, GANGLIA_VALUE_STRING,       "",  "zero", "%s", UDP_HEADER_SIZE+32, "Operating system release date"},
    {0, "mtu",          1200, GANGLIA_VALUE_UNSIGNED_INT, "",  "both", "%u", UDP_HEADER_SIZE+8,  "Network maximum transmission unit"},
    {0, NULL}

};

mmodule sys_module =
{
    STD_MMODULE_STUFF,
    sys_metric_init,
    sys_metric_cleanup,
    sys_metric_info,
    sys_metric_handler,
};

