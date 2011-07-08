#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import traceback
import os
import threading
import time
import socket
import select

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
        self.refresh_rate = 15
        if "refresh_rate" in params:
            self.refresh_rate = int(params["refresh_rate"])
        self.metric       = {}
        self.last_metric       = {}
        self.timeout      = 2

        self.host         = "localhost"
        self.port         = 11211
        if "host" in params:
            self.host = params["host"]
        if "port" in params:
            self.port = int(params["port"])
        self.type    = params["type"]
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
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        msg  = ""
        self.last_metric = self.metric.copy()
        try:
            dprint("connect %s:%d", self.host, self.port)
            sock.connect((self.host, self.port))
            sock.send("stats\r\n")

            while True:
                rfd, wfd, xfd = select.select([sock], [], [], self.timeout)
                if not rfd:
                    print >>sys.stderr, "ERROR: select timeout"
                    break

                for fd in rfd:
                    if fd == sock:
                        data = fd.recv(8192)
                        msg += data

                if msg.find("END"):
                    break

            sock.close()
        except socket.error, e:
            print >>sys.stderr, "ERROR: %s" % e

        for m in msg.split("\r\n"):
            d = m.split(" ")
            if len(d) == 3 and d[0] == "STAT" and floatable(d[2]):
                self.metric[self.mp+"_"+d[1]] = float(d[2])

    def metric_of(self, name):
        val = 0
        mp = name.split("_")[0]
        if name.rsplit("_",1)[1] == "rate" and name.rsplit("_",1)[0] in self.metric:
            _Lock.acquire()
            name = name.rsplit("_",1)[0]
            if name in self.last_metric:
                num = self.metric[name]-self.last_metric[name]
                period = self.metric[mp+"_time"]-self.last_metric[mp+"_time"]
                try:
                    val = num/period
                except ZeroDivisionError:
                    val = 0
            _Lock.release()
        elif name in self.metric:
            _Lock.acquire()
            val = self.metric[name]
            _Lock.release()
        return val

def metric_init(params):
    global descriptors, Desc_Skel, _Worker_Thread, Debug

    print '[memcached] memcached protocol "stats"'
    if "type" not in params:
        params["type"] = "memcached"

    if "metrix_prefix" not in params:
        if params["type"] == "memcached":
            params["metrix_prefix"] = "mc"
        elif params["type"] == "Tokyo Tyrant":
            params["metrix_prefix"] = "tt"

    print params

    # initialize skeleton of descriptors
    Desc_Skel = {
        'name'        : 'XXX',
        'call_back'   : metric_of,
        'time_max'    : 60,
        'value_type'  : 'float',
        'format'      : '%.0f',
        'units'       : 'XXX',
        'slope'       : 'XXX', # zero|positive|negative|both
        'description' : 'XXX',
        'groups'      : params["type"],
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
                "name"       : mp+"_curr_items",
                "units"      : "items",
                "slope"      : "both",
                "description": "Current number of items stored",
                }))
    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_cmd_get",
                "units"      : "commands",
                "slope"      : "positive",
                "description": "Cumulative number of retrieval reqs",
                }))
    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_cmd_set",
                "units"      : "commands",
                "slope"      : "positive",
                "description": "Cumulative number of storage reqs",
                }))
    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_bytes_read",
                "units"      : "bytes",
                "slope"      : "positive",
                "description": "Total number of bytes read by this server from network",
                }))
    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_bytes_written",
                "units"      : "bytes",
                "slope"      : "positive",
                "description": "Total number of bytes sent by this server to network",
                }))
    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_bytes",
                "units"      : "bytes",
                "slope"      : "both",
                "description": "Current number of bytes used to store items",
                }))
    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_limit_maxbytes",
                "units"      : "bytes",
                "slope"      : "both",
                "description": "Number of bytes this server is allowed to use for storage",
                }))
    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_curr_connections",
                "units"      : "connections",
                "slope"      : "both",
                "description": "Number of open connections",
                }))
    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_evictions",
                "units"      : "items",
                "slope"      : "both",
                "description": "Number of valid items removed from cache to free memory for new items",
                }))
    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_get_hits",
                "units"      : "items",
                "slope"      : "positive",
                "description": "Number of keys that have been requested and found present ",
                }))
    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_get_misses",
                "units"      : "items",
                "slope"      : "positive",
                "description": "Number of items that have been requested and not found",
                }))
    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_get_hits_rate",
                "units"      : "items",
                "slope"      : "both",
                "description": "Hits per second",
                }))
    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_get_misses_rate",
                "units"      : "items",
                "slope"      : "both",
                "description": "Misses per second",
                }))
    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_cmd_get_rate",
                "units"      : "commands",
                "slope"      : "both",
                "description": "Gets per second",
                }))
    descriptors.append(create_desc(Desc_Skel, {
                "name"       : mp+"_cmd_set_rate",
                "units"      : "commands",
                "slope"      : "both",
                "description": "Sets per second",
                }))

    # Tokyo Tyrant
    if "type" in params and params["type"].lower().find("tokyo") == 0:
        dtmp = descriptors[:]
        for d in dtmp:
            if d["name"] in [
                mp+"_bytes_read",
                mp+"_bytes_written",
                mp+"_limit_maxbytes",
                mp+"_curr_connections",
                mp+"_evictions",
                ]:
                descriptors.remove(d)
        for d in descriptors:
            if d["name"] == mp+"_get_hits":
                d["name"] = mp+"_cmd_get_hits"
            if d["name"] == mp+"_get_misses":
                d["name"] = mp+"_cmd_get_misses"

        descriptors.append(create_desc(Desc_Skel, {
                    "name"       : mp+"_cmd_set_hits",
                    "units"      : "items",
                    "slope"      : "positive",
                    "description": "Number of keys that have been stored and found present ",
                    }))
        descriptors.append(create_desc(Desc_Skel, {
                    "name"       : mp+"_cmd_set_misses",
                    "units"      : "items",
                    "slope"      : "positive",
                    "description": "Number of items that have been stored and not found",
                    }))

        descriptors.append(create_desc(Desc_Skel, {
                    "name"       : mp+"_cmd_delete",
                    "units"      : "commands",
                    "slope"      : "positive",
                    "description": "Cumulative number of delete reqs",
                    }))
        descriptors.append(create_desc(Desc_Skel, {
                    "name"       : mp+"_cmd_delete_hits",
                    "units"      : "items",
                    "slope"      : "positive",
                    "description": "Number of keys that have been deleted and found present ",
                    }))
        descriptors.append(create_desc(Desc_Skel, {
                    "name"       : mp+"_cmd_delete_misses",
                    "units"      : "items",
                    "slope"      : "positive",
                    "description": "Number of items that have been deleted and not found",
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
            "host"  : "localhost",
            "port"  : 11211,
            # "host"  : "tt101",
            # "port"  : 1978,
            # "type"  : "Tokyo Tyrant",
            # "metrix_prefix" : "tt101",
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
