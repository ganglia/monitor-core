import sys
#import traceback
#import os
import re
import time

PARAMS = {}

NAME_PREFIX = 'vm_'

METRICS = {
    'time' : 0,
    'data' : {}
}
LAST_METRICS = dict(METRICS)
METRICS_CACHE_MAX = 5

###############################################################################
# Explanation of metrics in /proc/meminfo can be found here
#
# http://www.redhat.com/advice/tips/meminfo.html
# and
# http://unixfoo.blogspot.com/2008/02/know-about-procmeminfo.html
# and
# http://www.centos.org/docs/5/html/5.2/Deployment_Guide/s2-proc-meminfo.html
###############################################################################
vminfo_file = "/proc/vmstat"


def get_metrics():
    """Return all metrics"""

    global METRICS, LAST_METRICS

    if (time.time() - METRICS['time']) > METRICS_CACHE_MAX:

	try:
	    file = open(vminfo_file, 'r')
    
	except IOError:
	    return 0

        # convert to dict
        metrics = {}
        for line in file:
            parts = re.split("\s+", line)
            metrics[parts[0]] = parts[1]

        # update cache
        LAST_METRICS = dict(METRICS)
        METRICS = {
            'time': time.time(),
            'data': metrics
        }

    return [METRICS, LAST_METRICS]

def get_value(name):
    """Return a value for the requested metric"""

    metrics = get_metrics()[0]

    name = name[len(NAME_PREFIX):] # remove prefix from name

    try:
        result = metrics['data'][name]
    except StandardError:
        result = 0

    return result


def get_delta(name):
    """Return change over time for the requested metric"""

    # get metrics
    [curr_metrics, last_metrics] = get_metrics()

    name = name[len(NAME_PREFIX):] # remove prefix from name

    try:
      delta = (float(curr_metrics['data'][name]) - float(last_metrics['data'][name])) /(curr_metrics['time'] - last_metrics['time'])
      if delta < 0:
	print name + " is less 0"
	delta = 0
    except KeyError:
      delta = 0.0      

    return delta


# Calculate VM efficiency
# Works similar like sar -B 1
# Calculated as pgsteal / pgscan, this is a metric of the efficiency of page reclaim. If  it  is  near  100%  then
# almost  every  page coming off the tail of the inactive list is being reaped. If it gets too low (e.g. less than 30%)  
# then the virtual memory is having some difficulty.  This field is displayed as zero if no pages  have  been
# scanned during the interval of time
def get_vmeff(name):  
    # get metrics
    [curr_metrics, last_metrics] = get_metrics()
        
    try:
      pgscan_diff = float(curr_metrics['data']['pgscan_kswapd_normal']) - float(last_metrics['data']['pgscan_kswapd_normal'])
      # To avoid division by 0 errors check whether pgscan is 0
      if pgscan_diff == 0:
	return 0.0
	
      delta = 100 * (float(curr_metrics['data']['pgsteal_normal']) - float(last_metrics['data']['pgsteal_normal'])) / pgscan_diff
      if delta < 0:
        print name + " is less 0"
        delta = 0
    except KeyError:
      delta = 0.0      

    return delta
  

def create_desc(skel, prop):
    d = skel.copy()
    for k,v in prop.iteritems():
        d[k] = v
    return d

