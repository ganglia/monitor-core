import sys
import traceback
import os
import re


###############################################################################
# Explanation of metrics in /proc/meminfo can be found here
#
# http://www.redhat.com/advice/tips/meminfo.html
# and
# http://unixfoo.blogspot.com/2008/02/know-about-procmeminfo.html
# and
# http://www.centos.org/docs/5/html/5.2/Deployment_Guide/s2-proc-meminfo.html
###############################################################################

meminfo_file = "/proc/meminfo"

def metrics_handler(name):  
    try:
        file = open(meminfo_file, 'r')

    except IOError:
        return 0

    value = 0
    for line in file:
	parts = re.split("\s+", line)
	if parts[0] == metric_map[name]['name'] + ":" :
	    # All of the measurements are in kBytes. We want to change them over
	    # to Bytes
	    if metric_map[name]['units'] == "Bytes":
		value = float(parts[1]) * 1024
	    else:
                value = parts[1]
	
    return float(value)

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
        'orig_name'   : 'XXX',
        'call_back'   : metrics_handler,
        'time_max'    : 60,
        'value_type'  : 'float',
        'format'      : '%.0f',
        'units'       : 'XXX',
        'slope'       : 'both', # zero|positive|negative|both
        'description' : 'XXX',
        'groups'      : 'memory',
        }

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : "mem_total",
                "orig_name"  : "MemTotal",
                "units"      : "Bytes",
                "description": "Total usable ram",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_free",
                "orig_name"  : "MemFree",
                "units"      : "Bytes",
                "description": "The amount of physical RAM left unused by the system. ",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_buffers",
                "orig_name"  : "Buffers",
                "units"      : "Bytes",
                "description": "Buffers used",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_cached",
                "orig_name"  : "Cached",
                "units"      : "Bytes",
                "description": "Cached Memory",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_swap_cached",
                "orig_name"  : "SwapCached",
                "units"      : "Bytes",
                "description": "Amount of Swap used as cache memory. Memory that once was swapped out, is swapped back in, but is still in the swapfile",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_active",
                "orig_name"  : "Active",
                "units"      : "Bytes",
                "description": "Memory that has been used more recently and usually not reclaimed unless absolutely necessary.",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_inactive",
                "orig_name"  : "Inactive",
                "units"      : "Bytes",
                "description": "The total amount of buffer or page cache memory that are free and available",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_total",
                "orig_name"  : "Active(anon)",
                "units"      : "Bytes",
                "description": "Active(anon)",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_inactive_anon",
                "orig_name"  : "Inactive(anon)",
                "units"      : "Bytes",
                "description": "Inactive(anon)",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_active_file",
                "orig_name"  : "Active(file)",
                "units"      : "Bytes",
                "description": "Active(file)",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_inactive_file",
                "orig_name"  : "Inactive(file)",
                "units"      : "Bytes",
                "description": "Inactive(file)",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_unevictable",
                "orig_name"  : "Unevictable",
                "units"      : "Bytes",
                "description": "Unevictable",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_mlocked",
                "orig_name"  : "Mlocked",
                "units"      : "Bytes",
                "description": "Mlocked",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_swap_total",
                "orig_name"  : "SwapTotal",
                "units"      : "Bytes",
                "description": "Total amount of physical swap memory",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_swap_free",
                "orig_name"  : "SwapFree",
                "units"      : "Bytes",
                "description": "Total amount of swap memory free",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_dirty",
                "orig_name"  : "Dirty",
                "units"      : "Bytes",
                "description": "The total amount of memory waiting to be written back to the disk. ",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_writeback",
                "orig_name"  : "Writeback",
                "units"      : "Bytes",
                "description": "The total amount of memory actively being written back to the disk.",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_anonpages",
                "orig_name"  : "AnonPages",
                "units"      : "Bytes",
                "description": "AnonPages",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_mapped",
                "orig_name"  : "Mapped",
                "units"      : "Bytes",
                "description": "Mapped",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_shmem",
                "orig_name"  : "Shmem",
                "units"      : "Bytes",
                "description": "Shmem",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_slab",
                "orig_name"  : "Slab",
                "units"      : "Bytes",
                "description": "Slab",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_s_reclaimable",
                "orig_name"  : "SReclaimable",
                "units"      : "Bytes",
                "description": "SReclaimable",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_s_unreclaimable",
                "orig_name"  : "SUnreclaim",
                "units"      : "Bytes",
                "description": "SUnreclaim",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_kernel_stack",
                "orig_name"  : "KernelStack",
                "units"      : "Bytes",
                "description": "KernelStack",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_page_tables",
                "orig_name"  : "PageTables",
                "units"      : "Bytes",
                "description": "PageTables",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_nfs_unstable",
                "orig_name"  : "NFS_Unstable",
                "units"      : "Bytes",
                "description": "NFS_Unstable",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_bounce",
                "orig_name"  : "Bounce",
                "units"      : "Bytes",
                "description": "Bounce",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_writeback_tmp",
                "orig_name"  : "WritebackTmp",
                "units"      : "Bytes",
                "description": "WritebackTmp",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_commit_limit",
                "orig_name"  : "CommitLimit",
                "units"      : "Bytes",
                "description": "CommitLimit",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_committed_as",
                "orig_name"  : "Committed_AS",
                "units"      : "Bytes",
                "description": "Committed_AS",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_vmalloc_total",
                "orig_name"  : "VmallocTotal",
                "units"      : "Bytes",
                "description": "VmallocTotal",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_vmalloc_used",
                "orig_name"  : "VmallocUsed",
                "units"      : "Bytes",
                "description": "VmallocUsed",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_vmalloc_chunk",
                "orig_name"  : "VmallocChunk",
                "units"      : "Bytes",
                "description": "VmallocChunk",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_hardware_corrupted",
                "orig_name"  : "HardwareCorrupted",
                "units"      : "Bytes",
                "description": "HardwareCorrupted",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_hugepages_total",
                "orig_name"  : "HugePages_Total",
                "units"      : "pages",
                "description": "HugePages_Total",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_hugepages_free",
                "orig_name"  : "HugePages_Free",
                "units"      : "pages",
                "description": "HugePages_Free",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_hugepage_rsvd",
                "orig_name"  : "HugePages_Rsvd",
                "units"      : "pages",
                "description": "HugePages_Rsvd",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_hugepages_surp",
                "orig_name"  : "HugePages_Surp",
                "units"      : "pages",
                "description": "HugePages_Surp",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_hugepage_size",
                "orig_name"  : "Hugepagesize",
                "units"      : "Bytes",
                "description": "Hugepagesize",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_directmap_4k",
                "orig_name"  : "DirectMap4k",
                "units"      : "Bytes",
                "description": "DirectMap4k",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "mem_directmap_2M",
                "orig_name"  : "DirectMap2M",
                "units"      : "Bytes",
                "description": "DirectMap2M",
                }))

    # We need a metric_map that maps metric_name to the index in /proc/meminfo
    metric_map = {}
    
    for d in descriptors:
	metric_name = d['name']
        metric_map[metric_name] = { "name": d['orig_name'], "units": d['units'] }
        
    return descriptors

def metric_cleanup():
    '''Clean up the metric module.'''
    pass

#This code is for debugging and unit testing
if __name__ == '__main__':
    metric_init({})
    for d in descriptors:
        v = d['call_back'](d['name'])
        print 'value for %s is %f' % (d['name'],  v)