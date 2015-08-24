#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import threading
import time
import urllib2
import traceback
import re
import copy
import sys
import socket

if sys.version_info < (2, 6):
    socket.setdefaulttimeout(2)

# global to store state for "total accesses"
METRICS = {
    'time': 0,
    'data': {}
}

LAST_METRICS = copy.deepcopy(METRICS)
METRICS_CACHE_MAX = 5

#Metric prefix
NAME_PREFIX = "ap_"
SSL_NAME_PREFIX = "apssl_"

SERVER_STATUS_URL = ""

descriptors = list()
Desc_Skel = {}
Scoreboard = {
    NAME_PREFIX + 'waiting'         : {'key': '_', 'desc': 'Waiting for Connection'},
    NAME_PREFIX + 'starting'        : {'key': 'S', 'desc': 'Starting up'},
    NAME_PREFIX + 'reading_request' : {'key': 'R', 'desc': 'Reading Request'},
    NAME_PREFIX + 'sending_reply'   : {'key': 'W', 'desc': 'Sending Reply'},
    NAME_PREFIX + 'keepalive'       : {'key': 'K', 'desc': 'Keepalive (read)'},
    NAME_PREFIX + 'dns_lookup'      : {'key': 'D', 'desc': 'DNS Lookup'},
    NAME_PREFIX + 'closing'         : {'key': 'C', 'desc': 'Closing connection'},
    NAME_PREFIX + 'logging'         : {'key': 'L', 'desc': 'Logging'},
    NAME_PREFIX + 'gracefully_fin'  : {'key': 'G', 'desc': 'Gracefully finishing'},
    NAME_PREFIX + 'idle'            : {'key': 'I', 'desc': 'Idle cleanup of worker'},
    NAME_PREFIX + 'open_slot'       : {'key': '.', 'desc': 'Open slot with no current process'},
    }
Scoreboard_bykey = dict([(v["key"], k) for (k, v) in Scoreboard.iteritems()])

SSL_REGEX = re.compile('^(cache type:) (.*)(<b>)(?P<shared_mem>[0-9]+)(</b> bytes, current sessions: <b>)(?P<current_sessions>[0-9]+)(</b><br>subcaches: <b>)(?P<num_subcaches>[0-9]+)(</b>, indexes per subcache: <b>)(?P<indexes_per_subcache>[0-9]+)(</b><br>)(.*)(<br>index usage: <b>)(?P<index_usage>[0-9]+)(%</b>, cache usage: <b>)(?P<cache_usage>[0-9]+)(%</b><br>total sessions stored since starting: <b>)(?P<sessions_stored>[0-9]+)(</b><br>total sessions expired since starting: <b>)(?P<sessions_expired>[0-9]+)(</b><br>total \(pre-expiry\) sessions scrolled out of the cache: <b>)(?P<sessions_scrolled_outof_cache>[0-9]+)(</b><br>total retrieves since starting: <b>)(?P<retrieves_hit>[0-9]+)(</b> hit, <b>)(?P<retrieves_miss>[0-9]+)(</b> miss<br>total removes since starting: <b>)(?P<removes_hit>[0-9]+)(</b> hit, <b>)(?P<removes_miss>[0-9]+)')
# Good for Apache 2.2
#SSL_REGEX = re.compile('^(cache type:) (.*)(<b>)(?P<shared_mem>[0-9]+)(</b> bytes, current sessions: <b>)(?P<current_sessions>[0-9]+)(</b><br>subcaches: <b>)(?P<num_subcaches>[0-9]+)(</b>, indexes per subcache: <b>)(?P<indexes_per_subcache>[0-9]+)(</b><br>index usage: <b>)(?P<index_usage>[0-9]+)(%</b>, cache usage: <b>)(?P<cache_usage>[0-9]+)(%</b><br>total sessions stored since starting: <b>)(?P<sessions_stored>[0-9]+)(</b><br>total sessions expired since starting: <b>)(?P<sessions_expired>[0-9]+)(</b><br>total \(pre-expiry\) sessions scrolled out of the cache: <b>)(?P<sessions_scrolled_outof_cache>[0-9]+)(</b><br>total retrieves since starting: <b>)(?P<retrieves_hit>[0-9]+)(</b> hit, <b>)(?P<retrieves_miss>[0-9]+)(</b> miss<br>total removes since starting: <b>)(?P<removes_hit>[0-9]+)(</b> hit, <b>)(?P<removes_miss>[0-9]+)')


Metric_Map = {
    'Uptime' : NAME_PREFIX + "uptime",
    'IdleWorkers' : NAME_PREFIX + "idle_workers",
    'BusyWorkers' : NAME_PREFIX + "busy_workers",
    'Total kBytes' : NAME_PREFIX + "bytes",
    'CPULoad' : NAME_PREFIX + "cpuload",
    "Total Accesses" : NAME_PREFIX + "rps"
}


