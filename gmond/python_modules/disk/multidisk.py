#/*******************************************************************************
#* Portions Copyright (C) 2007 Novell, Inc. All rights reserved.
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

import statvfs
import os
import ganglia

descriptors = list()
    
def Find_Metric (name):
    '''Find the metric definition data given the metric name.
   The metric name should always be unique.'''
    for d in descriptors:
        if d['name'] == name:
            return d
    pass
    
def Remote_Mount(device, type):
    '''Determine if the device specifed is a local or remote device.'''
    return ((device.rfind(':') != -1) 
        or ((type == "smbfs") and device.startswith('//'))
        or type.startswith('nfs') or (type == 'autofs')
        or (type == 'gfs') or (type == 'none'))

def DiskTotal_Handler(name):
    '''Calculate the total disk space for the device that is associated
    with the metric name.'''
    d = Find_Metric(name)
    if not d:
        return 0

    st = os.statvfs(d['mount'])
    size = st[statvfs.F_BLOCKS]
    blocksize = st[statvfs.F_BSIZE]
    vv = (size * blocksize) / 1e9
    return vv

def DiskUsed_Handler(name):
    '''Calculate the used disk space for the device that is associated
    with the metric name.'''
    d = Find_Metric(name)
    if not d:
        return float(0)
    
    st = os.statvfs(d['mount'])
    free = st[statvfs.F_BAVAIL]
    size = st[statvfs.F_BLOCKS]
    
    if size:
        return ((size - free) / float(size)) * 100
    else:
        return float(0)

def Init_Metric (line, name, tmax, type, units, slope, fmt, desc, handler):
    '''Create a metric definition dictionary object for a device.'''
    metric_name = line[0] + '-' + name
    
    d = {'name': metric_name.replace('/', '-').lstrip('-'),
        'call_back': handler,
        'time_max': tmax,
        'value_type': type,
        'units': units,
        'slope': slope,
        'format': fmt,
        'description': desc,
        'groups': 'disk',
        'mount': line[1]}
    return d
    

def metric_init(params):
    '''Discover all of the local disk devices on the system and create
    a metric definition dictionary object for each.'''
    global descriptors
    f = open('/proc/mounts', 'r')
    
    for l in f:
        line = l.split()
        if line[3].startswith('ro'): continue
        elif Remote_Mount(line[0], line[2]): continue
        elif (not line[0].startswith('/dev/')) and (not line[0].startswith('/dev2/')): continue;
        
	if ganglia.get_debug_msg_level() > 1:
            print 'Discovered device %s' % line[1]
        
        descriptors.append(Init_Metric(line, 'disk_total', int(1200), 
            'double', 'GB', 'both', '%.3f', 
            'Available disk space', DiskTotal_Handler))
        descriptors.append(Init_Metric(line, 'disk_used', int(180), 
            'float', '%', 'both', '%.1f', 
            'Percent used disk space', DiskUsed_Handler))
        
    f.close()
    return descriptors

def metric_cleanup():
    '''Clean up the metric module.'''
    pass

#This code is for debugging and unit testing    
if __name__ == '__main__':
    metric_init(None)
    for d in descriptors:
        v = d['call_back'](d['name'])
        print 'value for %s is %f' % (d['name'],  v)
