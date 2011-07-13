#       xenstats.py
#       
#       Copyright 2011 Marcos Amorim <marcosmamorim@gmail.com>
#       
#       This program is free software; you can redistribute it and/or modify
#       it under the terms of the GNU General Public License as published by
#       the Free Software Foundation; either version 2 of the License, or
#       (at your option) any later version.
#       
#       This program is distributed in the hope that it will be useful,
#       but WITHOUT ANY WARRANTY; without even the implied warranty of
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#       GNU General Public License for more details.
#       
#       You should have received a copy of the GNU General Public License
#       along with this program; if not, write to the Free Software
#       Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#       MA 02110-1301, USA.

import libvirt
import os
import time

descriptors = list()
conn        = libvirt.openReadOnly("xen:///")
conn_info   = conn.getInfo()

def xen_vms(name):
    '''Return number of virtual is running'''
    global conn

    vm_count   = conn.numOfDomains()

    return vm_count

def xen_mem(name):
    '''Return node memory '''
    global conn
    global conn_info

    # O xen retorna o valor da memoria em MB, vamos passar por KB
    return conn_info[1] * 1024

def xen_cpu(name):
    '''Return numbers of CPU's'''
    global conn
    global conn_info

    return conn_info[2]


def xen_mem_use(name):
    '''Return total memory usage'''
    global conn

    vm_mem = 0

    for id in conn.listDomainsID():
        dom = conn.lookupByID(id)
        info = dom.info()
        vm_mem = vm_mem + info[2]

    return vm_mem

def metric_init(params):
    global descriptors

    d1 = {'name': 'xen_vms',
            'call_back': xen_vms,
            'time_max': 20,
            'value_type': 'uint',
            'units': 'Qtd',
            'slope': 'both',
            'format': '%d',
            'description': 'Total number of running vms',
            'groups': 'xen',
            }
    d2 = {'name': 'xen_cpu',
            'call_back': xen_cpu,
            'time_max': 20,
            'value_type': 'uint',
            'units': 'CPUs',
            'slope': 'both',
            'format': '%d',
            'description': 'CPUs',
            'groups': 'xen'
            }

    d3 = {'name': 'xen_mem',
            'call_back': xen_mem,
            'time_max': 20,
            'value_type': 'uint',
            'units': 'KB',
            'slope': 'both',
            'format': '%d',
            'description': 'Total memory Xen',
            'groups': 'xen'
            }

    d4 = {'name': 'xen_mem_use',
            'call_back': xen_mem_use,
            'time_max': 20,
            'value_type': 'uint',
            'units': 'KB',
            'slope': 'both',
            'format': '%d',
            'description': 'Memory Usage',
            'groups': 'xen'
            }

    descriptors = [d1,d2,d3,d4]

    return descriptors

def metric_cleanup():
    '''Clean up the metric module.'''
    global conn
    conn.close()
    pass

#This code is for debugging and unit testing
if __name__ == '__main__':
    metric_init('init')
    for d in descriptors:
        v = d['call_back'](d['name'])
        print 'value for %s is %u' % (d['name'],  v)

