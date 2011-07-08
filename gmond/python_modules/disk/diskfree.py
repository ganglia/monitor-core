#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Disk Free gmond module for Ganglia
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

import re
import os


NAME_PREFIX = 'disk_free_'
PARAMS = {
    'mounts' : '/proc/mounts'
}


def get_value(name):
    """Return a value for the requested metric"""

    # parse unit type and path from name
    name_parser = re.match("^%s([a-z]+)_(\w+)$" % NAME_PREFIX, name)
    unit_type = name_parser.group(1)
    if name_parser.group(2) == 'rootfs':
        path = '/'
    else:
        path = '/' + name_parser.group(2).replace('_', '/')

    # get fs stats
    try:
        disk = os.statvfs(path)
        if unit_type == 'percent':
            result = (float(disk.f_bavail) / float(disk.f_blocks)) * 100
        else:
            result = (disk.f_bavail * disk.f_frsize) / float(2**30) # GB

    except OSError:
        result = 0

    except ZeroDivisionError:
        result = 0

    return result


def metric_init(lparams):
    """Initialize metric descriptors"""

    global PARAMS

    # set parameters
    for key in lparams:
        PARAMS[key] = lparams[key]

    # read mounts file
    try:
        f = open(PARAMS['mounts'])
    except IOError:
        f = []

    # parse mounts and create descriptors
    descriptors = []
    for line in f:
        if line.startswith('/'):
            mount_info = line.split()

            # create key from path
            if mount_info[1] == '/':
                path_key = 'rootfs'
            else:
                path_key = mount_info[1][1:].replace('/', '_')

            for unit_type in ['absolute', 'percent']:
                if unit_type == 'percent': 
			units = '%'
		else:
			units = 'GB'
                descriptors.append({
                    'name': NAME_PREFIX + unit_type + '_' + path_key,
                    'call_back': get_value,
                    'time_max': 60,
                    'value_type': 'float',
                    'units': units,
                    'slope': 'both',
                    'format': '%f',
                    'description': "Disk space available (%s) on %s" % (units, mount_info[1]),
                    'groups': 'disk'
                })

    return descriptors


def metric_cleanup():
    """Cleanup"""

    pass


# the following code is for debugging and testing
if __name__ == '__main__':
    descriptors = metric_init(PARAMS)
    for d in descriptors:
        print (('%s = %s') % (d['name'], d['format'])) % (d['call_back'](d['name']))
