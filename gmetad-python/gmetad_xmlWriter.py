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

import thread
import time
import logging

from gmetad_config import GmetadConfig
from gmetad_data import DataStore, Element

class XmlWriter:
    _xml_starttag = '<?xml version="1.0" encoding="ISO-8859-1" standalone="yes"?>'
    _xml_dtd = '''<!DOCTYPE GANGLIA_XML [
   <!ELEMENT GANGLIA_XML (GRID|CLUSTER|HOST)*>
      <!ATTLIST GANGLIA_XML VERSION CDATA #REQUIRED>
      <!ATTLIST GANGLIA_XML SOURCE CDATA #REQUIRED>
   <!ELEMENT GRID (CLUSTER | GRID | HOSTS | METRICS)*>
      <!ATTLIST GRID NAME CDATA #REQUIRED>
      <!ATTLIST GRID AUTHORITY CDATA #REQUIRED>
      <!ATTLIST GRID LOCALTIME CDATA #IMPLIED>
   <!ELEMENT CLUSTER (HOST | HOSTS | METRICS)*>
      <!ATTLIST CLUSTER NAME CDATA #REQUIRED>
      <!ATTLIST CLUSTER OWNER CDATA #IMPLIED>
      <!ATTLIST CLUSTER LATLONG CDATA #IMPLIED>
      <!ATTLIST CLUSTER URL CDATA #IMPLIED>
      <!ATTLIST CLUSTER LOCALTIME CDATA #REQUIRED>
   <!ELEMENT HOST (METRIC)*>
      <!ATTLIST HOST NAME CDATA #REQUIRED>
      <!ATTLIST HOST IP CDATA #REQUIRED>
      <!ATTLIST HOST LOCATION CDATA #IMPLIED>
      <!ATTLIST HOST REPORTED CDATA #REQUIRED>
      <!ATTLIST HOST TN CDATA #IMPLIED>
      <!ATTLIST HOST TMAX CDATA #IMPLIED>
      <!ATTLIST HOST DMAX CDATA #IMPLIED>
      <!ATTLIST HOST GMOND_STARTED CDATA #IMPLIED>
   <!ELEMENT METRIC (EXTRA_DATA*)>
      <!ATTLIST METRIC NAME CDATA #REQUIRED>
      <!ATTLIST METRIC VAL CDATA #REQUIRED>
      <!ATTLIST METRIC TYPE (string | int8 | uint8 | int16 | uint16 | int32 | uint32 | float | double | timestamp) #REQUIRED>
      <!ATTLIST METRIC UNITS CDATA #IMPLIED>
      <!ATTLIST METRIC TN CDATA #IMPLIED>
      <!ATTLIST METRIC TMAX CDATA #IMPLIED>
      <!ATTLIST METRIC DMAX CDATA #IMPLIED>
      <!ATTLIST METRIC SLOPE (zero | positive | negative | both | unspecified) #IMPLIED>
      <!ATTLIST METRIC SOURCE (gmond) 'gmond'>
   <!ELEMENT EXTRA_DATA (EXTRA_ELEMENT*)>
   <!ELEMENT EXTRA_ELEMENT EMPTY>
      <!ATTLIST EXTRA_ELEMENT NAME CDATA #REQUIRED>
      <!ATTLIST EXTRA_ELEMENT VAL CDATA #REQUIRED>
   <!ELEMENT HOSTS EMPTY>
      <!ATTLIST HOSTS UP CDATA #REQUIRED>
      <!ATTLIST HOSTS DOWN CDATA #REQUIRED>
      <!ATTLIST HOSTS SOURCE (gmond | gmetad) #REQUIRED>
   <!ELEMENT METRICS (EXTRA_DATA*)>
      <!ATTLIST METRICS NAME CDATA #REQUIRED>
      <!ATTLIST METRICS SUM CDATA #REQUIRED>
      <!ATTLIST METRICS NUM CDATA #REQUIRED>
      <!ATTLIST METRICS TYPE (string | int8 | uint8 | int16 | uint16 | int32 | uint32 | float | double | timestamp) #REQUIRED>
      <!ATTLIST METRICS UNITS CDATA #IMPLIED>
      <!ATTLIST METRICS SLOPE (zero | positive | negative | both | unspecified) #IMPLIED>
      <!ATTLIST METRICS SOURCE (gmond) 'gmond'>
]>'''
    _pcid_map = {'GANGLIA_XML' : 'GRID',
            'GRID' : 'CLUSTER',
            'CLUSTER' : 'HOST',
            'HOST' : 'METRIC'
    }
    
    def __init__(self):
        cfg = GmetadConfig()
        self.gridname = cfg[GmetadConfig.GRIDNAME]
        self.authority = cfg[GmetadConfig.AUTHORITY]
        self.localtime = time.time()
        
    def _getNumHostsForCluster(self, clusternode):
        #Returns a tuple of the form (hosts_up, hosts_down).
        hosts_up = 0
        hosts_down = 0
        for c in clusternode.children.values():
            if 'HOST' == c.id:
                try:
                    if int(c.tn) < int(c.tmax)*4:
                        hosts_up += 1
                    else:
                        hosts_down += 1
                except AttributeError:
                    pass
        return (hosts_up, hosts_down)
        
    def _getGridSummary(self, gridnode, filterList, queryargs):
        totalHostsUp = 0
        totalHostsDown = 0
        cbuf = ''
        for c in gridnode.children.values():
            if 'CLUSTER' == c.id:
                hosts = self._getNumHostsForCluster(c)
                totalHostsUp += hosts[0]
                totalHostsDown += hosts[1]
                cbuf += self._getXmlImpl(c, filterList, queryargs)
        rbuf = '<HOSTS UP="%d" DOWN="%d" SOURCE="gmetad" />%s\n' % (totalHostsUp, totalHostsDown, cbuf)
        return rbuf
        
    def _getClusterSummary(self, clusternode):
        rbuf = '<HOSTS UP="%d" DOWN="%d" SOURCE="gmetad" />\n' % self._getNumHostsForCluster(clusternode)
        metrics = {}
        for h in clusternode.children.values():
            if 'HOST' == h.id:
                for m in h.children.values():
                    if 'METRIC' == m.id and 'zero' != m.slope:
                        if not metrics.has_key(m.name):
                            metrics[m.name] = {'SUM':0.0, 'NUM':1, 'TYPE':m.type, 'UNITS':'double', 'SLOPE':m.slope, 'SOURCE':m.source}
                        else:
                            metrics[m.name]['NUM'] += 1
                            metrics[m.name]['SUM'] += float(m.val)
        for mn, md in metrics.items():
            rbuf += '<METRICS NAME="%s"' % mn
            for k, v in md.items():
                rbuf += ' %s="%s"' % (k, v)
            rbuf += ' />\n'
        return rbuf
        
    def _getXmlImpl(self, element, filterList=None, queryargs=None):
        rbuf = '<%s' % element.id
        if 'GRID' == element.id:
            element.localtime = '%d' % time.time()
        foundName = False
        try:
            rbuf += ' NAME="%s"' % element.name
            foundName = True
        except AttributeError:
            pass
        for k,v in element.__dict__.items():
            if k == 'id' or k == 'children' or (foundName and k == 'name'):
                continue
            rbuf += ' %s="%s"' % (k.upper(), v)
        if queryargs is not None:
            if ('GRID' == element.id or 'CLUSTER' == element.id) and (filterList is None or not len(filterList)):
                try:
                    if queryargs['filter'].lower().strip() == 'summary':
                        if 'GRID' == element.id:
                            rbuf += '>\n%s</GRID>\n' % self._getGridSummary(element, filterList, queryargs)
                            return rbuf
                        elif 'CLUSTER' == element.id:
                            rbuf += '>\n%s</CLUSTER>\n' % self._getClusterSummary(element)
                            return rbuf
                except ValueError:
                    pass
        if 0 < len(element.children):
            rbuf += '>\n'
            showAllChildren = True
            if filterList is not None and len(filterList):
                try:
                    key = Element.generateKey([self._pcid_map[element.id], filterList[0]])
                    rbuf += self._getXmlImpl(element.children[key], filterList[1:], queryargs)
                    showAllChildren = False
                except KeyError:
                    pass
            if showAllChildren:
                for c in element.children.values():
                    rbuf += self._getXmlImpl(c, filterList, queryargs)
            rbuf += '</%s>\n' % element.id
        else:
            rbuf += ' />\n'
        return rbuf
            
    def getXml(self, filter=None, queryargs=None):
        if filter is None:
            filterList = None
        elif not len(filter.strip()):
            filterList = None
        else:
            filterList = filter.split('/')
        rbuf = '%s\n%s\n' % (self._xml_starttag, self._xml_dtd)
        ds = DataStore()
        if ds.rootElement is not None:
            ds.lock.acquire()
            rbuf += self._getXmlImpl(ds.rootElement, filterList, queryargs)
            ds.lock.release()
        return rbuf
