import sys
import traceback
import os
import re
import time
import copy

METRICS = {
    'time' : 0,
    'data' : {}
}

# Got these from /proc/softirqs
softirq_pos = {
  'hi' : 1,
  'timer' : 2,
  'nettx' : 3,
  'netrx' : 4,
  'block' : 5,
  'blockiopoll' : 6,
  'tasklet' : 7,
  'sched' : 8,
  'hrtimer' : 9,
  'rcu' : 10
}

LAST_METRICS = copy.deepcopy(METRICS)
METRICS_CACHE_MAX = 5



stat_file = "/proc/stat"

###############################################################################
#
###############################################################################
def get_metrics():
    """Return all metrics"""

    global METRICS, LAST_METRICS

    if (time.time() - METRICS['time']) > METRICS_CACHE_MAX:

	try:
	    file = open(stat_file, 'r')
    
	except IOError:
	    return 0

        # convert to dict
        metrics = {}
        for line in file:
            parts = re.split("\s+", line)
            metrics[parts[0]] = list(parts[1:])

        # update cache
        LAST_METRICS = copy.deepcopy(METRICS)
        METRICS = {
            'time': time.time(),
            'data': metrics
        }

    return [METRICS, LAST_METRICS]


def get_value(name):
    """Return a value for the requested metric"""

    metrics = get_metrics()[0]

    NAME_PREFIX="cpu_"

    name = name.replace(NAME_PREFIX,"") # remove prefix from name

    try:
        result = metrics['data'][name][0]
    except StandardError:
        result = 0

    return result


def get_delta(name):
    """Return change over time for the requested metric"""

    # get metrics
    [curr_metrics, last_metrics] = get_metrics()

    NAME_PREFIX="cpu_"

    name = name.replace(NAME_PREFIX,"") # remove prefix from name

    if name == "procs_created":
      name = "processes"

    try:
      delta = (float(curr_metrics['data'][name][0]) - float(last_metrics['data'][name][0])) /(curr_metrics['time'] - last_metrics['time'])
      if delta < 0:
	print name + " is less 0"
	delta = 0
    except KeyError:
      delta = 0.0      

    return delta

##############################################################################
# SoftIRQ has multiple values which are defined in a dictionary at the top
##############################################################################
def get_softirq_delta(name):
    """Return change over time for the requested metric"""

    # get metrics
    [curr_metrics, last_metrics] = get_metrics()

    NAME_PREFIX="softirq_"

    name = name[len(NAME_PREFIX):] # remove prefix from name

    index = softirq_pos[name]

    try:
      delta = (float(curr_metrics['data']['softirq'][index]) - float(last_metrics['data']['softirq'][index])) /(curr_metrics['time'] - last_metrics['time'])
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
        'orig_name'   : 'XXX',
        'call_back'   : get_delta,
        'time_max'    : 60,
        'value_type'  : 'float',
        'format'      : '%.0f',
        'units'       : 'XXX',
        'slope'       : 'both', # zero|positive|negative|both
        'description' : '',
        'groups'      : 'cpu',
        }

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "cpu_ctxt",
                "units"      : "ctxs/sec",
                "description": "Context Switches",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "procs_created",
                "units"      : "proc/sec",
                "description": "Number of processes and threads created",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "cpu_intr",
                "units"      : "intr/sec",
                "description": "Interrupts serviced",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "procs_blocked",
                "units"      : "processes",
                "call_back"   : get_value,
                "description": "Processes blocked",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "softirq",
                "units"      : "ops/s",
                "description": "Soft IRQs",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "softirq_hi",
                "units"      : "ops/s",
                'groups'     : 'softirq',
                "call_back"   : get_softirq_delta
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "softirq_timer",
                "units"      : "ops/s",
                'groups'     : 'softirq',
                "call_back"   : get_softirq_delta
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "softirq_nettx",
                "units"      : "ops/s",
                'groups'     : 'softirq',
                "call_back"   : get_softirq_delta
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "softirq_netrx",
                "units"      : "ops/s",
                'groups'     : 'softirq',
                "call_back"   : get_softirq_delta
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "softirq_block",
                "units"      : "ops/s",
                'groups'     : 'softirq',
                "call_back"   : get_softirq_delta
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "softirq_blockiopoll",
                "units"      : "ops/s",
                'groups'     : 'softirq',
                "call_back"   : get_softirq_delta
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "softirq_tasklet",
                "units"      : "ops/s",
                'groups'     : 'softirq',
                "call_back"   : get_softirq_delta
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "softirq_sched",
                "units"      : "ops/s",
                'groups'     : 'softirq',
                "call_back"   : get_softirq_delta
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "softirq_hrtimer",
                "units"      : "ops/s",
                'groups'     : 'softirq',
                "call_back"   : get_softirq_delta
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : "softirq_rcu",
                "units"      : "ops/s",
                'groups'     : 'softirq',
                "call_back"   : get_softirq_delta
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
    while True:
        for d in descriptors:
            v = d['call_back'](d['name'])
            print '%s = %s' % (d['name'],  v)
        print 'Sleeping 15 seconds'
        time.sleep(5)
