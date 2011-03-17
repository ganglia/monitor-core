#/******************************************************************************
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

import os

OBSOLETE_POPEN = False
try:
    import subprocess
except ImportError:
    import popen2
    OBSOLETE_POPEN = True

import threading
import time

_WorkerThread = None    #Worker thread object
_glock = threading.Lock()   #Synchronization lock
_refresh_rate = 30 #Refresh rate of the netstat data

#Global dictionary storing the counts of the last connection state
# read from the netstat output
_conns = {'tcp_established': 0,
        'tcp_listen': 0,
        'tcp_timewait':0,
        'tcp_closewait':0,
        'tcp_synsent':0,
        'tcp_synrecv':0,
        'tcp_synwait':0,
        'tcp_finwait1':0,
        'tcp_finwait2':0,
        'tcp_closed':0,
        'tcp_lastack':0,
        'tcp_closing':0,
        'tcp_unknown':0}

def TCP_Connections(name):
    '''Return the requested connection type status.'''
    global _WorkerThread
    
    if _WorkerThread is None:
        print 'Error: No netstat data gathering thread created for metric %s' % name
        return 0
        
    if not _WorkerThread.running and not _WorkerThread.shuttingdown:
        try:
            _WorkerThread.start()
        except (AssertionError, RuntimeError):
            pass

    #Read the last connection total for the state requested. The metric
    # name passed in matches the dictionary slot for the state value.
    _glock.acquire()
    ret = int(_conns[name])
    _glock.release()
    return ret

#Metric descriptions
_descriptors = [{'name': 'tcp_established',
        'call_back': TCP_Connections,
        'time_max': 20,
        'value_type': 'uint',
        'units': 'Sockets',
        'slope': 'both',
        'format': '%u',
        'description': 'Total number of established TCP connections',
        'groups': 'network',
        },
    {'name': 'tcp_listen',
        'call_back': TCP_Connections,
        'time_max': 20,
        'value_type': 'uint',
        'units': 'Sockets',
        'slope': 'both',
        'format': '%u',
        'description': 'Total number of listening TCP connections',
        'groups': 'network',
        },
    {'name': 'tcp_timewait',
        'call_back': TCP_Connections,
        'time_max': 20,
        'value_type': 'uint',
        'units': 'Sockets',
        'slope': 'both',
        'format': '%u',
        'description': 'Total number of time_wait TCP connections',
        'groups': 'network',
        },
    {'name': 'tcp_closewait',
        'call_back': TCP_Connections,
        'time_max': 20,
        'value_type': 'uint',
        'units': 'Sockets',
        'slope': 'both',
        'format': '%u',
        'description': 'Total number of close_wait TCP connections',
        'groups': 'network',
        },
    {'name': 'tcp_synsent',
        'call_back': TCP_Connections,
        'time_max': 20,
        'value_type': 'uint',
        'units': 'Sockets',
        'slope': 'both',
        'format': '%u',
        'description': 'Total number of syn_sent TCP connections',
        'groups': 'network',
        },
    {'name': 'tcp_synrecv',
        'call_back': TCP_Connections,
        'time_max': 20,
        'value_type': 'uint',
        'units': 'Sockets',
        'slope': 'both',
        'format': '%u',
        'description': 'Total number of syn_recv TCP connections',
        'groups': 'network',
        },
    {'name': 'tcp_synwait',
        'call_back': TCP_Connections,
        'time_max': 20,
        'value_type': 'uint',
        'units': 'Sockets',
        'slope': 'both',
        'format': '%u',
        'description': 'Total number of syn_wait TCP connections',
        'groups': 'network',
        },
    {'name': 'tcp_finwait1',
        'call_back': TCP_Connections,
        'time_max': 20,
        'value_type': 'uint',
        'units': 'Sockets',
        'slope': 'both',
        'format': '%u',
        'description': 'Total number of fin_wait1 TCP connections',
        'groups': 'network',
        },
    {'name': 'tcp_finwait2',
        'call_back': TCP_Connections,
        'time_max': 20,
        'value_type': 'uint',
        'units': 'Sockets',
        'slope': 'both',
        'format': '%u',
        'description': 'Total number of fin_wait2 TCP connections',
        'groups': 'network',
        },
    {'name': 'tcp_closed',
        'call_back': TCP_Connections,
        'time_max': 20,
        'value_type': 'uint',
        'units': 'Sockets',
        'slope': 'both',
        'format': '%u',
        'description': 'Total number of closed TCP connections',
        'groups': 'network',
        },
    {'name': 'tcp_lastack',
        'call_back': TCP_Connections,
        'time_max': 20,
        'value_type': 'uint',
        'units': 'Sockets',
        'slope': 'both',
        'format': '%u',
        'description': 'Total number of last_ack TCP connections',
        'groups': 'network',
        },
    {'name': 'tcp_closing',
        'call_back': TCP_Connections,
        'time_max': 20,
        'value_type': 'uint',
        'units': 'Sockets',
        'slope': 'both',
        'format': '%u',
        'description': 'Total number of closing TCP connections',
        'groups': 'network',
        },
    {'name': 'tcp_unknown',
        'call_back': TCP_Connections,
        'time_max': 20,
        'value_type': 'uint',
        'units': 'Sockets',
        'slope': 'both',
        'format': '%u',
        'description': 'Total number of unknown TCP connections',
        'groups': 'network',
        }]

