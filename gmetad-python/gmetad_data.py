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
import logging
import time
import copy

from gmetad_element import Element
from gmetad_config import getConfig, GmetadConfig
from gmetad_notifier import GmetadNotifier

class DataStore:
    _shared_state = {}
    _initialized = False
    lock = thread.allocate_lock()
    
    def __init__(self):
        self.__dict__ = DataStore._shared_state
        if not DataStore._initialized:
            self.lock = thread.allocate_lock()
            self.lock.acquire()
            self.rootElement = None
            # Initialize the data store with the GANGLIA_XML and GRID tags.
            # Data store should never be completely empty.  Even if there are
            # no reporting data sources the web front end depends on having
            # at least a GANGLIA_XML tag and a nested GRID tag.
            cfg = getConfig()
            self.setNode(Element('GANGLIA_XML', {'VERSION':cfg.VERSION}))
            self.setNode(Element('GRID', {'NAME':cfg[GmetadConfig.GRIDNAME], 'AUTHORITY':cfg[GmetadConfig.AUTHORITY], 'LOCALTIME':'%d' % time.time()}), self.rootElement)
            self.lock.release()
            
            self.notifier = GmetadNotifier()
            self.notifier.start()
            DataStore._initialized = True
            
    def _doSummary(self, clusterNode):
        clusterNode.summary = {}
        for hostNode in clusterNode:
            for metricNode in hostNode:
                #if metricNode.type in ['string', 'timestamp'] or metricNode.slope == 'zero':
                if metricNode.type in ['string', 'timestamp']:
                    continue
                try:
                    summaryNode = clusterNode.summary[str(metricNode)]
                    summaryNode.val = float(summaryNode.val) + float(metricNode.val)
                except KeyError:
                    summaryNode = copy.copy(metricNode)
                    clusterNode.summary[str(summaryNode)] = summaryNode
                    summaryNode.num = 0
                summaryNode.num += 1
    
    def shutdown(self):
        self.notifier.shutdown()
        
    def setNode(self, node, parent=None):
        if parent is None:
            if self.rootElement is None:
                self.rootElement = node
                self.rootElement.source = 'gmetad'
            return self.rootElement
        parent[str(node)] = node
        return parent[str(node)]
        
    def getNode(self, ancestry=[]):
        if not len(ancestry):
            return self.rootElement
        node = None
        while ancestry:
            nodeId = ancestry.pop(0)
            if node is None:
                if nodeId == str(self.rootElement):
                    node = self.rootElement
                else: return None
            else:
                try:
                    node = node[nodeId]
                except KeyError:
                    pass
        return node

    def updateFinished(self, clusterPath=[]):
        clusterNode = self.getNode(clusterPath)
        self._doSummary(clusterNode);
        self.notifier.insertTransaction(clusterNode)
        
