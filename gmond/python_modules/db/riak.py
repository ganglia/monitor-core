#!/usr/bin/env python
# -*- coding: utf-8 -*-

# [Riak](https://wiki.basho.com/display/RIAK/Riak) is a Dynamo-inspired
# key/value store.
#
# This module collects metrics from the JSON stats interface of riak, available
# at http://localhost:8098/stats. The statistics-aggregator must be enabled in
# your riak configuration for this to work:
#     {riak_kv, [
#       %% ...
#       {riak_kv_stat, true},
#       %% ...
#     ]},
#
# You'll want to edit the url key in riak.conf to point at the interface your
# riak install is listening on:
#
#     param url {
#         value = "http://10.0.1.123:8098/stats"
#     }

import os
import sys
import threading
import time
import urllib2
import traceback
import json

descriptors = list()
Desc_Skel = {}
_Worker_Thread = None
_Lock = threading.Lock()  # synchronization lock
Debug = False


def dprint(f, *v):
    if Debug:
        print >>sys.stderr, "DEBUG: " + f % v


def floatable(str):
    try:
        float(str)
        return True
    except:
        return False


class UpdateMetricThread(threading.Thread):

    def __init__(self, params):
        threading.Thread.__init__(self)
        self.running = False
        self.shuttingdown = False
        self.refresh_rate = 30
        if "refresh_rate" in params:
            self.refresh_rate = int(params["refresh_rate"])
        self.metric = {}
        self.timeout = 10

        self.url = "http://localhost:8098/stats"
        if "url" in params:
            self.url = params["url"]
        self.mp = params["metrix_prefix"]

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
            req = urllib2.Request(url=self.url)
            res = urllib2.urlopen(req)
            stats = res.read()
            dprint("%s", stats)
            json_stats = json.loads(stats)
            for (key, value) in json_stats.iteritems():
                dprint("%s = %s", key, value)
                if value == 'undefined':
                    self.metric[self.mp + '_' + key] = 0
                else:
                    self.metric[self.mp + '_' + key] = value
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
        'slope'       : 'XXX',  # zero|positive|negative|both
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
        "slope": "both",
        "name": mp + "_riak_node_get_fsm_siblings_100",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_riak_node_get_fsm_siblings_95",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_riak_node_get_fsm_siblings_99",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_riak_node_get_fsm_siblings_mean",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_riak_node_get_fsm_siblings_median",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_executing_mappers",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_node_get_fsm_active",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_node_get_fsm_active_60s",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_node_get_fsm_in_rate",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_node_put_fsm_active_60s",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_node_put_fsm_in_rate",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_node_put_fsm_out_rate",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_pipeline_active",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_pipeline_create_count",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_pipeline_create_error_count",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_pipeline_create_error_one",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_pipeline_create_one",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_index_fsm_active",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_index_fsm_create",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_index_fsm_create_error",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_list_fsm_active",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_list_fsm_create",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_list_fsm_create_error",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_node_get_fsm_out_rate",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_node_get_fsm_rejected",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_node_get_fsm_rejected_60s",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_node_get_fsm_siblings_100",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_node_put_fsm_active",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_node_put_fsm_rejected",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_node_put_fsm_rejected_60s",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_read_repairs_primary_notfound_count",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_read_repairs_primary_notfound_one",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_read_repairs_primary_outofdate_count",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_read_repairs_primary_outofdate_one",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_rebalance_delay_mean",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_rejected_handoffs",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_riak_core_stat_ts",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_riak_kv_stat_ts",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_riak_pipe_stat_ts",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_riak_pipe_vnodeq_mean",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_rings_reconciled",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_vnode_index_refreshes",
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "slope": "both",
        "name": mp + "_node_puts",
        "units": "puts",
        "description": mp + "_node_puts"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_sys_logical_processors",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_sys_logical_processors"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "positive",
      "name": mp + "_ignored_gossip_total",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_ignored_gossip_total"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_vnode_gets",
      "units": "gets",
      "description": mp + "_vnode_gets"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_node_get_fsm_objsize_100",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_node_get_fsm_objsize_100"
    }))
    descriptors.append(create_desc(Desc_Skel, {
        "name": mp + "_node_put_fsm_time_mean",
        "units": "microseconds",
        "slope": "both",
        "description": mp + "_node_put_fsm_time_mean",
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "positive",
      "name": mp + "_node_puts_total",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_node_puts_total"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_riak_kv_vnodeq_mean",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_riak_kv_vnodeq_mean"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_riak_pipe_vnodes_running",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_riak_pipe_vnodes_running"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_vnode_index_writes_postings",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_vnode_index_writes_postings"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "positive",
      "name": mp + "_vnode_index_deletes_postings_total",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_vnode_index_deletes_postings_total"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "positive",
      "name": mp + "_read_repairs_total",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_read_repairs_total"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_precommit_fail",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_precommit_fail"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_rebalance_delay_max",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_rebalance_delay_max"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_converge_delay_mean",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_converge_delay_mean"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_node_get_fsm_time_95",
      "units": "microseconds",
      "description": mp + "_node_get_fsm_time_95"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_riak_pipe_vnodeq_median",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_riak_pipe_vnodeq_median"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_node_get_fsm_time_99",
      "value_type": "uint",
      "units": "microseconds",
      "description": mp + "_node_get_fsm_time_99"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_memory_code",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_memory_code"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_sys_smp_support",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_sys_smp_support"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_memory_processes_used",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_memory_processes_used"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_pbc_active",
      "units": "connections",
      "description": mp + "_pbc_active"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "positive",
      "name": mp + "_vnode_index_writes_postings_total",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_vnode_index_writes_postings_total"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "positive",
      "name": mp + "_vnode_gets_total",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_vnode_gets_total"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_sys_process_count",
      "units": "processes",
      "description": "Erlang processes"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_read_repairs",
      "units": "repairs",
      "description": mp + "_read_repairs"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_vnode_puts",
      "units": "puts",
      "description": mp + "_vnode_puts"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_riak_search_vnodes_running",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_riak_search_vnodes_running"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_riak_pipe_vnodeq_min",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_riak_pipe_vnodeq_min"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_node_get_fsm_objsize_mean",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_node_get_fsm_objsize_mean"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_cpu_nprocs",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_cpu_nprocs"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_node_put_fsm_time_100",
      "units": "microseconds",
      "description": mp + "_node_put_fsm_time_100"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_mem_total",
      "format": "%.1f",
      "value_type": "float",
      "units": "bytes",
      "description": mp + "_mem_total"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_riak_search_vnodeq_mean",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_riak_search_vnodeq_mean"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_node_get_fsm_siblings_median",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_node_get_fsm_siblings_median"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_riak_search_vnodeq_max",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_riak_search_vnodeq_max"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_sys_threads_enabled",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_sys_threads_enabled"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_vnode_index_writes",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_vnode_index_writes"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "positive",
      "name": mp + "_memory_total",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_memory_total"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_riak_pipe_vnodeq_max",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_riak_pipe_vnodeq_max"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_vnode_index_deletes_postings",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_vnode_index_deletes_postings"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_node_get_fsm_time_mean",
      "units": "microseconds",
      "description": "Mean for riak_kv_get_fsm calls"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_ring_num_partitions",
      "units": "vnodes",
      "description": mp + "_ring_num_partitions"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_riak_search_vnodeq_min",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_riak_search_vnodeq_min"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "positive",
      "name": mp + "_vnode_index_reads_total",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_vnode_index_reads_total"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_node_get_fsm_siblings_99",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_node_get_fsm_siblings_99"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_mem_allocated",
      "format": "%.1f",
      "value_type": "float",
      "units": "bytes",
      "description": mp + "_mem_allocated"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_node_get_fsm_siblings_95",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_node_get_fsm_siblings_95"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_node_get_fsm_objsize_95",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_node_get_fsm_objsize_95"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_node_put_fsm_time_median",
      "units": "microseconds",
      "description": mp + "_node_put_fsm_time_median"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_memory_processes",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_memory_processes"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "positive",
      "name": mp + "_riak_search_vnodeq_total",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_riak_search_vnodeq_total"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_node_get_fsm_objsize_99",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_node_get_fsm_objsize_99"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_memory_system",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_memory_system"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_sys_global_heaps_size",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_sys_global_heaps_size"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_gossip_received",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_gossip_received"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_node_get_fsm_time_median",
      "units": "microseconds",
      "description": mp + "_node_get_fsm_time_median"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "positive",
      "name": mp + "_vnode_index_deletes_total",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_vnode_index_deletes_total"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_riak_kv_vnodeq_max",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_riak_kv_vnodeq_max"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_riak_kv_vnodeq_min",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_riak_kv_vnodeq_min"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_node_gets",
      "units": "gets",
      "description": mp + "_node_gets"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_riak_kv_vnodeq_median",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_riak_kv_vnodeq_median"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_postcommit_fail",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_postcommit_fail"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_riak_search_vnodeq_median",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_riak_search_vnodeq_median"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_sys_wordsize",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_sys_wordsize"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_riak_kv_vnodes_running",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_riak_kv_vnodes_running"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_node_put_fsm_time_95",
      "units": "microseconds",
      "description": mp + "_node_put_fsm_time_95",
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "positive",
      "name": mp + "_rings_reconciled_total",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_rings_reconciled_total"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_rebalance_delay_min",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_rebalance_delay_min"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_node_put_fsm_time_99",
      "units": "microseconds",
      "description": mp + "_node_put_fsm_time_99",
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_ring_creation_size",
      "units": "vnodes",
      "description": mp + "_ring_creation_size"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_cpu_avg1",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_cpu_avg1"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_cpu_avg5",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_cpu_avg5"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "positive",
      "name": mp + "_pbc_connects_total",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_pbc_connects_total"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_memory_binary",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_memory_binary"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "positive",
      "name": mp + "_coord_redirs_total",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_coord_redirs_total"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "positive",
      "name": mp + "_riak_pipe_vnodeq_total",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_riak_pipe_vnodeq_total"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "positive",
      "name": mp + "_node_gets_total",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_node_gets_total"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "positive",
      "name": mp + "_vnode_puts_total",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_vnode_puts_total"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_memory_atom",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_memory_atom"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_cpu_avg15",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_cpu_avg15"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_node_get_fsm_objsize_median",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_node_get_fsm_objsize_median"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_node_get_fsm_siblings_mean",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_node_get_fsm_siblings_mean"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_converge_delay_min",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_converge_delay_min"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_node_get_fsm_time_100",
      "units": "microseconds",
      "description": mp + "_node_get_fsm_time_100"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_converge_delay_max",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_converge_delay_max"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_converge_delay_last",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_converge_delay_last"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_rebalance_delay_last",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_rebalance_delay_last"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_vnode_index_deletes",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_vnode_index_deletes"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "positive",
      "name": mp + "_riak_kv_vnodeq_total",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_riak_kv_vnodeq_total"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_handoff_timeouts",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_handoff_timeouts"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_vnode_index_reads",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_vnode_index_reads"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_sys_thread_pool_size",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_sys_thread_pool_size"
    }))
    descriptors.append(create_desc(Desc_Skel, {
      "slope": "both",
      "name": mp + "_pbc_connects",
      "format": "%u",
      "value_type": "uint",
      "units": "N",
      "description": mp + "_pbc_connects"
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
            "debug": True,
            }
        metric_init(params)
        while True:
            for d in descriptors:
                v = d['call_back'](d['name'])
                print ('value for %s is ' + d['format']) % (d['name'],  v)
            time.sleep(5)
    except KeyboardInterrupt:
        time.sleep(0.2)
        os._exit(1)
    except:
        traceback.print_exc()
        os._exit(1)
