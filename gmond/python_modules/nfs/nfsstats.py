#!/usr/bin/python

import os
import stat
import re
import time
import syslog
import sys
import string

def test_proc( p_file, p_string ):
    global p_match
    """
    Check if <p_file> contains keyword <p_string> e.g. proc3, proc4
    """

    p_fd = open( p_file )

    p_contents = p_fd.read()

    p_fd.close()

    p_match = re.search(".*" + p_string + "\s.*", p_contents, flags=re.MULTILINE)

    if not p_match:
        return False
    else:
        return True

verboselevel = 0
descriptors = [ ]
old_values = { }
#  What we want ganglia to monitor, where to find it, how to extract it, ...
configtable = [
    {
        'group': 'nfs_client',
        'tests': [ 'stat.S_ISREG(os.stat("/proc/net/rpc/nfs").st_mode)', 'test_proc("/proc/net/rpc/nfs", "proc3")' ],
        'prefix': 'nfs_v3_',
        #  The next 4 lines can be at the 'group' level or the 'name' level
        'file': '/proc/net/rpc/nfs',
        'value_type': 'float',
        'units': 'calls/sec',
        'format': '%f',
        'names': {
            'total':       { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){2}(\d+.*\d)\n" },
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
        'group': 'nfs_client_v4',
        'tests': [ 'stat.S_ISREG(os.stat("/proc/net/rpc/nfs").st_mode)', 'test_proc("/proc/net/rpc/nfs", "proc4")' ],
        'prefix': 'nfs_v4_',
        #  The next 4 lines can be at the 'group' level or the 'name' level
        'file': '/proc/net/rpc/nfs',
        'value_type': 'float',
        'units': 'calls/sec',
        'format': '%f',
        'names': {
            'total':        { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){2}(\d+.*\d)\n" },
            'read':         { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){2}(\S*)" },
            'write':        { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){3}(\S*)" },
            'commit':       { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){4}(\S*)" },
            'open':         { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){5}(\S*)" },
            'open_conf':    { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){6}(\S*)" },
            'open_noat':    { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){7}(\S*)" },
            'open_dgrd':    { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){8}(\S*)" },
            'close':        { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){9}(\S*)" },
            'setattr':      { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){10}(\S*)" },
            'renew':        { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){11}(\S*)" },
            'setclntid':    { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){12}(\S*)" },
            'confirm':      { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){13}(\S*)" },
            'lock':         { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){14}(\S*)" },
            'lockt':        { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){15}(\S*)" },
            'locku':        { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){16}(\S*)" },
            'access':       { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){17}(\S*)" },
            'getattr':      { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){18}(\S*)" },
            'lookup':       { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){19}(\S*)" },
            'lookup_root':  { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){20}(\S*)" },
            'remove':       { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){21}(\S*)" },
            'rename':       { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){22}(\S*)" },
            'link':         { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){23}(\S*)" },
            'symlink':      { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){24}(\S*)" },
            'create':       { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){25}(\S*)" },
            'pathconf':     { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){26}(\S*)" },
            'statfs':       { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){27}(\S*)" },
            'readlink':     { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){28}(\S*)" },
            'readdir':      { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){29}(\S*)" },
            'server_caps':  { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){30}(\S*)" },
            'delegreturn':  { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){31}(\S*)" },
            'getacl':       { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){32}(\S*)" },
            'setacl':       { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){33}(\S*)" },
            'fs_locations': { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){34}(\S*)" },
            'rel_lkowner':  { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){35}(\S*)" },
            'secinfo':      { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){36}(\S*)" },
            'exchange_id':  { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){37}(\S*)" },
            'create_ses':   { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){38}(\S*)" },
            'destroy_ses':  { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){39}(\S*)" },
            'sequence':     { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){40}(\S*)" },
            'get_lease_t':  { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){41}(\S*)" },
            'reclaim_comp': { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){42}(\S*)" },
            'layoutget':    { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){43}(\S*)" },
            'getdevinfo':   { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){44}(\S*)" },
            'layoutcommit': { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){45}(\S*)" },
            'layoutreturn': { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){46}(\S*)" },
            'getdevlist':   { 'description':'dummy description', 're':  ".*proc4 (?:\S*\s){47}(\S*)" }
        }
    },
    {
        'group': 'nfs_server',
        'tests': [ 'stat.S_ISREG(os.stat("/proc/net/rpc/nfsd").st_mode)', 'test_proc("/proc/net/rpc/nfsd", "proc3")' ],
        'prefix': 'nfsd_v3_',
        #  The next 4 lines can be at the 'group' level or the 'name' level
        'file': '/proc/net/rpc/nfsd',
        'value_type': 'float',
        'units': 'calls/sec',
        'format': '%f',
        'names': {
            'total':       { 'description':'dummy description', 're':  ".*proc3 (?:\S*\s){2}(\d+.*\d)\n" },
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
    },
    {
        'group': 'nfs_server_v4',
        'tests': [ 'stat.S_ISREG(os.stat("/proc/net/rpc/nfsd").st_mode)', 'test_proc("/proc/net/rpc/nfsd", "proc4ops")' ],
        'prefix': 'nfsd_v4_',
        #  The next 4 lines can be at the 'group' level or the 'name' level
        'file': '/proc/net/rpc/nfsd',
        'value_type': 'float',
        'units': 'calls/sec',
        'format': '%f',
        'names': {
            'total':         { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){2}(\d+.*\d)\n" },
            'op0-unused':    { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){1}(\S*)" },
            'op1-unused':    { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){2}(\S*)" },
            'op2-future':    { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){3}(\S*)" },
            'access':        { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){4}(\S*)" },
            'close':         { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){5}(\S*)" },
            'commit':        { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){6}(\S*)" },
            'create':        { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){7}(\S*)" },
            'delegpurge':    { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){8}(\S*)" },
            'delegreturn':   { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){9}(\S*)" },
            'getattr':       { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){10}(\S*)" },
            'getfh':         { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){11}(\S*)" },
            'link':          { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){12}(\S*)" },
            'lock':          { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){13}(\S*)" },
            'lockt':         { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){14}(\S*)" },
            'locku':         { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){15}(\S*)" },
            'lookup':        { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){16}(\S*)" },
            'lookup_root':   { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){17}(\S*)" },
            'nverify':       { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){18}(\S*)" },
            'open':          { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){19}(\S*)" },
            'openattr':      { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){20}(\S*)" },
            'open_conf':     { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){21}(\S*)" },
            'open_dgrd':     { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){22}(\S*)" },
            'putfh':         { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){23}(\S*)" },
            'putpubfh':      { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){24}(\S*)" },
            'putrootfh':     { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){25}(\S*)" },
            'read':          { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){26}(\S*)" },
            'readdir':       { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){27}(\S*)" },
            'readlink':      { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){28}(\S*)" },
            'remove':        { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){29}(\S*)" },
            'rename':        { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){30}(\S*)" },
            'renew':         { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){31}(\S*)" },
            'restorefh':     { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){32}(\S*)" },
            'savefh':        { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){33}(\S*)" },
            'secinfo':       { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){34}(\S*)" },
            'setattr':       { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){35}(\S*)" },
            'setcltid':      { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){36}(\S*)" },
            'setcltidconf':  { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){37}(\S*)" },
            'verify':        { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){38}(\S*)" },
            'write':         { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){39}(\S*)" },
            'rellockowner':  { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){40}(\S*)" },
            'bc_ctl':        { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){41}(\S*)" },
            'bind_conn':     { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){42}(\S*)" },
            'exchange_id':   { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){43}(\S*)" },
            'create_ses':    { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){44}(\S*)" },
            'destroy_ses':   { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){45}(\S*)" },
            'free_stateid':  { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){46}(\S*)" },
            'getdirdeleg':   { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){47}(\S*)" },
            'getdevinfo':    { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){48}(\S*)" },
            'getdevlist':    { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){49}(\S*)" },
            'layoutcommit':  { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){50}(\S*)" },
            'layoutget':     { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){51}(\S*)" },
            'layoutreturn':  { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){52}(\S*)" },
            'secinfononam':  { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){53}(\S*)" },
            'sequence':      { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){54}(\S*)" },
            'set_ssv':       { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){55}(\S*)" },
            'test_stateid':  { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){56}(\S*)" },
            'want_deleg':    { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){57}(\S*)" },
            'destroy_clid':  { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){58}(\S*)" },
            'reclaim_comp':  { 'description':'dummy description', 're':  ".*proc4ops (?:\S*\s){59}(\S*)" }
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

        # 2nd param defines number of params that will follow (differs between NFS versions)
        max_plimit = re.split("\W+", p_match.group())[1]

        # Parse our defined params list in order to ensure list will not exceed max_plimit
        n = 0
        names_keys = configtable[i]['names'].keys()
        keys_to_remove = []
        for _tmpkey in names_keys:
            _tmplist = names_keys
            param_pos = re.split("{(\d+)\}", configtable[i]['names'][_tmpkey].values()[0])[1]
	    if int(param_pos) > int(max_plimit):
                keys_to_remove.append(_tmpkey)
            n += 1

        if len(keys_to_remove) > 0:
            for key in keys_to_remove:
                names_keys.remove(key)

        #  For each name in the group ...
        for name in names_keys:
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

    m_value = m.group(1)

    #RB: multiple (space seperated) values: calculate sum
    if string.count( m_value, ' ' ) > 0:
        m_fields = string.split( m_value, ' ' )

        sum_value = 0

        for f in m_fields:
            sum_value = sum_value + int(f)

        m_value = sum_value
    
    #  Return time and value.
    ts = time.time()
    return (ts, int(m_value))

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
