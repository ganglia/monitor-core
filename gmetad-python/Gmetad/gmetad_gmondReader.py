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

import threading
import xml.sax
import socket
import time
import logging
import zlib

from gmetad_config import GmetadConfig, getConfig
from gmetad_random import getRandomInterval
from gmetad_data import DataStore
from gmetad_data import Element

class GmondContentHandler(xml.sax.ContentHandler):
    ''' This class implements the XML parser used to parse XML data from a gmond cluster. '''
    def __init__(self):
        # Initialize the parser and the class object.
        xml.sax.ContentHandler.__init__(self)
        self._elemStack = []
        self._elemStackLen = 0
        self._ancestry = []
        
    def startElement(self, tag, attrs):
        ''' This methods creates an element based on XML tags and inserts it into the data store. '''
        ds = DataStore()
        # Create a new node based on the XML attributes.
        e = Element(tag, attrs)
        # If this is the head tag for the XML dump, initialize the data store 
        if 'GANGLIA_XML' == tag:
            ds.acquireLock(self)
            self._elemStack.append(ds.getNode()) # Fetch the root node.  It has already been set into the tree.
            self._elemStackLen += 1
            cfg = getConfig()
            # We'll go ahead and update any existing GRID tag with a new one (new time) even if one already exists.
            e = Element('GRID', {'NAME':cfg[GmetadConfig.GRIDNAME], 'AUTHORITY':cfg[GmetadConfig.AUTHORITY], 'LOCALTIME':'%d' % time.time()})
            self._ancestry.append('GANGLIA_XML')
        # Insert the new node into the data store at the appropriate location
        self._elemStack.append(ds.setNode(e, self._elemStack[self._elemStackLen-1]))
        # If this is a cluster or nested grid node, then keep track of the data store path to this node.
        if (len(self._ancestry) < 2 or (len(self._ancestry) == 2 and e.id in ['GRID', 'CLUSTER'])):
            self._ancestry.append('%s:%s'%(e.id,e.getAttr('name')))
        self._elemStackLen += 1
        
    def endElement(self, tag):
        # Release the data store lock of we hit the end of the XML dump
        if tag == 'GANGLIA_XML':
            DataStore().releaseLock(self)
        self._elemStack.pop()
        self._elemStackLen -= 1
        
    def getClusterAncestry(self):
        return self._ancestry

class GmondReader(threading.Thread):
    ''' This class implements a cluster reader thread that will periodically ping the cluster
        for all of it's data. '''
    def __init__(self,dataSource,name=None,target=None,args=(),kwargs={}):
        # Intialize the thread
        threading.Thread.__init__(self,name,target,args,kwargs)
        self._cond = threading.Condition()
        self._shuttingDown = False
        self.dataSource = dataSource
        self.lastKnownGoodHost = 0
        logging.debug('Reader created for cluster %s' % self.dataSource.name)
        
    def _getEndpoint(self, hostspec, port=8649):
        hostinfo = hostspec.split(':')
        if len(hostinfo) > 1:
            port = int(hostinfo[1])
        return (hostinfo[0], port)
        
    def run(self):
        ds = DataStore()
        while not self._shuttingDown:
            connected = False
            # Create a socket and connect to the cluster data source.
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            try:
                sock.connect( self._getEndpoint(self.dataSource.hosts[self.lastKnownGoodHost]) )
                connected = True
            except socket.error:
                # Keep track of the last good data source within the cluster. If we can't reconnect to the
                #  same data source, try the next one in the list.
                curidx = self.lastKnownGoodHost
                while True:
                    curidx += 1
                    if curidx >= len(self.dataSource.hosts):
                        curidx = 0
                    if curidx == self.lastKnownGoodHost: break
                    try:
                        sock.connect( self._getEndpoint(self.dataSource.hosts[curidx]) )
                        self.lastKnownGoodHost = curidx
                        connected=True
                        break
                    except socket.error:
                        pass
            if connected:
                logging.info('Querying data source %s via host %s' % (self.dataSource.name, self.dataSource.hosts[self.lastKnownGoodHost]))
                xmlbuf = ''
                while True:
                    # Read all of the XML data from the data source.
                    buf = sock.recv(8192)
                    if not buf:
                        break
                    xmlbuf += buf
                sock.close()

                # These are the gzip header magic numbers, per RFC 1952 section 2.3.1
                if xmlbuf[0:2] == '\x1f\x8b':
                    # 32 is a magic number in zlib.h for autodetecting the zlib or gzip header
                    xmlbuf = zlib.decompress(xmlbuf, zlib.MAX_WBITS + 32)

                # Create an XML parser and parse the buffer
                gch = GmondContentHandler()
                xml.sax.parseString(xmlbuf, gch)
                # Notify the data store that all updates for the cluster are finished.
                clusterNode = ds.getNode(gch.getClusterAncestry())
                if clusterNode is not None:
                    clusterNode.setAttr('status', 'up')
            else:
                logging.error('Could not connect to any host for data source %s' % self.dataSource.name)
                ds = DataStore()
                cfg = getConfig()
                gridKey = Element.generateKey(['GRID',cfg[GmetadConfig.GRIDNAME]])
                clusterKey = Element.generateKey(['CLUSTER', self.dataSource.name])
                gridNode = ds.getNode([str(ds.rootElement), gridKey])
                clusterNode = None
                if gridNode is not None and str(gridNode) == gridKey:
                    try:
                        clusterNode = gridNode[clusterKey]
                    except KeyError:
                        clusterNode = Element('CLUSTER', {'NAME':self.dataSource.name,  'LOCALTIME':'%d' % time.time()})
                        ds.setNode(clusterNode, gridNode)
                if clusterNode is not None:
                    clusterNode.setAttr('status', 'down')
                    #clusterNode.localtime = time.time()
                    
            ds.updateFinished(clusterNode)
                    
            if self._shuttingDown:
                break
            # Go to sleep for a while.
            self._cond.acquire()
            self._cond.wait(getRandomInterval(self.dataSource.interval))
            self._cond.release()        
            
    def shutdown(self):
        # Release all locks and shut down the thread.
        self._shuttingDown = True
        self._cond.acquire()
        self._cond.notifyAll()
        self._cond.release()
        self.join()
