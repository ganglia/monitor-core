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

import thread, threading
import logging
import time

from gmetad_element import Element
from gmetad_config import getConfig, GmetadConfig
from gmetad_notifier import GmetadNotifier
from gmetad_random import getRandomInterval

class DataStore:
    '''The Datastore object stores all of the metric data
       as well as summary data for all data sources. It is
       a singleton object which guarantees that all instances
       of the object will produce the same data store. '''
    _shared_state = {} #Storage for a singleton object
    _initialized = False
    lock = thread.allocate_lock()
    
    def __init__(self):
        # Replace the objects attributes with the original
        #  attributes and data.
        self.__dict__ = DataStore._shared_state

        # Make sure that the DataStore object is only initialized once.
        if not DataStore._initialized:
            # Allocate a lock that will be used by all threads
            #  that are reading or writing to the data store.
            self.lock = thread.allocate_lock()
            self.lock.acquire()
            self.rootElement = None
            # Initialize the data store with the GANGLIA_XML and GRID tags.
            # Data store should never be completely empty.  Even if there are
            # no reporting data sources the web front end depends on having
            # at least a GANGLIA_XML tag and a nested GRID tag.
            cfg = getConfig()
            self.setNode(Element('GANGLIA_XML', {'VERSION':cfg.VERSION, 'SOURCE':'gmetad'}))
            self.setNode(Element('GRID', {'NAME':cfg[GmetadConfig.GRIDNAME], 'AUTHORITY':cfg[GmetadConfig.AUTHORITY], 'LOCALTIME':'%d' % time.time()}), self.rootElement)
            self.lock.release()

            # Start up the grid summary thread. 
            self.gridSummary = DataStoreGridSummary()
            self.gridSummary.start()

            # Start up the plugin notifier.
            self.notifier = GmetadNotifier()
            self.notifier.start()
            DataStore._initialized = True

    def _doGridSummary(self, gridNode):
        '''This private function will calculate the summaries for a
            single grid.  It is called each time new data
            has been read for a specific grid.  All summary data
            is placed in the summaryData dictionary. '''
        self.acquireLock(self)

        # Clear out the summaryData from the last run and initialize
        #  the dictionary for new summaries.
        gridNode.summaryData = {}
        gridNode.summaryData['summary'] = {}
        gridNode.summaryData['hosts_up'] = 0
        gridNode.summaryData['hosts_down'] = 0
        gridUp = (gridNode.getAttr('status') == 'up')
        summaryTime = int(time.time())

        # Summarize over each host contained by the cluster
        for clusterNode in gridNode:
            # Assume that cluster is up if we get a clusterNode object
            clusterUp = True
            for hostNode in clusterNode:
                reportedTime = summaryTime
                # Sum up the status of all of the hosts
                if 'HOST' == hostNode.id:
                    # Calculate the difference between the last known reported time
                    #  and the current time.  This determines if the host is up or down
                    # ** There may still be some issues with the way that this calculation is done
                    # ** The metric node below may also have the same issues.
                    reportedTime = int(hostNode.getAttr('reported'))
                    tn = int(hostNode.getAttr('tn'))
                    # If the last reported time is the same as the current reported time, then
                    #  the host has not been updated.  Therefore calculate the time offset from
                    #  the current time.
                    if hostNode.lastReportedTime == reportedTime:
                        tn = summaryTime - reportedTime
                        if tn < 0: tn = 0
                        hostNode.setAttr('tn', str(tn))
                    else:
                        hostNode.lastReportedTime = reportedTime

                    try:
                        if clusterUp and (int(hostNode.getAttr('tn')) < int(hostNode.getAttr('tmax'))*4):
                            gridNode.summaryData['hosts_up'] += 1
                        else:
                            gridNode.summaryData['hosts_down'] += 1
                    except AttributeError:
                        pass
                    except KeyError:
                        pass
                # Summarize over each metric within a host
                for metricNode in hostNode:
                    tn = int(metricNode.getAttr('tn'))

                    # If the last reported time is the same as the current reported time, then
                    #  the host has not been updated.  Therefore calculate the time offset from
                    #  the current time.
                    if hostNode.lastReportedTime == reportedTime:
                        tn = summaryTime - reportedTime
                        if tn < 0: tn = 0
                        metricNode.setAttr('tn', str(tn))

                    # Don't include metrics that can not be summarized
                    if metricNode.getAttr('type') in ['string', 'timestamp']:
                        continue
                    try:
                        # Pull the existing summary node from the summary data
                        # dictionary. If one doesn't exist, add it in the exception.
                        summaryNode = gridNode.summaryData['summary'][str(metricNode)]
                        currSum = summaryNode.getAttr('sum')
                        summaryNode.incAttr('sum',  float(metricNode.getAttr('val')))
                    except KeyError:
                        # Since summary metrics use a different tag, create the new 
                        #  summary node with correct tag.
                        summaryNode = metricNode.summaryCopy(tag='METRICS')
                        # Initialize the first summary value and change the data type
                        # to double for all metric summaries
                        summaryNode.setAttr('sum', float(metricNode.getAttr('val')))
                        summaryNode.setAttr('type', 'double')
                        # Add the summary node to the summary dictionary
                        gridNode.summaryData['summary'][str(summaryNode)] = summaryNode
                    summaryNode.incAttr('num', 1)
        self.releaseLock(self)

    def _doClusterSummary(self, clusterNode):
        '''This private function will calculate the summaries for a
            single cluster.  It is called each time that new data
            has been read for a specific cluster.  All summary data
            is placed in the summaryData dictionary. '''
        self.acquireLock(self)

        # Clear out the summaryData from the last run and initialize
        #  the dictionary for new summaries.
        clusterNode.summaryData = {}
        clusterNode.summaryData['summary'] = {}
        clusterNode.summaryData['hosts_up'] = 0
        clusterNode.summaryData['hosts_down'] = 0
        clusterUp = (clusterNode.getAttr('status') == 'up')
        summaryTime = int(time.time())
        
        # Summarize over each host contained by the cluster
        for hostNode in clusterNode:
            reportedTime = summaryTime
            # Sum up the status of all of the hosts
            if 'HOST' == hostNode.id:
                # Calculate the difference between the last known reported time
                #  and the current time.  This determines if the host is up or down
                # ** There may still be some issues with the way that this calculation is done
                # ** The metric node below may also have the same issues.
                reportedTime = int(hostNode.getAttr('reported'))
                tn = int(hostNode.getAttr('tn'))
                # If the last reported time is the same as the current reported time, then
                #  the host has not been updated.  Therefore calculate the time offset from
                #  the current time.
                if hostNode.lastReportedTime == reportedTime:
                    tn = summaryTime - reportedTime
                    if tn < 0: tn = 0
                    hostNode.setAttr('tn', str(tn))
                else:
                    hostNode.lastReportedTime = reportedTime
                    
                try:
                    if clusterUp and (int(hostNode.getAttr('tn')) < int(hostNode.getAttr('tmax'))*4):
                        clusterNode.summaryData['hosts_up'] += 1
                    else:
                        clusterNode.summaryData['hosts_down'] += 1
                except AttributeError:
                    pass
                except KeyError:
                    pass
            # Summarize over each metric within a host
            for metricNode in hostNode:
                tn = int(metricNode.getAttr('tn'))
                    
                # If the last reported time is the same as the current reported time, then
                #  the host has not been updated.  Therefore calculate the time offset from
                #  the current time.
                if hostNode.lastReportedTime == reportedTime:
                    tn = summaryTime - reportedTime
                    if tn < 0: tn = 0
                    metricNode.setAttr('tn', str(tn))
                    
                # Don't include metrics that can not be summarized
                if metricNode.getAttr('type') in ['string', 'timestamp']:
                    continue
                try:
                    # Pull the existing summary node from the summary data
                    # dictionary. If one doesn't exist, add it in the exception.
                    summaryNode = clusterNode.summaryData['summary'][str(metricNode)]
                    currSum = summaryNode.getAttr('sum')
                    summaryNode.incAttr('sum',  float(metricNode.getAttr('val')))
                except KeyError:
                    # Since summary metrics use a different tag, create the new 
                    #  summary node with correct tag.
                    summaryNode = metricNode.summaryCopy(tag='METRICS')
                    # Initialize the first summary value and change the data type
                    # to double for all metric summaries
                    summaryNode.setAttr('sum', float(metricNode.getAttr('val')))
                    summaryNode.setAttr('type', 'double')
                    # Add the summary node to the summary dictionary
                    clusterNode.summaryData['summary'][str(summaryNode)] = summaryNode
                summaryNode.incAttr('num', 1)
        self.releaseLock(self)
    
    def shutdown(self):
        # Shut down the notifier and the grid summary threads
        self.notifier.shutdown()
        self.gridSummary.shutdown()
        
    def setNode(self, node, parent=None):
        ''' Add a new node to the data store in the appropriate
            position in the tree. '''
        if parent is None:
            # If there isn't a root node, the new node becomes the root
            if self.rootElement is None:
                self.rootElement = node
            return self.rootElement
        if str(node) in parent.children:
            try:
                node.children = parent[str(node)].children
            except AttributeError:
                pass
            try:
                node.summaryData = parent[str(node)].summaryData
            except AttributeError:
                pass
        # Add the new node as a child of the parent
        parent[str(node)] = node
        return parent[str(node)]
        
    def getNode(self, ancestry=[]):
        ''' Find a node in the data store based on a node path '''
        # If no path was given, just return the root node.
        if not len(ancestry):
            return self.rootElement
        node = None
        # Follow the path given in the ancestry list until the 
        #  correct node is found.
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

    def updateFinished(self, clusterNode):
        ''' This method is called when the gmond reader has finished updating
            a cluster.  It indicates that a summary can be done over the
            entire cluster and then the cluster transaction needs to be
            entered and passed to the plugins. '''
        if clusterNode is not None:
            if 'CLUSTER' == clusterNode.id:
                self._doClusterSummary(clusterNode);
            if 'GRID' == clusterNode.id:
                self._doGridSummary(clusterNode)
            self.notifier.insertTransaction(clusterNode)
        
    def acquireLock(self, obj):
        ''' Acquire a data store lock. '''
        self.lock.acquire()
        logging.debug('DataStore lock acquired %s'%str(obj))

    def releaseLock(self, obj):
        ''' Release the data store lock. ''' 
        self.lock.release()
        logging.debug('DataStore lock released%s'%str(obj))
        