def get_metrics():

    global METRICS, LAST_METRICS, SERVER_STATUS_URL, COLLECT_SSL

    if (time.time() - METRICS['time']) > METRICS_CACHE_MAX:

        metrics = dict([(k, 0) for k in Scoreboard.keys()])

        # This is the short server-status. Lacks SSL metrics
        try:
            req = urllib2.Request(SERVER_STATUS_URL + "?auto")

            # Download the status file
            if sys.version_info < (2, 6):
                res = urllib2.urlopen(req)
            else:
                res = urllib2.urlopen(req, timeout=2)

            for line in res:
                split_line = line.rstrip().split(": ")
                long_metric_name = split_line[0]
                if long_metric_name == "Scoreboard":
                    for sck in split_line[1]:
                        metrics[ Scoreboard_bykey[sck] ] += 1
                else:
                    # Apache > 2.4.16 inserts the hostname as the first line, so ignore
                    if len(split_line) == 1:
                        continue
                    
                    if long_metric_name in Metric_Map:
                        metric_name = Metric_Map[long_metric_name]
                    else:
                        metric_name = long_metric_name
                    metrics[metric_name] = split_line[1]

        except urllib2.URLError:
            traceback.print_exc()

        # If we are collecting SSL metrics we'll do
        if COLLECT_SSL:

            try:
                req2 = urllib2.Request(SERVER_STATUS_URL)

                # Download the status file
                if sys.version_info < (2, 6):
                    res = urllib2.urlopen(req2)
                else:
                    res = urllib2.urlopen(req2, timeout=2)

                for line in res:
                    regMatch = SSL_REGEX.match(line)
                    if regMatch:
                        linebits = regMatch.groupdict()
                        for key in linebits:
                            #print SSL_NAME_PREFIX + key + "=" + linebits[key]
                            metrics[SSL_NAME_PREFIX + key] = linebits[key]

            except urllib2.URLError:
                traceback.print_exc()

        LAST_METRICS = copy.deepcopy(METRICS)
        METRICS = {
            'time': time.time(),
            'data': metrics
        }

    return [METRICS, LAST_METRICS]


def get_value(name):
    """Return a value for the requested metric"""

    metrics = get_metrics()[0]

    try:
        result = metrics['data'][name]
    except StandardError:
        result = 0

    return result


def get_delta(name):
    """Return change over time for the requested metric"""

    # get metrics
    [curr_metrics, last_metrics] = get_metrics()

    # If it's ap_bytes metric multiply result by 1024
    if name == NAME_PREFIX + "bytes":
        multiplier = 1024
    else:
        multiplier = 1

    try:
        delta = multiplier * (float(curr_metrics['data'][name]) - float(last_metrics['data'][name])) / (curr_metrics['time'] - last_metrics['time'])
        if delta < 0:
            print name + " is less 0"
            delta = 0
    except KeyError:
        delta = 0.0

    return delta


def create_desc(prop):
    d = Desc_Skel.copy()
    for k, v in prop.iteritems():
        d[k] = v
    return d


