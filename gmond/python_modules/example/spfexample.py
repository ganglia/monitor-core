#/******************************************************************************
#* Copyright (C) 2007 Novell, Inc. All rights reserved.
#*
#* Redistribution and use in source and binary forms, with or without
#* modification, are permitted provided that the following conditions are met:
#*
#*  - Redistributions of source code must retain the above copyright notice,
#*    this list of conditions and the following disclaimer.
#*
#*  - Redistributions in binary form must reproduce the above copyright notice,
#*    this list of conditions and the following disclaimer in the documentation
#*    and/or other materials provided with the distribution.
#*
#*  - Neither the name of Novell, Inc. nor the names of its
#*    contributors may be used to endorse or promote products derived from this
#*    software without specific prior written permission.
#*
#* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
#* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#* ARE DISCLAIMED. IN NO EVENT SHALL Novell, Inc. OR THE CONTRIBUTORS
#* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#* POSSIBILITY OF SUCH DAMAGE.
#*
#* Author: Brad Nicholes (bnicholes novell.com)
#******************************************************************************/

import random, time

descriptors = list()

# This is a list of bogus hosts that this module will report
#  through the spoofing functionality.  Normally a spoofing
#  module would be expected to discover spoofed hosts through
#  some sort of query mechanism or by reading some type of
#  input file.
spoofHosts = [['spf1.myhost.org','10.10.0.1'], ['spf2.myhost.org','10.10.0.2'], ['spf3.myhost.org','10.10.0.3']]

# Default hardcoded metric values
location = 'unspecified'
osname = 'SuSE Linux Ultimate'
started = int(time.time())
bootTime = {}

def spf_heartbeat(name):
    '''Return heartbeat.'''
    return started

def spf_location(name):
    '''Return location.'''
    return location

def spf_boottime(name):
    '''Return boottime.'''
    return started

def spf_osname(name):
    '''Return OS Name.'''
    return osname

def Random_Numbers(name):
    '''Return a random number.'''
    return int(random.uniform(0, 100))

def Init_Metric (spfHost, name, tmax, metrictype, units, slope, fmt, desc, metricAlias, ipAddr, handler):
    '''Create a metric definition dictionary object for an imaginary box.'''
    metric_name = name + ':' + spfHost
    spoofHost = ipAddr + ':' + spfHost

    # name - Should result in a unique metric indentifier. Attaching the spoofed host name
    #          allows the same metric name to be reused while the resulting name is unique.
    # spoof_host - Must follow the same format as gmetric. The format should be the IP and
    #          host name separated by a colon.
    # spoof_name - Must be the original name of the metric that this definition will be spoofing.
    #          In other words, if this metric name is spf_boottime, the spoof_name would be
    #          boottime if this metric is meant to spoof the original boottime metric.


    d = {'name': metric_name,
        'call_back': handler,
        'time_max': tmax,
        'value_type': metrictype,
        'units': units,
        'slope': slope,
        'format': fmt,
        'description': desc,
        'spoof_host': spoofHost,
        'spoof_name': metricAlias}

    return d

def metric_init(params):
    '''Initialize the random number generator and create the
    metric definition dictionary object for each metric.'''
    global descriptors
    random.seed()

    for metric_name, ipaddr in spoofHosts:
        descriptors.append(Init_Metric(metric_name, 'spf_random_cpu_util', int(90),
                'uint', '%', 'both', '%u',  'Spoofed CPU Utilization', 'cpu_util', ipaddr, Random_Numbers))
        descriptors.append(Init_Metric(metric_name, 'spf_heartbeat', int(20),
                'uint', '', '', '%u',  'Spoofed Heartbeat', 'heartbeat', ipaddr, spf_heartbeat))
        descriptors.append(Init_Metric(metric_name, 'spf_location', int(1200),
                'string', '(x,y,z)', '', '%s',  'Spoofed Location', 'location', ipaddr, spf_location))
        descriptors.append(Init_Metric(metric_name, 'spf_boottime', int(1200),
                'uint', 's', 'zero', '%u',  'Spoofed Boot Time', 'boottime', ipaddr, spf_boottime))
        descriptors.append(Init_Metric(metric_name, 'spf_osname', int(1200),
                'string', '', 'zero', '%s',  'Spoofed Operating System Name', 'os_name', ipaddr, spf_osname))

    return descriptors

def metric_cleanup():
    '''Clean up the metric module.'''
    pass

#This code is for debugging and unit testing
if __name__ == '__main__':
    params = {}
    d = metric_init(params)
    for d in descriptors:
        v = d['call_back'](d['name'])
        print 'value for %s is %s' % (d['name'],  str(v))
    print  d