class DataStoreGridSummary(threading.Thread):
    ''' This class implements the thread that periodically runs a summary over all of the clusters.
        It bases its summary data on the summaries that have been previously calculated 
        for each cluster. '''
    def __init__(self):
        # Initialize the thread
        threading.Thread.__init__(self)

        self._cond = threading.Condition()
        self._running = False
        self._shuttingDown = False

    def _doGridSummary(self):
        ''' This method summarizes the entire grid based on summary data
            acquired from the cluster summaries. '''
        ds = DataStore()
        rootNode = ds.rootElement
        # If the data store doesn't contain a root node yet, then there is no
        #  reason to do a grid summary.
        if rootNode is None: return
        # Lock the data store before starting the summary.
        ds.acquireLock(self)
        try:
            # Summarize each grid.  There should only be one for now.
            for gridNode in rootNode:
                # Clear out the summaryData for the grid from the last run and initialize
                #  the dictionary for new summaries.
                gridNode.summaryData = {}
                gridNode.summaryData['summary'] = {}
                gridNode.summaryData['hosts_up'] = 0
                gridNode.summaryData['hosts_down'] = 0
                for clusterNode in gridNode:
                    # Sum up the status of all of the hosts
                    try:
                        gridNode.summaryData['hosts_up'] += clusterNode.summaryData['hosts_up']
                        gridNode.summaryData['hosts_down'] += clusterNode.summaryData['hosts_down']
                    except AttributeError:
                        pass
                    except KeyError:
                        pass
                    # Summarize over all of the metrics in the cluster node summary.
                    for metricNode in clusterNode.summaryData['summary'].itervalues():
                        # Don't include metrics that can not be summarized
                        if metricNode.getAttr('type') in ['string', 'timestamp']:
                            continue
                        try:
                            # Pull the existing summary node from the summary data
                            #   dictionary. If one doesn't exist, add it in the exception.
                            summaryNode = gridNode.summaryData['summary'][str(metricNode)]
                            summaryNode.incAttr('sum', metricNode.getAttr('sum'))
                        except KeyError:
                            # Create the new summary node with correct tag.
                            summaryNode = metricNode.summaryCopy(tag=metricNode.tag)
                            # Add the new summary node to the grid summary dictionary
                            gridNode.summaryData['summary'][str(summaryNode)] = summaryNode
                            summaryNode.setAttr('sum', metricNode.getAttr('sum'))
                        summaryNode.incAttr('num', 1)
        except Exception, e:
            print 'Grid summary ' + str(e) 
        ds.releaseLock(self)

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
                self._doGridSummary()

    def shutdown(self):
        # Release all locks and tell the thread to shut down.
        self._shuttingDown = True
        self._cond.acquire()
        self._cond.notifyAll()
        self._cond.release()
        self.join()
        
