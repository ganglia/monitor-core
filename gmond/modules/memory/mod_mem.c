#include <gm_metric.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <apr_strings.h>
#include <libmetrics.h>

mmodule mem_module;
static int num_numa_nodes = 0;
static Ganglia_25metric *dynamic_metric_info = NULL;

static int mem_metric_init ( apr_pool_t *p )
{
    int i, j;
    struct dirent *entry;
    DIR *dir;

    libmetrics_init();

    for (i = 0; mem_module.metrics_info[i].name != NULL; i++) {
        MMETRIC_INIT_METADATA(&(mem_module.metrics_info[i]),p);
        MMETRIC_ADD_METADATA(&(mem_module.metrics_info[i]),MGROUP,"memory");
    }

    #ifdef LINUX
    /* Count NUMA nodes dynamically */
    dir = opendir("/sys/devices/system/node");
    if (dir) {
        while ((entry = readdir(dir)) != NULL) {
            if (strncmp(entry->d_name, "node", 4) == 0 && entry->d_name[4] >= '0' && entry->d_name[4] <= '9') {
                num_numa_nodes++;
            }
        }
        closedir(dir);
    }

    if (num_numa_nodes > 0) {
        int base_count = 0;
        while (mem_module.metrics_info[base_count].name != NULL) base_count++;

        dynamic_metric_info = apr_pcalloc(p, (base_count + num_numa_nodes + 1) * sizeof(Ganglia_25metric));
        memcpy(dynamic_metric_info, mem_module.metrics_info, base_count * sizeof(Ganglia_25metric));

        for (j = 0; j < num_numa_nodes; j++) {
            char *name = apr_pstrcat(p, "mem_SUnreclaim_node", apr_itoa(p, j), NULL);
            char *desc = apr_pstrcat(p, "Amount of un-reclaimable slab memory on NUMA node ", apr_itoa(p, j), NULL);
            
            dynamic_metric_info[base_count + j].key = base_count + j;
            dynamic_metric_info[base_count + j].name = name;
            dynamic_metric_info[base_count + j].msg_size = UDP_HEADER_SIZE+8;
            dynamic_metric_info[base_count + j].type = GANGLIA_VALUE_FLOAT;
            dynamic_metric_info[base_count + j].units = "KB";
            dynamic_metric_info[base_count + j].slope = "both";
            dynamic_metric_info[base_count + j].fmt = "%.0f";
            dynamic_metric_info[base_count + j].desc = desc;
            
            MMETRIC_INIT_METADATA(&(dynamic_metric_info[base_count + j]), p);
            MMETRIC_ADD_METADATA(&(dynamic_metric_info[base_count + j]), MGROUP, "memory");
        }
        mem_module.metrics_info = dynamic_metric_info;
    }
    #endif

    return 0;
}

static void mem_metric_cleanup ( void )
{
}

static g_val_t mem_metric_handler ( int metric_index )
{
    g_val_t val;
    int base_count = 10; 

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
#ifdef LINUX
    case 7:
        return mem_sreclaimable_func();
    case 8:
        return mem_slab_func();
    case 9:
        return mem_available_func();
#endif
    default:
        #ifdef LINUX
        if (metric_index >= base_count && metric_index < (base_count + num_numa_nodes)) {
            int node_id = metric_index - base_count;
            char path[128];
            FILE *fp;
            
            val.f = 0.0;
            snprintf(path, sizeof(path), "/sys/devices/system/node/node%d/meminfo", node_id);
            fp = fopen(path, "r");
            if (fp) {
                char line[256];
                while (fgets(line, sizeof(line), fp)) {
                    if (strstr(line, "SUnreclaim:")) {
                        char *p = strchr(line, ':');
                        if (p) val.f = atof(p + 1);
                        break;
                    }
                }
                fclose(fp);
            }
            return val;
        }
        #endif
        break;
    }

    val.f = 0;
    return val;
}

static Ganglia_25metric mem_metric_info[] = 
{
    {0, "mem_total",  1200, GANGLIA_VALUE_FLOAT, "KB", "zero", "%.0f", UDP_HEADER_SIZE+8, "Total amount of memory displayed in KBs"},
    {0, "mem_free",    180, GANGLIA_VALUE_FLOAT, "KB", "both", "%.0f", UDP_HEADER_SIZE+8, "Amount of available memory"},
    {0, "mem_shared",  180, GANGLIA_VALUE_FLOAT, "KB", "both", "%.0f", UDP_HEADER_SIZE+8, "Amount of shared memory"},
    {0, "mem_buffers", 180, GANGLIA_VALUE_FLOAT, "KB", "both", "%.0f", UDP_HEADER_SIZE+8, "Amount of buffered memory"},
    {0, "mem_cached",  180, GANGLIA_VALUE_FLOAT, "KB", "both", "%.0f", UDP_HEADER_SIZE+8, "Amount of cached memory"},
    {0, "swap_free",   180, GANGLIA_VALUE_FLOAT, "KB", "both", "%.0f", UDP_HEADER_SIZE+8, "Amount of available swap memory"},
    {0, "swap_total", 1200, GANGLIA_VALUE_FLOAT, "KB", "zero", "%.0f", UDP_HEADER_SIZE+8, "Total amount of swap space displayed in KBs"},
#ifdef LINUX
    {0, "mem_sreclaimable", 180, GANGLIA_VALUE_FLOAT, "KB", "both", "%.0f", UDP_HEADER_SIZE+8, "Amount of reclaimable slab memory"},
    {0, "mem_slab",    180, GANGLIA_VALUE_FLOAT, "KB", "both", "%.0f", UDP_HEADER_SIZE+8, "Amount of in-kernel data structures cache"},
    {0, "mem_available",    180, GANGLIA_VALUE_FLOAT, "KB", "both", "%.0f", UDP_HEADER_SIZE+8, "Estimate of how much memory is available"},
#endif
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
