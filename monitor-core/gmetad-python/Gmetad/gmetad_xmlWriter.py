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
        self.gridDepth = -1
        
    def _getNumHostsForCluster(self, clusternode):
        #Returns a tuple of the form (hosts_up, hosts_down).
        hosts_up = 0
        hosts_down = 0
        # If there is summary data, then pull the host status counts.
        summaryData = clusternode.getSummaryData()
        if summaryData is not None:
            hosts_up = clusternode.summaryData['hosts_up']
            hosts_down = clusternode.summaryData['hosts_down']
        return (hosts_up, hosts_down)
        
    def _getGridSummary(self, gridnode, filterList, queryargs):
        ''' This method will generate a grid XML dump for the given grid node.'''
        cbuf = ''
        # Pull the host status summaries from the grid node.
        hosts = self._getNumHostsForCluster(gridnode)
        # If we have summary data, then interate through all of the metric nodes and generate XML for each.
        summaryData = gridnode.getSummaryData()
        if summaryData is not None:
            for m in summaryData['summary'].itervalues():
                cbuf += self._getXmlImpl(m, filterList, queryargs)
        # Format the XML based on all of the results.
        rbuf = '<HOSTS UP="%d" DOWN="%d" SOURCE="gmetad" />\n%s' % (hosts[0], hosts[1], cbuf)
        if self.gridDepth == 0:
            # Generate the XML for each cluster/grid node.
            for c in gridnode.children.values():
                if 'CLUSTER' == c.id or 'GRID' == c.id:
                    rbuf += self._getXmlImpl(c, filterList, queryargs)
        return rbuf
        
    def _getClusterSummary(self, clusternode, filterList, queryargs):
        ''' This method will generate a cluster XML dump for the given cluster node.'''
        cbuf = ''
        # Pull the host status summaries from the cluster node.
        hosts = self._getNumHostsForCluster(clusternode)
        # If we have summary data, then interate through all of the metric nodes and generate XML for each.
        summaryData = clusternode.getSummaryData()
        if summaryData is not None:
            for m in summaryData['summary'].itervalues():
                cbuf += self._getXmlImpl(m, filterList, queryargs)
        # Format the XML based on all of the results.
        rbuf = '<HOSTS UP="%d" DOWN="%d" SOURCE="gmetad" />\n%s' % (hosts[0], hosts[1], cbuf)
        return rbuf
        
    def _getXmlImpl(self, element, filterList=None, queryargs=None):
        ''' This method can be called recursively to traverse the data store and produce XML
            for specific nodes. It also respects the filter and query args when generating the XML.'''
        skipTag = None
        rbuf = ''
        if element.id in ['CLUSTER', 'HOST', 'EXTRA_DATA', 'EXTRA_ELEMENT'] and self.gridDepth > 0:
            skipTag = True
        # If this is a grid tag, then get the local time since a time stamp was never provided by gmond.
        if 'GRID' == element.id:
            element.setAttr('localtime', int(time.time()))
            self.gridDepth += 1
            logging.info('Found <GRID> depth is now: %d' %self.gridDepth)
        if not skipTag:
            # Add the XML tag
            rbuf = '<%s' % element.tag
            # Add each attribute that is contained in the.  By pass some specific attributes.
            for k,v in element.getAttrs().items():
                rbuf += ' %s="%s"' % (k.upper(), v)
        if queryargs is not None or ('GRID' == element.id and self.gridDepth > 0):
            if (('GRID' == element.id or 'CLUSTER' == element.id) and (filterList is None or not len(filterList))) or ('GRID' == element.id and self.gridDepth > 0):
                try:
                    # If the filter specifies that this is a summary rather than a regular XML dump, generate the 
                    #  summary XML.
                    if (queryargs is not None and queryargs['filter'].lower().strip() == 'summary') or ('GRID' == element.id and self.gridDepth > 0):
                        # A summary XML dump will contain a grid summary as well as each cluster summary.  Each will
                        #  be added during recusive calls to this method.
                        if 'GRID' == element.id:
                            rbuf += '>\n%s</GRID>\n' % self._getGridSummary(element, filterList, queryargs)
                            self.gridDepth -= 1
                            logging.info('Found </GRID> depth is now %d' %self.gridDepth)
                            return rbuf
                        elif 'CLUSTER' == element.id:
                            if not skipTag:
                                rbuf += '>\n%s</CLUSTER>\n' % self._getClusterSummary(element, filterList, queryargs)
                            else:
                                rbuf += '%s' % self._getClusterSummary(element, filterList, queryargs)
                            return rbuf
                except ValueError:
                    pass
        # If there aren't any children, then no reason to continue.
        if 0 < len(element.children):
            if not skipTag:
                # Close the last tag
                rbuf += '>\n'
            showAllChildren = True
            # If there was a specific filter specified, then only include the appropriate children.  Otherwise
            #  show all of the children.
            if filterList is not None and len(filterList):
                try:
                    # Get the key and call this method recusively for the child
                    key = Element.generateKey([self._pcid_map[element.id], filterList[0]])
                    rbuf += self._getXmlImpl(element.children[key], filterList[1:], queryargs)
                    showAllChildren = False
                except KeyError:
                    pass
            if showAllChildren:
                # For each child, call this method recusively.  This will produce a complete dump of all children
                for c in element.children.values():
                    rbuf += self._getXmlImpl(c, filterList, queryargs)
            if 'GRID' == element.tag:
                self.gridDepth -= 1
                logging.info('Found </GRID> depth is now: %d' %self.gridDepth)
            if not skipTag:
                rbuf += '</%s>\n' % element.tag
        else:
            if not skipTag:
                rbuf += ' />\n'
        return rbuf
            
    def getXml(self, filter=None, queryargs=None):
        ''' This method generates the output XML for either the entire data store or 
            specified portions based on the filter and query args.'''
        if filter is None:
            filterList = None
        elif not len(filter.strip()):
            filterList = None
        else:
            filterList = filter.split('/')
        rbuf = '%s\n%s\n' % (self._xml_starttag, self._xml_dtd)
        ds = DataStore()
        if ds.rootElement is not None:
            ds.acquireLock(self)
            rbuf += self._getXmlImpl(ds.rootElement, filterList, queryargs)
            ds.releaseLock(self)
        return rbuf
