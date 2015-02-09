# This module allows you to collect available entropy on your system. Why is
# entropy important.
# [http://www.chrissearle.org/node/326]
#   There are two random number sources on linux - /dev/random and /dev/urandom.
#   /dev/random will block if there is nothing left in the entropy bit bucket.
#   /dev/urandom uses the same bucket - but will not block
#   (it can reuse the pool of bits).
# Therefore if you are running SSL on the box you want to know this.

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
