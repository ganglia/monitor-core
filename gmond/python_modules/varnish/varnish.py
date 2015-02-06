#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Varnish gmond module for Ganglia
#
# Copyright (C) 2011 by Michael T. Conigliaro <mike [at] conigliaro [dot] org>.
# All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

import os
import time
import copy

NAME_PREFIX = 'varnish_'
PARAMS = {
    'stats_command' : 'varnishstat -1'
}
METRICS = {
    'time' : 0,
    'data' : {}
}
LAST_METRICS = copy.deepcopy(METRICS)
METRICS_CACHE_MAX = 5

def create_desc(skel, prop):
    d = skel.copy()
    for k,v in prop.iteritems():
        d[k] = v
    return d


def get_metrics():
    """Return all metrics"""

    global METRICS, LAST_METRICS

    if (time.time() - METRICS['time']) > METRICS_CACHE_MAX:

        # get raw metric data
        io = os.popen(PARAMS['stats_command'])

        # convert to dict
        metrics = {}
        for line in io.readlines():
            values = line.split()[:2]
            try:
                metrics[values[0]] = int(values[1])
            except ValueError:
                metrics[values[0]] = 0

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

    # get delta
    name = name[len(NAME_PREFIX):] # remove prefix from name
    try:
        delta = float(curr_metrics['data'][name] - last_metrics['data'][name])/(curr_metrics['time'] - last_metrics['time'])
        if delta < 0:
            print "Less than 0"
            delta = 0
    except StandardError:
        delta = 0

    return delta


def get_cache_hit_ratio(name):
    """Return cache hit ratio"""

    try:
        result = get_delta(NAME_PREFIX + 'cache_hit') / get_delta(NAME_PREFIX + 'client_req') * 100
    except ZeroDivisionError:
        result = 0

    return result


