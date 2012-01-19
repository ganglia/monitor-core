#!/usr/bin/python

import os
import stat
import re
import time
import syslog
import sys

verboselevel = 0
descriptors = [ ]
old_values = { }
#  What we want ganglia to monitor, where to find it, how to extract it, ...
configtable = [
    {
        'group': 'nfs_client',
        'tests': [ 'stat.S_ISREG(os.stat("/proc/net/rpc/nfs").st_mode)' ],
        'prefix': 'nfs_v3_',
        #  The next 4 lines can be at the 'group' level or the 'name' level
        'file': '/proc/net/rpc/nfs',
        'value_type': 'float',
        'units': 'calls/sec',
        'format': '%f',
        'names': {
            'getattr':     { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){2}(\S*)" },
            'setattr':     { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){3}(\S*)" },
            'lookup':      { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){4}(\S*)" },
            'access':      { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){5}(\S*)" },
            'readlink':    { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){6}(\S*)" },
            'read':        { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){7}(\S*)" },
            'write':       { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){8}(\S*)" },
            'create':      { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){9}(\S*)" },
            'mkdir':       { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){10}(\S*)" },
            'symlink':     { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){11}(\S*)" },
            'mknod':       { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){12}(\S*)" },
            'remove':      { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){13}(\S*)" },
            'rmdir':       { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){14}(\S*)" },
            'rename':      { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){15}(\S*)" },
            'link':        { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){16}(\S*)" },
            'readdir':     { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){17}(\S*)" },
            'readdirplus': { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){18}(\S*)" },
            'fsstat':      { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){19}(\S*)" },
            'fsinfo':      { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){20}(\S*)" },
            'pathconf':    { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){21}(\S*)" },
            'commit':      { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){22}(\S*)" }
        }
    },
    {
        'group': 'nfs_server',
        'tests': [ 'stat.S_ISREG(os.stat("/proc/net/rpc/nfsd").st_mode)' ],
        'prefix': 'nfsd_v3_',
        #  The next 4 lines can be at the 'group' level or the 'name' level
        'file': '/proc/net/rpc/nfsd',
        'value_type': 'float',
        'units': 'calls/sec',
        'format': '%f',
        'names': {
            'getattr':     { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){2}(\S*)" },
            'setattr':     { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){3}(\S*)" },
            'lookup':      { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){4}(\S*)" },
            'access':      { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){5}(\S*)" },
            'readlink':    { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){6}(\S*)" },
            'read':        { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){7}(\S*)" },
            'write':       { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){8}(\S*)" },
            'create':      { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){9}(\S*)" },
            'mkdir':       { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){10}(\S*)" },
            'symlink':     { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){11}(\S*)" },
            'mknod':       { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){12}(\S*)" },
            'remove':      { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){13}(\S*)" },
            'rmdir':       { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){14}(\S*)" },
            'rename':      { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){15}(\S*)" },
            'link':        { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){16}(\S*)" },
            'readdir':     { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){17}(\S*)" },
            'readdirplus': { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){18}(\S*)" },
            'fsstat':      { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){19}(\S*)" },
            'fsinfo':      { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){20}(\S*)" },
            'pathconf':    { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){21}(\S*)" },
            'commit':      { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){22}(\S*)" }
        },
    }
]

#  Ganglia will call metric_init(), which should return a dictionary for each thing that it
#  should monitor, including the name of the callback function to get the current value.
def metric_init(params):
    global descriptors
    global old_values
    global configtable

    for i in range(0, len(configtable)):
        #  Don't set up dictionary for any group member if group applicability tests fail.
        tests_passed = True
        for j in range(0, len(configtable[i]['tests'])):
            try:
                if eval(configtable[i]['tests'][j]):
		    pass
                else:
                    tests_passed = False
                    break
            except:
               tests_passed = False
               break
        if not tests_passed:
            continue
        #  For each name in the group ...
        for name in configtable[i]['names'].keys():
            #  ... set up dictionary ...
            if 'format' in configtable[i]['names'][name]:
		format_str = configtable[i]['names'][name]['format']
            else:
		format_str = configtable[i]['format']
            if 'units' in configtable[i]['names'][name]:
		unit_str = configtable[i]['names'][name]['units']
            else:
		unit_str = configtable[i]['units']
            if 'value_type' in configtable[i]['names'][name]:
		value_type_str = configtable[i]['names'][name]['value_type']
            else:
		value_type_str = configtable[i]['value_type']
            if 'file' in configtable[i]['names'][name]:
		file_str = configtable[i]['names'][name]['file']
            else:
		file_str = configtable[i]['file']
		

            descriptors.append({
                'name': configtable[i]['prefix'] + name,
                'call_back': call_back,
                'time_max': 90,
                'format': format_str,
                'units': unit_str,
                'value_type': value_type_str,
                'slope': 'both',
                'description': configtable[i]['names'][name]['description'],
                'groups': configtable[i]['group'],
                #  The following are module-private data stored in a public variable
                'file': file_str,
                're': configtable[i]['names'][name]['re']
            })
            #  And get current value cached as previous value, for future comparisons.
            (ts, value) =  get_value(configtable[i]['prefix'] + name)
            old_values[configtable[i]['prefix'] + name] = { 
                'time':ts,
                'value':value
            }

    #  Pass ganglia the complete list of dictionaries.
    return descriptors

#  Ganglia will call metric_cleanup() when it exits.
def metric_cleanup():
    pass

#  metric_init() registered this as the callback function.
def call_back(name):
    global old_values

    #  Get new value
    (new_time, new_value) = get_value(name)
 
    #  Calculate rate of change 
    try:
        rate = (new_value - old_values[name]['value'])/(new_time - old_values[name]['time'])
    except ZeroDivisionError:
        rate = 0.0

    #  Stash values for comparison next time round.
    old_values[name]['value'] = new_value
    old_values[name]['time'] = new_time
    return rate

def get_value(name):
    global descriptors

    #  Search descriptors array for this name's file and extractor RE
    for i in range(0, len(descriptors)):
        if descriptors[i]['name'] == name:
            break
    contents = file(descriptors[i]['file']).read()
    m = re.search(descriptors[i]['re'], contents, flags=re.MULTILINE)
    
    #  Return time and value.
    ts = time.time()
    return (ts, int(m.group(1)))

def debug(level, text):
    global verboselevel
    if level > verboselevel:
        return
    if sys.stderr.isatty():
        print text
    else:
        syslog.syslog(text)

#This code is for debugging and unit testing
if __name__ == '__main__':
    metric_init(None)
    #  wait some time time for some real data
    time.sleep(5)
    for d in descriptors:
        v = d['call_back'](d['name'])
        debug(10, ('__main__: value for %s is %s' % (d['name'], d['format'])) % (v))
