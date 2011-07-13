#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import threading
import time
import urllib2
import traceback
import json

descriptors = list()
Desc_Skel   = {}
_Worker_Thread = None
_Lock = threading.Lock() # synchronization lock
Debug = False

def dprint(f, *v):
    if Debug:
        print >>sys.stderr, "DEBUG: "+f % v

def floatable(str):
    try:
        float(str)
        return True
    except:
        return False

class UpdateMetricThread(threading.Thread):

    def __init__(self, params):
        threading.Thread.__init__(self)
        self.running      = False
        self.shuttingdown = False
        self.refresh_rate = 30
        if "refresh_rate" in params:
            self.refresh_rate = int(params["refresh_rate"])
        self.metric       = {}
        self.timeout      = 10

        self.url          = "http://localhost:8098/stats"
        if "url" in params:
            self.url = params["url"]
        self.mp      = params["metrix_prefix"]

    def shutdown(self):
        self.shuttingdown = True
        if not self.running:
            return
        self.join()

    def run(self):
        self.running = True

        while not self.shuttingdown:
            _Lock.acquire()
            self.update_metric()
            _Lock.release()
            time.sleep(self.refresh_rate)

        self.running = False

    def update_metric(self):
        try:
            req = urllib2.Request(url = self.url)
            res = urllib2.urlopen(req)
            stats = res.read()
            dprint("%s", stats)
            json_stats = json.loads(stats)
            for (key,value) in json_stats.iteritems():
              dprint("%s = %s", key, value)
              if value == 'undefined':
                self.metric[self.mp+'_'+key] = 0
              else:
                self.metric[self.mp+'_'+key] = value
        except urllib2.URLError:
            traceback.print_exc()
        else:
            res.close()

    def metric_of(self, name):
        val = 0
        mp = name.split("_")[0]
        if name in self.metric:
            _Lock.acquire()
            val = self.metric[name]
            _Lock.release()
        return val

def metric_init(params):
    global descriptors, Desc_Skel, _Worker_Thread, Debug

    if "metrix_prefix" not in params:
      params["metrix_prefix"] = "riak"

    print params

    # initialize skeleton of descriptors
    Desc_Skel = {
        'name'        : 'XXX',
        'call_back'   : metric_of,
        'time_max'    : 60,
        'value_type'  : 'uint',
        'format'      : '%u',
        'units'       : 'XXX',
        'slope'       : 'XXX', # zero|positive|negative|both
        'description' : 'XXX',
        'groups'      : 'riak',
        }

    if "refresh_rate" not in params:
        params["refresh_rate"] = 15
    if "debug" in params:
        Debug = params["debug"]
    dprint("%s", "Debug mode on")

    _Worker_Thread = UpdateMetricThread(params)
    _Worker_Thread.start()

    # IP:HOSTNAME
    if "spoof_host" in params:
        Desc_Skel["spoof_host"] = params["spoof_host"]

    mp = params["metrix_prefix"]

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_ring_creation_size",
                "units"      : "vnodes",
                "slope"      : "both",
                "description": mp+"_ring_creation_size",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_node_get_fsm_time_mean",
                "units"      : "microseconds",
                "slope"      : "both",
                "description": "Mean for riak_kv_get_fsm calls",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_pbc_active",
                "units"      : "connections",
                "slope"      : "both",
                "description": "Active pb socket connections",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_node_put_fsm_time_100",
                "units"      : "microseconds",
                "slope"      : "both",
                "description": "100th percentile for riak_kv_put_fsm calls",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_node_put_fsm_time_mean",
                "units"      : "microseconds",
                "slope"      : "both",
                "description": "Mean for riak_kv_put_fsm calls",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_node_get_fsm_time_95",
                "units"      : "microseconds",
                "slope"      : "both",
                "description": "95th percentile for riak_kv_get_fsm calls",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_vnode_puts",
                "units"      : "puts",
                "slope"      : "both",
                "description": "Puts handled by local vnodes in the last minute",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_node_puts",
                "units"      : "puts",
                "slope"      : "both",
                "description": "Puts coordinated by this node in the last minute",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_node_get_fsm_time_median",
                "units"      : "microseconds",
                "slope"      : "both",
                "description": "Median for riak_kv_get_fsm calls",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_sys_process_count",
                "units"      : "processes",
                "slope"      : "both",
                "description": "Erlang processes",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_node_put_fsm_time_median",
                "units"      : "microseconds",
                "slope"      : "both",
                "description": "Median for riak_kv_put_fsm calls",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_node_put_fsm_time_95",
                "units"      : "microseconds",
                "slope"      : "both",
                "description": "95th percentile for riak_kv_put_fsm calls",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_node_get_fsm_time_100",
                "units"      : "microseconds",
                "slope"      : "both",
                "description": "100th percentile for riak_kv_get_fsm calls",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_node_get_fsm_time_99",
                "units"      : "microseconds",
                "slope"      : "both",
                "description": "99th percentile for riak_kv_get_fsm calls",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_vnode_gets",
                "units"      : "gets",
                "slope"      : "both",
                "description": "Gets handled by local vnodes in the last minute",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_read_repairs",
                "units"      : "repairs",
                "slope"      : "both",
                "description": mp+"_read_repairs",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_ring_num_partitions",
                "units"      : "vnodes",
                "slope"      : "both",
                "description": mp+"_ring_num_partitions",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_mem_total",
                "units"      : "bytes",
                "format"     : "%.1f",
                'value_type'  : 'float',
                "slope"      : "both",
                "description": mp+"_mem_total",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_node_gets",
                "units"      : "gets",
                "slope"      : "both",
                "description": "Gets coordinated by this node in the last minute",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_mem_allocated",
                "units"      : "bytes",
                "format"     : "%.1f",
                'value_type'  : 'float',
                "slope"      : "both",
                "description": mp+"_mem_allocated",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_node_put_fsm_time_99",
                "units"      : "microseconds",
                "slope"      : "both",
                "description": "99th percentile for riak_kv_put_fsm calls",
                }))

    return descriptors

def create_desc(skel, prop):
    d = skel.copy()
    for k,v in prop.iteritems():
        d[k] = v
    return d

def metric_of(name):
    return _Worker_Thread.metric_of(name)

def metric_cleanup():
    _Worker_Thread.shutdown()

if __name__ == '__main__':
    try:
        params = {
            "debug" : True,
            }
        metric_init(params)

        while True:
            for d in descriptors:
                v = d['call_back'](d['name'])
                print ('value for %s is '+d['format']) % (d['name'],  v)
            time.sleep(5)
    except KeyboardInterrupt:
        time.sleep(0.2)
        os._exit(1)
    except:
        traceback.print_exc()
        os._exit(1)