def metric_init(lparams):
    """Initialize metric descriptors"""

    global PARAMS, Desc_Skel

    # set parameters
    for key in lparams:
        PARAMS[key] = lparams[key]

    # define descriptors
    time_max = 60

    Desc_Skel = {
        'name'        : 'XXX',
        'call_back'   : 'XXX',
        'time_max'    : 60,
        'value_type'  : 'float',
        'format'      : '%f',
        'units'       : 'XXX',
        'slope'       : 'both', # zero|positive|negative|both
        'description' : 'XXX',
        'groups'      : 'varnish',
        }

    descriptors = []

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'cache_hit_ratio',
                "call_back"  : get_cache_hit_ratio,
                "units"      : "pct",
                "description": "Cache Hit ratio",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'client_conn',
                "call_back"  : get_delta,
                "units"      : "conn/s",
                "description": "Client connections accepted",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'client_drop',
                "call_back"  : get_delta,
                "units"      : "conn/s",
                "description": "Connection dropped, no sess/wrk",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'client_req',
                "call_back"  : get_delta,
                "units"      : "req/s",
                "description": "Client requests received",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'cache_hit',
                "call_back"  : get_delta,
                "units"      : "hit/s",
                "description": "Cache hits",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'cache_hitpass',
                "call_back"  : get_delta,
                "units"      : "hit/s",
                "description": "Cache hits for pass",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'cache_miss',
                "units"      : "miss/s",
                "call_back"  : get_delta,
                "description": "Cache misses",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'backend_conn',
                "call_back"  : get_delta,
                "units"      : "conn/s",
                "description": "Backend conn. success",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'backend_unhealthy',
                "call_back"  : get_delta,
                "units"      : "conn/s",
                "description": "Backend conn. not attempted",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'backend_busy',
                "call_back"  : get_delta,
                "units"      : "busy/s",
                "description": "Backend conn. too many",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'backend_fail',
                "call_back"  : get_delta,
                "units"      : "fail/s",
                "description": "Backend conn. failures",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'backend_reuse',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "Backend conn. reuses",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'backend_toolate',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "Backend conn. was closed",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'backend_recycle',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "Backend conn. recycles",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'backend_unused',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "Backend conn. unused",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'fetch_head',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "Fetch head",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'fetch_length',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "Fetch with Length",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'fetch_chunked',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "Fetch chunked",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'fetch_eof',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "Fetch EOF",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'fetch_bad',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "Fetch had bad headers",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'fetch_close',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "Fetch wanted close",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'fetch_oldhttp',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "Fetch pre HTTP/1.1 closed",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'fetch_zero',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "Fetch zero len",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'fetch_failed',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "Fetch failed",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_sess_mem',
                "call_back"  : get_value,
                "units"      : "Bytes",
                "description": "N struct sess_mem",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_sess',
                "call_back"  : get_value,
                "units"      : "sessions",
                "description": "N struct sess",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_object',
                "call_back"  : get_value,
                "units"      : "objects",
                "description": "N struct object",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_vampireobject',
                "call_back"  : get_value,
                "units"      : "objects",
                "description": "N unresurrected objects",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_objectcore',
                "call_back"  : get_value,
                "units"      : "objects",
                "description": "N struct objectcore",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_objecthead',
                "call_back"  : get_value,
                "units"      : "objects",
                "description": "N struct objecthead",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_smf',
                "call_back"  : get_value,
                "units"      : "",
                "description": "N struct smf",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_smf_frag',
                "call_back"  : get_value,
                "units"      : "frags",
                "description": "N small free smf",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_smf_large',
                "call_back"  : get_value,
                "units"      : "frags",
                "description": "N large free smf",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_vbe_conn',
                "call_back"  : get_value,
                "units"      : "conn",
                "description": "N struct vbe_conn",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_wrk',
                "call_back"  : get_value,
                "units"      : "threads",
                "description": "N worker threads",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_wrk_create',
                "call_back"  : get_delta,
                "units"      : "threads/s",
                "description": "N worker threads created",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_wrk_failed',
                "call_back"  : get_delta,
                "units"      : "wrk/s",
                "description": "N worker threads not created",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_wrk_max',
                "call_back"  : get_delta,
                "units"      : "threads/s",
                "description": "N worker threads limited",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_wrk_queue',
                "call_back"  : get_value,
                "units"      : "req",
                "description": "N queued work requests",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_wrk_overflow',
                "call_back"  : get_delta,
                "units"      : "req/s",
                "description": "N overflowed work requests",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_wrk_drop',
                "call_back"  : get_delta,
                "units"      : "req/s",
                "description": "N dropped work requests",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_backend',
                "call_back"  : get_value,
                "units"      : "backends",
                "description": "N backends",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_expired',
                "call_back"  : get_delta,
                "units"      : "obj/s",
                "description": "N expired objects",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_lru_nuked',
                "call_back"  : get_delta,
                "units"      : "obj/s",
                "description": "N LRU nuked objects",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_lru_saved',
                "call_back"  : get_delta,
                "units"      : "obj/s",
                "description": "N LRU saved objects",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_lru_moved',
                "call_back"  : get_delta,
                "units"      : "obj/s",
                "description": "N LRU moved objects",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_deathrow',
                "call_back"  : get_delta,
                "units"      : "obj/s",
                "description": "N objects on deathrow",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'losthdr',
                "call_back"  : get_delta,
                "units"      : "hdrs/s",
                "description": "HTTP header overflows",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_objsendfile',
                "call_back"  : get_delta,
                "units"      : "obj/s",
                "description": "Objects sent with sendfile",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_objwrite',
                "call_back"  : get_delta,
                "units"      : "obj/s",
                "description": "Objects sent with write",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_objoverflow',
                "call_back"  : get_delta,
                "description": "Objects overflowing workspace",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 's_sess',
                "call_back"  : get_delta,
                "description": "Total Sessions",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 's_req',
                "call_back"  : get_delta,
                "description": "Total Requests",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 's_pipe',
                "call_back"  : get_delta,
                "description": "Total pipe",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 's_pass',
                "call_back"  : get_delta,
                "description": "Total pass",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 's_fetch',
                "call_back"  : get_delta,
                "description": "Total fetch",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 's_hdrbytes',
                "call_back"  : get_delta,
                "description": "Total header bytes",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 's_bodybytes',
                "call_back"  : get_delta,
                "units"      : "bytes/s",
                "description": "Total body bytes",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'sess_closed',
                "call_back"  : get_delta,
                "units"      : "sessions/s",
                "description": "Session Closed",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'sess_pipeline',
                "call_back"  : get_delta,
                "description": "Session Pipeline",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'sess_readahead',
                "call_back"  : get_delta,
                "description": "Session Read Ahead",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'sess_linger',
                "call_back"  : get_delta,
                "description": "Session Linger",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'sess_herd',
                "call_back"  : get_delta,
                "description": "Session herd",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'shm_records',
                "call_back"  : get_delta,
                "description": "SHM records",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'shm_writes',
                "call_back"  : get_delta,
                "description": "SHM writes",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'shm_flushes',
                "call_back"  : get_delta,
                "description": "SHM flushes due to overflow",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'shm_cont',
                "call_back"  : get_delta,
                "description": "SHM MTX contention",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'shm_cycles',
                "call_back"  : get_delta,
                "description": "SHM cycles through buffer",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'sm_nreq',
                "call_back"  : get_delta,
                "description": "allocator requests",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'sm_nobj',
                "call_back"  : get_delta,
                "description": "outstanding allocations",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'sm_balloc',
                "call_back"  : get_value,
                "description": "bytes allocated",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'sm_bfree',
                "call_back"  : get_delta,
                "description": "bytes free",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'sma_nreq',
                "call_back"  : get_delta,
                "description": "SMA allocator requests",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'sma_nobj',
                "call_back"  : get_value,
                "units"      : "obj",
                "description": "SMA outstanding allocations",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'sma_nbytes',
                "call_back"  : get_value,
                "units"      : "Bytes",
                "description": "SMA outstanding bytes",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'sma_balloc',
                "call_back"  : get_delta,
                "units"      : "bytes/s",
                "description": "SMA bytes allocated",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'sma_bfree',
                "call_back"  : get_delta,
                "units"      : "bytes/s",
                "description": "SMA bytes free",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'sms_nreq',
                "call_back"  : get_delta,
                "units"      : "req/s",
                "description": "SMS allocator requests",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'sms_nobj',
                "call_back"  : get_value,
                "units"      : "obj",
                "description": "SMS outstanding allocations",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'sms_nbytes',
                "call_back"  : get_value,
                "units"      : "Bytes",
                "description": "SMS outstanding bytes",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'sms_balloc',
                "call_back"  : get_delta,
                "units"      : "bytes/s",
                "description": "SMS bytes allocated",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'sms_bfree',
                "call_back"  : get_delta,
                "units"      : "Bytes/s",
                "description": "SMS bytes freed",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'backend_req',
                "call_back"  : get_delta,
                "units"      : "req/s",
                "description": "Backend requests made",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_vcl',
                "call_back"  : get_value,
                "units"      : "vcl",
                "description": "N vcl total",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_vcl_avail',
                "call_back"  : get_value,
                "units"      : "vcl",
                "description": "N vcl available",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_vcl_discard',
                "call_back"  : get_value,
                "units"      : "vcl",
                "description": "N vcl discarded",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_purge',
                "call_back"  : get_value,
                "units"      : "purges",
                "description": "N total active purges",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_purge_add',
                "call_back"  : get_delta,
                "units"      : "purges/sec",
                "description": "N new purges added",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_purge_retire',
                "call_back"  : get_delta,
                "units"      : "purges/s",
                "description": "N old purges deleted",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_purge_obj_test',
                "call_back"  : get_delta,
                "units"      : "purges/s",
                "description": "N objects tested",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_purge_re_test',
                "call_back"  : get_delta,
                "description": "N regexps tested against",
                "units"      : "purges/s",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_purge_dups',
                "call_back"  : get_delta,
                "units"      : "purges/s",
                "description": "N duplicate purges removed",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'hcb_nolock',
                "call_back"  : get_delta,
                "units"      : "locks/s",
                "description": "HCB Lookups without lock",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'hcb_lock',
                "call_back"  : get_delta,
                "units"      : "locks/s",
                "description": "HCB Lookups with lock",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'hcb_insert',
                "call_back"  : get_delta,
                "units"      : "inserts/s",
                "description": "HCB Inserts",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'esi_parse',
                "call_back"  : get_delta,
                "units"      : "obj/s",
                "description": "Objects ESI parsed (unlock)",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'esi_errors',
                "call_back"  : get_delta,
                "units"      : "err/s",
                "description": "ESI parse errors (unlock)",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'accept_fail',
                "call_back"  : get_delta,
                "units"      : "accepts/s",
                "description": "Accept failures",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'client_drop_late',
                "call_back"  : get_delta,
                "units"      : "conn/s",
                "description": "Connection dropped late",
                }))

    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'uptime',
                "call_back"  : get_value,
                "units"      : "seconds",
                "description": "Client uptime",
                }))

    ##############################################################################
    
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'backend_retry',
                "call_back"  : get_delta,
                "units"      : "retries/s",
                "description": "Backend conn. retry",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'dir_dns_cache_full',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "DNS director full dnscache",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'dir_dns_failed',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "DNS director failed lookups",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'dir_dns_hit',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "DNS director cached lookups hit",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'dir_dns_lookups',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "DNS director lookups",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'esi_warnings',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "ESI parse warnings (unlock)",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'fetch_1xx',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "Fetch no body (1xx)",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'fetch_204',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "Fetch no body (204)",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'fetch_304',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "Fetch no body (304)",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_ban',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "N total active bans",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_ban_add',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "N new bans added",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_ban_retire',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "N old bans deleted",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_ban_obj_test',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "N objects tested",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_ban_re_test',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "N regexps tested against",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_ban_dups',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "N duplicate bans removed",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_ban_add',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "N new bans added",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_ban_dups',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "N duplicate bans removed",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_ban_obj_test',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "N objects tested",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_ban_re_test',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "N regexps tested against",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_ban_retire',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "N old bans deleted",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_gunzip',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "Gunzip operations",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_gzip',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "Gzip operations",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_vbc',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "N struct vbc",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_waitinglist',
                "call_back"  : get_delta,
                "units"      : "/s",
                "description": "N struct waitinglist",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_wrk_lqueue',
                "call_back"  : get_value,
                "units"      : "",
                "description": "work request queue length",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'n_wrk_queued',
                "call_back"  : get_value,
                "units"      : "req",
                "description": "N queued work requests",
                }))
    
    descriptors.append( create_desc(Desc_Skel, {
                "name"       : NAME_PREFIX + 'vmods',
                "call_back"  : get_value,
                "units"      : "vmods",
                "description": "Loaded VMODs",
                }))



    return descriptors


def metric_cleanup():
    """Cleanup"""

    pass


# the following code is for debugging and testing
if __name__ == '__main__':
    descriptors = metric_init(PARAMS)
    while True:
        for d in descriptors:
            print (('%s = %s') % (d['name'], d['format'])) % (d['call_back'](d['name']))
        print 'Sleeping 15 seconds'
        time.sleep(15)
