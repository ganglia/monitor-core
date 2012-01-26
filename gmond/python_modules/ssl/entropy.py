import sys


entropy_file = "/proc/sys/kernel/random/entropy_avail"

def metrics_handler(name):  
    try:
        f = open(entropy_file, 'r')

    except IOError:
        return 0

    for l in f:
        line = l

    return int(line)

def metric_init(params):
    global descriptors, node_id

    dict = {'name': 'entropy_avail',
        'call_back': metrics_handler,
        'time_max': 90,
        'value_type': 'uint',
        'units': 'bits',
        'slope': 'both',
        'format': '%u',
        'description': 'Entropy Available',
        'groups': 'ssl'}

    descriptors = [dict]

    return descriptors

def metric_cleanup():
    '''Clean up the metric module.'''
    pass

#This code is for debugging and unit testing
if __name__ == '__main__':
    metric_init({})
    for d in descriptors:
        v = d['call_back'](d['name'])
        print 'value for %s is %u' % (d['name'],  v)