class NetstatThread(threading.Thread):
    '''This thread continually gathers the current states of the tcp socket
    connections on the machine.  The refresh rate is controlled by the 
    RefreshRate parameter that is passed in through the gmond.conf file.'''

    def __init__(self):
        threading.Thread.__init__(self)
        self.running = False
        self.shuttingdown = False
        self.popenChild = None

    def shutdown(self):
        self.shuttingdown = True
        if self.popenChild != None:
            try:
                self.popenChild.wait()
            except OSError, e:
                if e.errno == 10: # No child processes
                    pass

        if not self.running:
            return
        self.join()

    def run(self):
        global _conns, _refresh_rate
        
        #Array positions for the connection type and state data
        # acquired from the netstat output.
        tcp_at = 0
        tcp_state_at = 5
        
        #Make a temporary copy of the tcp connecton dictionary.
        tempconns = _conns.copy()
        
        #Set the state of the running thread
        self.running = True
        
        #Continue running until a shutdown event is indicated
        while not self.shuttingdown:
            if self.shuttingdown:
                break
                
            #Zero out the temporary connection state dictionary.
            for conn in tempconns:
                tempconns[conn] = 0

            #Call the netstat utility and split the output into separate lines
            if not OBSOLETE_POPEN:
                self.popenChild = subprocess.Popen(["netstat", '-t', '-a', '-n'], stdout=subprocess.PIPE)
                lines = self.popenChild.communicate()[0].split('\n')
            else:
                self.popenChild = popen2.Popen3("netstat -t -a -n")
                lines = self.popenChild.fromchild.readlines()

            try:
                self.popenChild.wait()
            except OSError, e:
                if e.errno == 10: # No child process
                    continue
            
            #Iterate through the netstat output looking for the 'tcp' keyword in the tcp_at 
            # position and the state information in the tcp_state_at position. Count each 
            # occurance of each state.
            for tcp in lines:
                # skip empty lines
                if tcp == '':
                    continue

                line = tcp.split()
                if line[tcp_at] == 'tcp':
                    if line[tcp_state_at] == 'ESTABLISHED':
                        tempconns['tcp_established'] += 1
                    elif line[tcp_state_at] == 'LISTEN':
                        tempconns['tcp_listen'] += 1
                    elif line[tcp_state_at] == 'TIME_WAIT':
                        tempconns['tcp_timewait'] += 1
                    elif line[tcp_state_at] == 'CLOSE_WAIT':
                        tempconns['tcp_closewait'] += 1
                    elif line[tcp_state_at] == 'SYN_SENT':
                        tempconns['tcp_synsent'] += 1
                    elif line[tcp_state_at] == 'SYN_RECV':
                        tempconns['tcp_synrecv'] += 1
                    elif line[tcp_state_at] == 'SYN_WAIT':
                        tempconns['tcp_synwait'] += 1
                    elif line[tcp_state_at] == 'FIN_WAIT1':
                        tempconns['tcp_finwait1'] += 1
                    elif line[tcp_state_at] == 'FIN_WAIT2':
                        tempconns['tcp_finwait2'] += 1
                    elif line[tcp_state_at] == 'CLOSED':
                        tempconns['tcp_closed'] += 1
                    elif line[tcp_state_at] == 'LAST_ACK':
                        tempconns['tcp_lastack'] += 1
                    elif line[tcp_state_at] == 'CLOSING':
                        tempconns['tcp_closing'] += 1
                    elif line[tcp_state_at] == 'UNKNOWN':
                        tempconns['tcp_unknown'] += 1
                        
            #Acquire a lock and copy the temporary connection state dictionary
            # to the global state dictionary.
            _glock.acquire()
            for conn in _conns:
                _conns[conn] = tempconns[conn]
            _glock.release()
            
            #Wait for the refresh_rate period before collecting the netstat data again.
            if not self.shuttingdown:
                time.sleep(_refresh_rate)
            
        #Set the current state of the thread after a shutdown has been indicated.
        self.running = False

def metric_init(params):
    '''Initialize the tcp connection status module and create the
    metric definition dictionary object for each metric.'''
    global _refresh_rate, _WorkerThread
    
    #Read the refresh_rate from the gmond.conf parameters.
    if 'RefreshRate' in params:
        _refresh_rate = int(params['RefreshRate'])
    
    #Start the worker thread
    _WorkerThread = NetstatThread()
    
    #Return the metric descriptions to Gmond
    return _descriptors

def metric_cleanup():
    '''Clean up the metric module.'''
    
    #Tell the worker thread to shutdown
    _WorkerThread.shutdown()

#This code is for debugging and unit testing    
if __name__ == '__main__':
    params = {'Refresh': '20'}
    metric_init(params)
    while True:
        try:
            for d in _descriptors:
                v = d['call_back'](d['name'])
                print 'value for %s is %u' % (d['name'],  v)
            time.sleep(5)
        except KeyboardInterrupt:
            os._exit(1)
