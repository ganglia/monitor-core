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

# This module reads a list of mountpoints from the "mounts" parameter (probably
# /proc/mounts) and creates a "disk_free_(absolute|percent)_*" metric for each
# mountpoint it finds.


import re
import os

NAME_PREFIX = 'disk_free_'
PARAMS = {
    'mounts': '/proc/mounts',
    'min_disk_size': 1,
    'explicit_mounts_to_check': ''
}
PATHS = {}


def get_value(name):
    """Return a value for the requested metric"""

    # parse unit type and path from name
    name_parser = re.match("^%s(absolute|percent)_(.*)$" % NAME_PREFIX, name)
    unit_type = name_parser.group(1)
    if name_parser.group(2) == 'rootfs':
        path = '/'
    elif name_parser.group(2) in PATHS:
        path = '/' + PATHS[name_parser.group(2)]
    else:
        path = '/' + name_parser.group(2).replace('_', '/')

    # get fs stats
    try:
        disk = os.statvfs(path)
        if unit_type == 'percent':
            result = (float(disk.f_bavail) / float(disk.f_blocks)) * 100
        else:
            result = (disk.f_bavail * disk.f_frsize) / float(2**30)  # GB

    except OSError:
        result = 0

    except ZeroDivisionError:
        result = 0

    return result


def metric_init(lparams):
    """Initialize metric descriptors"""

    global PARAMS, PATHS

    # set parameters
    for key in lparams:
        PARAMS[key] = lparams[key]

    # read mounts file
    try:
        f = open(PARAMS['mounts'])
    except IOError:
        f = []

    # Let's see if there are any mounts we need to add to the explicit list of mounts to check
    explicit_mounts_to_check = list()

    if "explicit_mounts_to_check" in PARAMS and PARAMS['explicit_mounts_to_check'] != "":
        explicit_mounts_temp = PARAMS['explicit_mounts_to_check'].split(" ")
        for mount in explicit_mounts_temp:
            explicit_mounts_to_check.append( mount[1:].replace('/', '_') )

        del explicit_mounts_temp

    # parse mounts and create descriptors
    descriptors = []
    for line in f:
        # We only want local file systems
        if line.startswith('/') or line.startswith('tmpfs'):
            mount_info = line.split()

            # create key from path
            if mount_info[1] == '/':
                path_key = 'rootfs'
            else:
                # strip off leading slash
                path_key = mount_info[1][1:].replace('/', '_')

            # Calculate the size of the disk. We'll use it exclude small disks
            disk = os.statvfs(mount_info[1])
            disk_size = (disk.f_blocks * disk.f_frsize) / float(2**30)

            if (disk_size > PARAMS['min_disk_size'] and mount_info[1] != "/dev") or path_key in explicit_mounts_to_check:
                PATHS[path_key] = mount_info[1]
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
    PARAMS = {
        'mounts': '/proc/mounts',
        'min_disk_size': 1,
        'explicit_mounts_to_check': ""
    }
    descriptors = metric_init(PARAMS)
    for d in descriptors:
        print (('%s = %s') % (d['name'], d['format'])) % (d['call_back'](d['name']))
