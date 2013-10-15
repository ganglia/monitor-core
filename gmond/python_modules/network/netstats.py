import sys
import re
import time
import copy
import string

PARAMS = {}

METRICS = {
    'time' : 0,
    'data' : {}
}

stats_files = [ "/proc/net/netstat", "/proc/net/snmp" ]

LAST_METRICS = copy.deepcopy(METRICS)
METRICS_CACHE_MAX = 5

stats_pos = {} 

def get_metrics():
    """Return all metrics"""

    global METRICS, LAST_METRICS

    if (time.time() - METRICS['time']) > METRICS_CACHE_MAX:

	new_metrics = {}

	for file in stats_files:
	    try:
		file = open(file, 'r')
	
	    except IOError:
		return 0
    
	    # convert to dict
	    metrics = {}
	    for line in file:
		if re.match("(.*): [0-9]", line):
		    count = 0
		    metrics = re.split("\s+", line)
		    metric_group = metrics[0].replace(":", "").lower()
		    new_metrics[metric_group] = dict()
		    for value in metrics:
			# Skip first
			if count > 0 and value >= 0 and count in stats_pos[metric_group]:
			    metric_name = stats_pos[metric_group][count]
			    new_metrics[metric_group][metric_name] = value
			count += 1

	    file.close()

        # update cache
        LAST_METRICS = copy.deepcopy(METRICS)
        METRICS = {
            'time': time.time(),
            'data': new_metrics
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

    parts = name.split("_")
    group = parts[0]
    metric = "_".join(parts[1:])

    try:
      delta = (float(curr_metrics['data'][group][metric]) - float(last_metrics['data'][group][metric])) /(curr_metrics['time'] - last_metrics['time'])
      if delta < 0:
	print name + " is less 0"
	delta = 0
    except KeyError:
      delta = 0.0      

    return delta


def get_tcploss_percentage(name):

    # get metrics
    [curr_metrics, last_metrics] = get_metrics()

    try:
      pct = 100 * (float(curr_metrics['data']['tcpext']["tcploss"]) - float(last_metrics["data"]['tcpext']["tcploss"])) / (float(curr_metrics['data']['tcp']['outsegs']) +  float(curr_metrics['data']['tcp']['insegs']) - float(last_metrics['data']['tcp']['insegs']) - float(last_metrics['data']['tcp']['outsegs']))
      if pct < 0:
	print name + " is less 0"
	pct = 0
    except KeyError:
      pct = 0.0
    except ZeroDivisionError:
      pct = 0.0

    return pct

def get_retrans_percentage(name):

    # get metrics
    [curr_metrics, last_metrics] = get_metrics()

    try:
      pct = 100 * (float(curr_metrics['data']['tcp']["retranssegs"]) - float(last_metrics['data']['tcp']["retranssegs"])) / (float(curr_metrics['data']['tcp']['outsegs']) +  float(curr_metrics['data']['tcp']['insegs']) - float(last_metrics['data']['tcp']['insegs']) - float(last_metrics['data']['tcp']['outsegs']))
      if pct < 0:
	print name + " is less 0"
	pct = 0
    except KeyError:
      pct = 0.0
    except ZeroDivisionError:
      pct = 0.0

    return pct


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
        'call_back'   : get_delta,
        'time_max'    : 60,
        'value_type'  : 'float',
        'format'      : '%.5f',
        'units'       : 'count/s',
        'slope'       : 'both', # zero|positive|negative|both
        'description' : 'XXX',
        'groups'      : 'XXX',
        }

    ####################################################################################
    # Let's figure out what metrics are available
    #
    # Read /proc/net/netstat
    ####################################################################################
    for file in stats_files:
	try:
	    file = open(file, 'r')
    
	except IOError:
	    return 0
	
	# Find mapping
	for line in file:
	    # Lines with 
	    if not re.match("(.*): [0-9]", line):
		count = 0
		mapping = re.split("\s+", line)
		metric_group = mapping[0].replace(":", "").lower()
		stats_pos[metric_group] = dict()
		for metric in mapping:
		    # Skip first 
		    if count > 0 and metric != "":
			lowercase_metric = metric.lower()
			stats_pos[metric_group][count] = lowercase_metric
		    count += 1
    
	file.close()

    for group in stats_pos:
	for item in stats_pos[group]:
	    descriptors.append(create_desc(Desc_Skel, {
		    "name"       : group + "_" + stats_pos[group][item],
		    "description": stats_pos[group][item],
		    'groups'	 : group
		    }))

    descriptors.append(create_desc(Desc_Skel, {
	"name"       : "tcpext_" + "tcploss_percentage",
	"call_back"  : get_tcploss_percentage,
	"description": "TCP percentage loss, tcploss / insegs + outsegs",
	"units"      : "pct",
        'groups'      : 'tcpext'
	}))

    descriptors.append(create_desc(Desc_Skel, {
	"name"       : "tcp_" + "retrans_percentage",
	"call_back"  : get_retrans_percentage,
	"description": "TCP retrans percentage, retranssegs / insegs + outsegs",
	"units"      : "pct",
        'groups'      : 'tcp'
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
