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

import os
import rrdtool
import logging
import threading
from random import randrange

from rrd_plugin import RRDPlugin
from gmetad_config import getConfig, GmetadConfig
from gmetad_data import DataStore

def get_plugin():
    return RRDSummaryPlugin('rrdsummary')

def getRandomInterval(midpoint, range=5):
    return randrange(max(midpoint-range,0), midpoint+range)

class RRDSummaryPlugin(RRDPlugin):

    def __init__(self, cfgid):
        RRDPlugin.__init__(self, 'RRD')

    def start(self):
        '''Called by the engine during initialization to get the plugin going.'''
        self.rootSummary = RRDRootSummary()
        self.rootSummary.start()
        print "RRDSummary start called"
    
    def stop(self):
        '''Called by the engine during shutdown to allow the plugin to shutdown.'''
        self.rootSummary.shutdown()
        print "RRDSummary stop called"

    def notify(self, clusterNode):
        '''Called by the engine when the internal data structure has changed.'''
        gmetadConfig = getConfig()
        for ds in gmetadConfig[GmetadConfig.DATA_SOURCE]:
            if ds.name == clusterNode.name:
                break
        if ds is None:
            logging.info('No matching data source for %s'%clusterNode.name)
            return
        clusterPath = '%s/%s'%(self.cfg[RRDPlugin.RRD_ROOTDIR], clusterNode.name)
        self._checkDir(clusterPath)
        clusterPath = '%s/__SummaryInfo__'%clusterPath
        self._checkDir(clusterPath)
        for metricNode in clusterNode.summaryData['summary'].itervalues():
            rrdPath = '%s/%s.rrd'%(clusterPath,metricNode.name)
            if not os.path.isfile(rrdPath):
                self._createRRD(clusterNode, metricNode, rrdPath, ds.interval, True)
                #need to do some error checking here if the createRRD failed
            self._updateRRD(clusterNode, metricNode, rrdPath, True)
        print "RRDSummary notify called"


class RRDRootSummary(threading.Thread, RRDPlugin):
    def __init__(self):
        threading.Thread.__init__(self)
        RRDPlugin.__init__(self, 'RRD')

        self._cond = threading.Condition()
        self._running = False
        self._shuttingDown = False

    def writeRootSummary(self):
        ds = DataStore()
        rootNode = ds.rootElement
        if rootNode is None: return
        ds.acquireLock(self)
        try:
            gmetadConfig = getConfig()
            rootPath = '%s/__SummaryInfo__'%self.cfg[RRDPlugin.RRD_ROOTDIR]
            self._checkDir(rootPath)
            for gridNode in rootNode:
                if not hasattr(gridNode, 'summaryData'): 
                    continue
            
                for metricNode in gridNode.summaryData['summary'].itervalues():
                    rrdPath = '%s/%s.rrd'%(rootPath,metricNode.name)
                    if not os.path.isfile(rrdPath):
                        self._createRRD(rootNode, metricNode, rrdPath, 15, True)
                        #need to do some error checking here if the createRRD failed
                    self._updateRRD(rootNode, metricNode, rrdPath, True)
        except Exception, e:
            print e
        ds.releaseLock(self)
        print "RRDRootSummary called"

    def run(self):
        if self._running:
            return
        self._running = True
        while not self._shuttingDown:
            self._cond.acquire()
            # wait a random time between 10 and 30 seconds
            self._cond.wait(getRandomInterval(20, 10))
            self._cond.release()        
            if not self._shuttingDown:
                self.writeRootSummary()

    def shutdown(self):
        self._shuttingDown = True
        self._cond.acquire()
        self._cond.notifyAll()
        self._cond.release()
        self.join()
        
