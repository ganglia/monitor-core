#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import os
import threading
import time

descriptors = list()
Desc_Skel   = {}
_Worker_Thread = None
_Lock = threading.Lock() # synchronization lock

Debug = False

def dprint(f, *v):
    if Debug:
        print >> sys.stderr, "DEBUG: "+f % v

class UpdateTrafficThread(threading.Thread):

    __slots__ = ( 'proc_file' )

    def __init__(self, params):
        threading.Thread.__init__(self)
        self.running       = False
        self.shuttingdown  = False
        self.refresh_rate = 10
        if "refresh_rate" in params:
            self.refresh_rate = int(params["refresh_rate"])

        self.target_device = params["target_device"]
        self.metric       = {}

        self.proc_file = "/proc/net/dev"
        self.stats_tab = {
            "recv_bytes"  : 0,
            "recv_pkts"   : 1,
            "recv_errs"   : 2,
            "recv_drops"  : 3,
            "trans_bytes" : 8,
            "trans_pkts"  : 9,
            "trans_errs"  : 10,
            "trans_drops" : 11,
            }
        self.stats      = {}
        self.stats_prev = {}

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
        f = open(self.proc_file, "r")
        for l in f:
            a = l.split(":")
            dev = a[0].lstrip()
            if dev != self.target_device: continue

            dprint("%s", ">>update_metric")
            self.stats = {}
            _stats = a[1].split()
            for name, index in self.stats_tab.iteritems():
                self.stats[name+'_'+self.target_device] = int(_stats[index])
            self.stats["time"] = time.time()
            dprint("%s", self.stats)

            if "time" in self.stats_prev:
                dprint("%s: %d = %d - %d", "DO DIFF", self.stats["time"]-self.stats_prev["time"], self.stats["time"], self.stats_prev["time"])
                d = self.stats["time"] - self.stats_prev["time"]
                for name, cur in self.stats.iteritems():
                    self.metric[name] = float(cur - self.stats_prev[name])/d

            self.stats_prev = self.stats.copy()
            break

        return

    def metric_of(self, name):
        val = 0
        if name in self.metric:
            _Lock.acquire()
            val = self.metric[name]
            _Lock.release()
        return val

def metric_init(params):
    global Desc_Skel, _Worker_Thread, Debug

    print '[traffic1] Received the following parameters'
    print params

    Desc_Skel = {
        'name'        : 'XXX',
        'call_back'   : metric_of,
        'time_max'    : 60,
        'value_type'  : 'float',
        'format'      : '%.3f',
        'units'       : 'XXX',
        'slope'       : 'both',
        'description' : 'XXX',
        'groups'      : 'network',
        }

    if "refresh_rate" not in params:
        params["refresh_rate"] = 10
    if "debug" in params:
        Debug = params["debug"]
    dprint("%s", "Debug mode on")
    if "target_device" not in params:
        params["target_device"] = "lo"
    target_device = params["target_device"]

    _Worker_Thread = UpdateTrafficThread(params)
    _Worker_Thread.start()

    # IP:HOSTNAME
    if "spoof_host" in params:
        Desc_Skel["spoof_host"] = params["spoof_host"]

    descriptors.append(create_desc(Desc_Skel, {
                "name"        : "recv_bytes_" + target_device,
                "units"       : "bytes/sec",
                "description" : "received bytes per sec",
                }))
    descriptors.append(create_desc(Desc_Skel, {
                "name"        : "recv_pkts_" + target_device,
                "units"       : "pkts/sec",
                "description" : "received packets per sec",
                }))
    descriptors.append(create_desc(Desc_Skel, {
                "name"        : "recv_errs_" + target_device,
                "units"       : "pkts/sec",
                "description" : "received error packets per sec",
                }))

    descriptors.append(create_desc(Desc_Skel, {
                "name"        : "trans_bytes_" + target_device,
                "units"       : "bytes/sec",
                "description" : "transmitted bytes per sec",
                }))
    descriptors.append(create_desc(Desc_Skel, {
                "name"        : "trans_pkts_" + target_device,
                "units"       : "pkts/sec",
                "description" : "transmitted packets per sec",
                }))
    descriptors.append(create_desc(Desc_Skel, {
                "name"        : "trans_errs_" + target_device,
                "units"       : "pkts/sec",
                "description" : "transmitted error packets per sec",
                }))

    return descriptors

def create_desc(skel, prop):
    d = skel.copy()
    for k, v in prop.iteritems():
        d[k] = v
    return d

def metric_of(name):
    return _Worker_Thread.metric_of(name)

def metric_cleanup():
    _Worker_Thread.shutdown()

if __name__ == '__main__':
    try:
        params = {
            "target_device": "eth0",
            "debug"        : True,
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
    except StandardError:
        print sys.exc_info()[0]
        os._exit(1)
