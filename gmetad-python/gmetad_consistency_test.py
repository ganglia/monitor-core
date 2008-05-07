#!/usr/bin/env python

#/*******************************************************************************
#* Portions Copyright (C) 2008 Novell, Inc. All rights reserved.
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
#* Authors: Matt Ryan (mrayn novell.com)
#*                  Brad Nicholes (bnicholes novell.com)
#******************************************************************************/

import optparse
import os
import time
import signal
import socket
import xml.sax
import sys
from urlparse import urlsplit

class GmetadElement:
    def __init__(self, id):
        self.id = id
        self._data = {}
        self.child_elements = []
        
    def __getitem__(self, key):
        return self._data[key]
        
    def __setitem__(self, key, value):
        self._data[key] = value
        
    def __str__(self):
        buf = 'ID: %s\nAttrs:' % self.id
        for k, v in self._data.items():
            buf += ' %s=>%s' % (k,v)
        buf += '\n'
        for ce in self.child_elements:
            buf += str(ce)
        return buf
        

class GmetadXmlHandler(xml.sax.ContentHandler):
    def __init__(self):
        xml.sax.ContentHandler.__init__(self)
        self.elemList = []
        self._elemStack = []
        self._elemStackSize = 0
        
    def startElement(self, tag, attrs):
        newElem = GmetadElement(tag)
        for ak, av in attrs.items():
            newElem[ak] = av
        if self._elemStackSize:
            self._elemStack[self._elemStackSize-1].child_elements.append(newElem)
        else:
            self.elemList.append(newElem)
        self._elemStack.append(newElem)
        self._elemStackSize += 1
        
    def endElement(self, tag):
        self._elemStack.pop()
        self._elemStackSize -= 1    

def urlCompare(u1, u2):
    url1 = urlsplit(u1)
    url2 = urlsplit(u2)
    if url1[0] != url2[0] or url1[2] != url2[2] or url1[3] != url2[3] or url1[4] != url2[4]:
        return False
    url1host = ''
    url1port = None
    url2host = ''
    url2port = None
    try:
        url1host, url1port = url1[1].split(':')
    except ValueError:
        pass
    try:
        url2host, urh2port = url2[1].split(':')
    except ValueError:
        pass
    if url1port != url2port:
        return False
    url1host_hostname = url1host
    url2host_hostname = url2host
    try:
        url1host_hostname, remnants = url1host.split('.',1)
    except ValueError:
        pass
    try:
        url2host_hostname, remnants = url2host.split('.',1)
    except ValueError:
        pass
    if url1host_hostname != url2host_hostname:
        return False
    return True
    
ignore_attr_values = ['LOCALTIME', 'TN', 'REPORTED']
        
def checkEquivalentXml(oldelem, newelem):
    global ignore_attr_values
    if oldelem.id != newelem.id:
        raise Exception, 'Element ids do not match (old=%s, new=%s)' % (oldelem.id, newelem.id)
    if len(oldelem._data) != len(newelem._data):
        raise Exception, 'Element attribute numbers do not match at node %s (old=%d, new=%d)' % (oldelem.id, len(oldelem._data), len(newelem._data))
    if len(oldelem.child_elements) != len(newelem.child_elements):
        raise Exception, 'Element children numbers do not match at node %s (old=%d, new=%d)' % (oldelem.id, len(oldelem.child_elements), len(newelem.child_elements))
    for k in oldelem._data.keys():
        if not newelem._data.has_key(k):
            raise Exception, 'Attribute "%s" not found in new XML' % k
        if oldelem[k] != newelem[k]:
            if k in ignore_attr_values:
                # Skip context-sensitive values
                continue
            if oldelem.id == 'METRIC' and k == 'VAL':
                # Skip metric values, since they are measured real-time and can change.
                continue
            if oldelem.id == 'GRID' and k == 'AUTHORITY':
                if urlCompare(oldelem[k], newelem[k]):
                    continue
            raise Exception, 'Value for attribute "%s" of tag %s does not match (old=%s, new=%s)' % (k, oldelem.id, oldelem[k], newelem[k])
    for k in newelem._data.keys():
        if not oldelem._data.has_key(k):
            raise Exception, 'Attribute "%s" not found in old XML' % k
        if oldelem[k] != newelem[k]:
            if k in ignore_attr_values:
                # Skip context-sensitive values
                continue
            if oldelem.id == 'METRIC' and k == 'VAL':
                # Skip metric values, since they are measured real-time and can change.
                continue
            if oldelem.id == 'GRID' and k == 'AUTHORITY':
                if urlCompare(oldelem[k], newelem[k]):
                    continue
            raise Exception, 'Value for attribute "%s" of tag %s does not match (old=%s, new=%s)' % (k, oldelem.id, oldelem[k], newelem[k])
    for oce in oldelem.child_elements:
        for nce in newelem.child_elements:
            if oce._data.has_key('NAME') and nce._data.has_key('NAME'):
                if oce['NAME'] == nce['NAME']:
                    checkEquivalentXml(oce, nce)
                    break
            else:
                checkEquivalentXml(oce, nce)
        
def get_socket(hspec):
    host, port = hspec.split(':')
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, int(port)))
    return sock
    