def metric_init(params):
    global descriptors, metric_map, Desc_Skel

    descriptors = []

    Desc_Skel = {
        'name'        : 'XXX',
        'call_back'   : get_value,
        'time_max'    : 60,
        'value_type'  : 'float',
        'format'      : '%.4f',
        'units'       : 'count',
        'slope'       : 'both', # zero|positive|negative|both
        'description' : 'XXX',
        'groups'      : 'memory_vm',
        }

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_inactive_anon",
                "description": "nr_inactive_anon",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_active_anon",
                "description": "nr_active_anon",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_inactive_file",
                "description": "nr_inactive_file",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_active_file",
                "description": "nr_active_file",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_unevictable",
                "description": "nr_unevictable",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_mlock",
                "description": "nr_mlock",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_anon_pages",
                "description": "nr_anon_pages",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_mapped",
                "description": "nr_mapped",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_file_pages",
                "description": "nr_file_pages",
                }))

    #
    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_dirty",
                "description": "nr_dirty",
                }))

    #
    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_writeback",
                "description": "nr_writeback",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_slab_reclaimable",
                "description": "nr_slab_reclaimable",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_slab_unreclaimable",
                "description": "nr_slab_unreclaimable",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_page_table_pages",
                "description": "nr_page_table_pages",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_kernel_stack",
                "description": "nr_kernel_stack",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_unstable",
                "description": "nr_unstable",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_bounce",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "nr_bounce",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_vmscan_write",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "nr_vmscan_write",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_writeback_temp",
                "units"      : "ops/s",
                "description": "nr_writeback_temp",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_isolated_anon",
                "units"      : "ops/s",
                "description": "nr_isolated_anon",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_isolated_file",
                "units"      : "ops/s",
                "description": "nr_isolated_file",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_shmem",
                "units"      : "ops/s",
                "description": "nr_shmem",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "numa_hit",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "numa_hit",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "numa_miss",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "numa_miss",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "numa_foreign",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "numa_foreign",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "numa_interleave",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "numa_interleave",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "numa_local",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "numa_local",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "numa_other",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "numa_other",
                }))

    #
    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgpgin",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgpgin",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgpgout",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgpgout",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pswpin",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pswpin",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pswpout",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pswpout",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgalloc_dma",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgalloc_dma",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgalloc_dma32",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgalloc_dma32",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgalloc_normal",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgalloc_normal",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgalloc_movable",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgalloc_movable",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgfree",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgfree",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgactivate",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgactivate",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgdeactivate",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgdeactivate",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgfault",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgfault",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgmajfault",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgmajfault",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgrefill_dma",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgrefill_dma",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgrefill_dma32",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgrefill_dma32",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgrefill_normal",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgrefill_normal",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgrefill_movable",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgrefill_movable",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgsteal_dma",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgsteal_dma",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgsteal_dma32",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgsteal_dma32",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgsteal_normal",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgsteal_normal",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgsteal_movable",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgsteal_movable",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgscan_kswapd_dma",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgscan_kswapd_dma",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgscan_kswapd_dma32",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgscan_kswapd_dma32",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgscan_kswapd_normal",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgscan_kswapd_normal",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgscan_kswapd_movable",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgscan_kswapd_movable",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgscan_direct_dma",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgscan_direct_dma",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgscan_direct_dma32",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgscan_direct_dma32",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgscan_direct_normal",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgscan_direct_normal",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgscan_direct_movable",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgscan_direct_movable",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "zone_reclaim_failed",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "zone_reclaim_failed",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pginodesteal",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pginodesteal",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "slabs_scanned",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "slabs_scanned",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "kswapd_steal",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "kswapd_steal",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "kswapd_inodesteal",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "kswapd_inodesteal",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "kswapd_low_wmark_hit_quickly",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "kswapd_low_wmark_hit_quickly",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "kswapd_high_wmark_hit_quickly",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "kswapd_high_wmark_hit_quickly",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "kswapd_skip_congestion_wait",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "kswapd_skip_congestion_wait",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pageoutrun",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pageoutrun",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "allocstall",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "allocstall",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "pgrotated",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "pgrotated",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "unevictable_pgs_culled",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "unevictable_pgs_culled",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "unevictable_pgs_scanned",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "unevictable_pgs_scanned",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "unevictable_pgs_rescued",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "unevictable_pgs_rescued",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "unevictable_pgs_mlocked",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "unevictable_pgs_mlocked",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "unevictable_pgs_munlocked",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "unevictable_pgs_munlocked",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "unevictable_pgs_cleared",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "unevictable_pgs_cleared",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "unevictable_pgs_stranded",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "unevictable_pgs_stranded",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "unevictable_pgs_mlockfreed",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "unevictable_pgs_mlockfreed",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_dirtied",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "nr_dirtied",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_written",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "nr_written",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_anon_transparent_hugepages",
                "description": "nr_anon_transparent_hugepages",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_dirty_threshold",
                "description": "nr_dirty_threshold",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "nr_dirty_background_threshold",
                "description": "nr_dirty_background_threshold",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "compact_blocks_moved",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "compact_blocks_moved",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "compact_pages_moved",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "compact_pages_moved",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "compact_pagemigrate_failed",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "compact_pagemigrate_failed",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "compact_stall",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "compact_stall",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "compact_fail",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "compact_fail",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "compact_success",
                "call_back"  : get_delta,
                "units"      : "ops/s",
                "description": "compact_success",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + "vmeff",
                "description": "VM efficiency",
                'call_back'   : get_vmeff,
                'units'       : 'pct',
                }))

    return descriptors

def metric_cleanup():
    '''Clean up the metric module.'''
    pass

#This code is for debugging and unit testing
if __name__ == '__main__':
    descriptors = metric_init(PARAMS)
    while True:
        for d in descriptors:
            v = d['call_back'](d['name'])
            print '%s = %s' % (d['name'],  v)
        print 'Sleeping 15 seconds'
        time.sleep(15)