def metric_init(params):
    global descriptors, Desc_Skel, SERVER_STATUS_URL, COLLECT_SSL

    print '[apache_status] Received the following parameters'
    print params

    if "metric_group" not in params:
        params["metric_group"] = "apache"

    Desc_Skel = {
        'name'        : 'XXX',
        'call_back'   : get_value,
        'time_max'    : 60,
        'value_type'  : 'uint',
        'units'       : 'proc',
        'slope'       : 'both',
        'format'      : '%d',
        'description' : 'XXX',
        'groups'      : params["metric_group"],
        }

    if "refresh_rate" not in params:
        params["refresh_rate"] = 15

    if "url" not in params:
        params["url"] = "http://localhost:7070/server-status"

    if "collect_ssl" not in params:
        params["collect_ssl"] = False

    SERVER_STATUS_URL = params["url"]
    COLLECT_SSL = params["collect_ssl"]

    # IP:HOSTNAME
    if "spoof_host" in params:
        Desc_Skel["spoof_host"] = params["spoof_host"]

    descriptors.append(create_desc({
                "name"       : NAME_PREFIX + "rps",
                "value_type" : "float",
                "units"      : "req/sec",
                "call_back"   : get_delta,
                "format"     : "%.3f",
                "description": "request per second",
                }))

    descriptors.append(create_desc({
                "name"       : NAME_PREFIX + "bytes",
                "value_type" : "float",
                "units"      : "bytes/sec",
                "call_back"   : get_delta,
                "format"     : "%.3f",
                "description": "bytes transferred per second",
                }))

    descriptors.append(create_desc({
                "name"       : NAME_PREFIX + "cpuload",
                "value_type" : "float",
                "units"      : "pct",
                "format"     : "%.6f",
                "call_back"   : get_value,
                "description": "Pct of time CPU utilized",
                }))

    descriptors.append(create_desc({
                "name"       : NAME_PREFIX + "busy_workers",
                "value_type" : "uint",
                "units"      : "threads",
                "format"     : "%u",
                "call_back"   : get_value,
                "description": "Busy threads",
                }))

    descriptors.append(create_desc({
                "name"       : NAME_PREFIX + "idle_workers",
                "value_type" : "uint",
                "units"      : "threads",
                "format"     : "%u",
                "call_back"   : get_value,
                "description": "Idle threads",
                }))

    descriptors.append(create_desc({
                "name"       : NAME_PREFIX + "uptime",
                "value_type" : "uint",
                "units"      : "seconds",
                "format"     : "%u",
                "call_back"   : get_value,
                "description": "Uptime",
                }))

    for k, v in Scoreboard.iteritems():
        descriptors.append(create_desc({
                    "name"        : k,
                    "call_back"   : get_value,
                    "description" : v["desc"],
                    }))

    ##########################################################################
    # SSL metrics
    ##########################################################################
    if params['collect_ssl']:

        descriptors.append(create_desc({
                    "name"       : SSL_NAME_PREFIX + "shared_mem",
                    "value_type" : "float",
                    "units"      : "bytes",
                    "format"     : "%.3f",
                    "call_back"   : get_value,
                    "description": "Shared memory",
                    }))

        descriptors.append(create_desc({
                    "name"       : SSL_NAME_PREFIX + "current_sessions",
                    "value_type" : "uint",
                    "units"      : "sessions",
                    "format"     : "%u",
                    "call_back"   : get_value,
                    "description": "Current sessions",
                    }))

        descriptors.append(create_desc({
                    "name"       : SSL_NAME_PREFIX + "num_subcaches",
                    "value_type" : "uint",
                    "units"      : "subcaches",
                    "format"     : "%u",
                    "call_back"   : get_value,
                    "description": "Number of subcaches",
                    }))

        descriptors.append(create_desc({
                    "name"       : SSL_NAME_PREFIX + "indexes_per_subcache",
                    "value_type" : "float",
                    "units"      : "indexes",
                    "format"     : "%.3f",
                    "call_back"   : get_value,
                    "description": "Subcaches",
                    }))

        descriptors.append(create_desc({
                    "name"       : SSL_NAME_PREFIX + "index_usage",
                    "value_type" : "float",
                    "units"      : "pct",
                    "format"     : "%.3f",
                    "call_back"   : get_value,
                    "description": "Index usage",
                    }))

        descriptors.append(create_desc({
                    "name"       : SSL_NAME_PREFIX + "cache_usage",
                    "value_type" : "float",
                    "units"      : "pct",
                    "format"     : "%.3f",
                    "call_back"   : get_value,
                    "description": "Cache usage",
                    }))

        descriptors.append(create_desc({
                    "name"       : SSL_NAME_PREFIX + "sessions_stored",
                    "value_type" : "float",
                    "units"      : "sessions/sec",
                    "format"     : "%.3f",
                    "call_back"   : get_delta,
                    "description": "Sessions stored",
                    }))

        descriptors.append(create_desc({
                    "name"       : SSL_NAME_PREFIX + "sessions_expired",
                    "value_type" : "float",
                    "units"      : "sessions/sec",
                    "format"     : "%.3f",
                    "call_back"   : get_delta,
                    "description": "Sessions expired",
                    }))

        descriptors.append(create_desc({
                    "name"       : SSL_NAME_PREFIX + "retrieves_hit",
                    "value_type" : "float",
                    "units"      : "retrieves/sec",
                    "format"     : "%.3f",
                    "call_back"   : get_delta,
                    "description": "Retrieves Hit",
                    }))

        descriptors.append(create_desc({
                    "name"       : SSL_NAME_PREFIX + "retrieves_miss",
                    "value_type" : "float",
                    "units"      : "retrieves/sec",
                    "format"     : "%.3f",
                    "call_back"   : get_delta,
                    "description": "Retrieves Miss",
                    }))

        descriptors.append(create_desc({
                    "name"       : SSL_NAME_PREFIX + "removes_hit",
                    "value_type" : "float",
                    "units"      : "removes/sec",
                    "format"     : "%.3f",
                    "call_back"   : get_delta,
                    "description": "Removes Hit",
                    }))

        descriptors.append(create_desc({
                    "name"       : SSL_NAME_PREFIX + "removes_miss",
                    "value_type" : "float",
                    "units"      : "removes/sec",
                    "format"     : "%.3f",
                    "call_back"   : get_delta,
                    "description": "Removes Miss",
                    }))

    return descriptors


def metric_cleanup():
    '''Clean up the metric module.'''
    pass


if __name__ == '__main__':
    try:
        params = {
            'url'         : 'http://localhost:7070/server-status',
            'collect_ssl' : False
            }
        metric_init(params)
        while True:
            for d in descriptors:
                v = d['call_back'](d['name'])
                if d['name'] == NAME_PREFIX + "rps":
                    print 'value for %s is %.4f' % (d['name'], v)
                else:
                    print 'value for %s is %s' % (d['name'], v)
            time.sleep(15)
    except KeyboardInterrupt:
        os._exit(1)
