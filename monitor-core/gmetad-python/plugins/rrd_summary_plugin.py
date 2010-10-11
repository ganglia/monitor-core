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
from Gmetad.gmetad_config import getConfig, GmetadConfig
from Gmetad.gmetad_data import DataStore

def get_plugin():
    ''' All plugins are required to implement this method.  It is used as the factory
        function that instanciates a new plugin instance. '''
    # The plugin configuration ID that is passed in must match the section name 
    #  in the configuration file.
    return RRDSummaryPlugin('rrdsummary')

def getRandomInterval(midpoint, range=5):
    return randrange(max(midpoint-range,0), midpoint+range)

class RRDSummaryPlugin(RRDPlugin):
    ''' This class implements the RRD plugin that stores metric summary data to RRD files.
        It is derived from the RRDPlugin class.'''

    def __init__(self, cfgid):
        RRDPlugin.__init__(self, 'RRD')

    def start(self):
        '''Called by the engine during initialization to get the plugin going.'''
        # Start the RRD root summary thread that will periodically update the summary RRDs
        self.rootSummary = RRDRootSummary()
        self.rootSummary.start()
        #print "RRDSummary start called"
    
    def stop(self):
        '''Called by the engine during shutdown to allow the plugin to shutdown.'''
        # Shut down the RRD root summary thread
        self.rootSummary.shutdown()
        #print "RRDSummary stop called"

    def notify(self, clusterNode):
        '''Called by the engine when the internal data structure has changed.'''
        gmetadConfig = getConfig()
        try:
            if clusterNode.getAttr('status') == 'down':
                return
        except AttributeError:
            pass
        # Find the data source configuration entry that matches the cluster name
        for ds in gmetadConfig[GmetadConfig.DATA_SOURCE]:
            if ds.name == clusterNode.getAttr('name'):
                break
        if ds is None:
            logging.info('No matching data source for %s'%clusterNode.getAttr('name'))
            return

        if 'CLUSTER' == clusterNode.id:
            # Create the summary RRD base path and validate it
            clusterPath = '%s/%s'%(self.cfg[RRDPlugin.RRD_ROOTDIR], clusterNode.getAttr('name'))
            self._checkDir(clusterPath)
            clusterPath = '%s/__SummaryInfo__'%clusterPath
            self._checkDir(clusterPath)
            # Update metrics for each cluster
            for metricNode in clusterNode.summaryData['summary'].itervalues():
                # Create the summary RRD final path and validate it
                rrdPath = '%s/%s.rrd'%(clusterPath,metricNode.getAttr('name'))
                # Create the RRD metric summary file if it doesn't exist
                if not os.path.isfile(rrdPath):
                    self._createRRD(clusterNode, metricNode, rrdPath, ds.interval, True)
                    #need to do some error checking here if the createRRD failed
                # Update the RRD file.
                try:
                    self._updateRRD(clusterNode, metricNode, rrdPath, True)
                except Exception:
                    pass

        if 'GRID' == clusterNode.id:
            try:
                # Create the summary RRD base path and validate it
                gridPath = '%s/%s'%(self.cfg[RRDPlugin.RRD_ROOTDIR], clusterNode.getAttr('name'))
                self._checkDir(gridPath)
                gridPath = '%s/__SummaryInfo__'%gridPath
                # Update metrics for the grid
                # If there isn't any summary data, then no need to continue.
                if not hasattr(clusterNode, 'summaryData'): 
                    return
            
                # Update metrics RRDs for grid summary
                for metricNode in clusterNode.summaryData['summary'].itervalues():
                    # Create the summary RRD final path and validate it.
                    rrdPath = '%s/%s.rrd'%(gridPath,metricNode.getAttr('name'))
                    # if the RRD file doesn't exist then create it
                    if not os.path.isfile(rrdPath):
                        self._createRRD(clusterNode, metricNode, rrdPath, 15, True)
                        #need to do some error checking here if the createRRD failed
                    # Update the RRD file.
                    self._updateRRD(clusterNode, metricNode, rrdPath, True)
            except Exception, e:
                logging.error('Error writing to summary RRD %s'%str(e))

        #print "RRDSummary notify called"


class RRDRootSummary(threading.Thread, RRDPlugin):
    ''' This class writes the root summaries for all of the clusters in the grid. It is a thread class that is
        also derived from the RRDPlugin class. '''
    
    def __init__(self):
        threading.Thread.__init__(self)
        # Call the base class init so that we have access to its configuration directives.
        RRDPlugin.__init__(self, 'RRD')

        self._cond = threading.Condition()
        self._running = False
        self._shuttingDown = False

    def writeRootSummary(self):
        ''' This method updates the RRD summary files.'''
        ds = DataStore()
        rootNode = ds.rootElement
        # If there isn't a root node then there is not need to continue.
        if rootNode is None: return
        # Get a lock on the data store.
        ds.acquireLock(self)
        try:
            gmetadConfig = getConfig()
            # Create the summary RRD base path and validate it
            rootPath = '%s/__SummaryInfo__'%self.cfg[RRDPlugin.RRD_ROOTDIR]
            self._checkDir(rootPath)
            # Update metrics for each grid node (there should only be one.)
            for gridNode in rootNode:
                # If there isn't any summary data, then no need to continue.
                if not hasattr(gridNode, 'summaryData'): 
                    continue
            
                # Update metrics RRDs for each cluster summary in the grid
                for metricNode in gridNode.summaryData['summary'].itervalues():
                    # Create the summary RRD final path and validate it.
                    rrdPath = '%s/%s.rrd'%(rootPath,metricNode.getAttr('name'))
                    # if the RRD file doesn't exist then create it
                    if not os.path.isfile(rrdPath):
                        self._createRRD(rootNode, metricNode, rrdPath, 15, True)
                        #need to do some error checking here if the createRRD failed
                    # Update the RRD file.
                    self._updateRRD(rootNode, metricNode, rrdPath, True)
        except Exception, e:
            logging.error('Error writing to summary RRD %s'%str(e))
        ds.releaseLock(self)
        #print "RRDRootSummary called"

    def run(self):
        if self._running:
            return
        self._running = True
        while not self._shuttingDown:
            self._cond.acquire()
            # wait a random time between 10 and 30 seconds
            self._cond.wait(getRandomInterval(20, 10))
            self._cond.release()        
            # If we aren't shutting down then do the grid summary
            if not self._shuttingDown:
                self.writeRootSummary()

    def shutdown(self):
        self._shuttingDown = True
        self._cond.acquire()
        self._cond.notifyAll()
        self._cond.release()
        self.join()
        