def get_xml_from_socket(sock):
    xmlbuf = ''
    while True:
        buf = sock.recv(8192)
        if not buf:
            break
        xmlbuf += buf
    return xmlbuf
    
def compare_xml(oldXmlHandler, newXmlHandler):
    if len(oldXmlHandler.elemList) != len(newXmlHandler.elemList):
        raise Exception, 'Different number of base elements.'
    for oe in oldXmlHandler.elemList:
        for ne in newXmlHandler.elemList:
            if oe._data.has_key('NAME') and ne._data.has_key('NAME'):
                if oe['NAME'] == ne['NAME']:
                    checkEquivalentXml(oe, ne)
                    break
            else:
                checkEquivalentXml(oe, ne)
    
def run_xml_consistency_test(old, new):
    print 'Running XML Consistency Test.'
    print 'Old host is %s' % old
    print 'New host is %s' % new
    
    sock = get_socket(old)
    xmlbuf = get_xml_from_socket(sock)
    oldXmlHandler = GmetadXmlHandler()
    xml.sax.parseString(xmlbuf, oldXmlHandler)
    
    sock = get_socket(new)
    xmlbuf = get_xml_from_socket(sock)    
    newXmlHandler = GmetadXmlHandler()
    xml.sax.parseString(xmlbuf, newXmlHandler)
    
    compare_xml(oldXmlHandler, newXmlHandler)

def run_interactive_consistency_test(old, new):
    filters = ['\n', '/\n', '/Grid-Node\n', '/Grid-Node/localhost\n', '/Grid-Node/localhost/mem_free\n']
    for filter in filters:
        print 'Running interactive consistency test with filter "%s"' % filter.strip()
        print 'Old host is %s' % old
        print 'New host is %s' % new
        
        sock = get_socket(old)
        sock.send(filter)
        xmlbuf = get_xml_from_socket(sock)
        oldXmlHandler = GmetadXmlHandler()
        xml.sax.parseString(xmlbuf, oldXmlHandler)
        
        sock = get_socket(new)
        sock.send(filter)
        xmlbuf = get_xml_from_socket(sock)
        newXmlHandler = GmetadXmlHandler()
        xml.sax.parseString(xmlbuf, newXmlHandler)
        
        compare_xml(oldXmlHandler, newXmlHandler)
    

if __name__ == '__main__':
    p = optparse.OptionParser()
    p.add_option('-I', '--old_gmetad_interactive',  action='store',
            help='Location of old gmetad interactive port (default="localhost:8652")',
            default='localhost:8652')
    p.add_option('-i', '--new_gmetad_interactive', action='store',
            help='Location of new gmetad interactive port (default="localhost:8652")',
            default='localhost:8652')
    p.add_option('-X', '--old_gmetad_xml', action='store',
            help='Location of old gmetad xml port (default="localhost:8651")',
            default='localhost:8651')
    p.add_option('-x', '--new_gmetad_xml', action='store',
            help='Location of new gmetad xml port (default="localhost:8651")',
            default='localhost:8651')
    p.add_option('-s', '--server_path', action='store',
            help='Path to new gmetad script.  If not provided, it will not be started.',
            default=None)
    p.add_option('-c', '--conf', action='store',
            help='Path to gmetad configuration file (default="/etc/ganglia/gmetad.conf")',
            default='/etc/ganglia/gmetad.conf')
    p.add_option('-l', '--logfile', action='store',
            help='Path to gmetad log file, overrides configuration',
            default=None)
            
    options, args = p.parse_args()
    
    do_interactive_test=False
    do_xml_test=False
    if options.old_gmetad_interactive == options.new_gmetad_interactive:
        print 'Locations for old and new gmetad interative ports are the same.'
        print 'Skipping the interactive port consistency test.'
    else:
        do_interactive_test=True
    if options.old_gmetad_xml == options.new_gmetad_xml:
        print 'Locations for old and new gmetad xml ports are the same.'
        print 'Skipping the xml port consistency test.'
    else:
        do_xml_test=True
        
    if not do_interactive_test and not do_xml_test:
        sys.exit()
        
    gmetad_pidfile = None
    if options.server_path is not None:
        gmetad_pidfile = '/tmp/gmetad.pid'
        cmd = 'python %s -c %s -i %s -x %s -p %s' % (options.server_path,
                options.conf,
                options.new_gmetad_interactive.split(':')[1],
                options.new_gmetad_xml.split(':')[1],
                gmetad_pidfile)
        if options.logfile is not None:
            cmd += ' -l %s' % options.logfile
        os.system(cmd)
        time.sleep(1) # wait for it to come up
    
    try:
        if do_xml_test:
            run_xml_consistency_test(options.old_gmetad_xml, options.new_gmetad_xml)
        if do_interactive_test:
            run_interactive_consistency_test(options.old_gmetad_interactive, options.new_gmetad_interactive)
    finally:
        if gmetad_pidfile is not None:
            f = open(gmetad_pidfile, 'r')
            line = f.readline()
            pid = line.strip()
            os.kill(int(pid), signal.SIGTERM)
    print 'All tests passed.'

