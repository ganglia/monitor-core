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
from time import time
from gmetad_plugin import GmetadPlugin
from gmetad_config import getConfig, GmetadConfig

def get_plugin():
    return RRDPlugin('rrd')

class RRDPlugin(GmetadPlugin):

    RRAS = 'RRAs'
    RRD_ROOTDIR = 'rrd_rootdir'

    _cfgDefaults = {
            RRAS : [
                    'RRA:AVERAGE:0.5:1:244',
                    'RRA:AVERAGE:0.5:24:244',
                    'RRA:AVERAGE:0.5:168:244',
                    'RRA:AVERAGE:0.5:672:244',
                    'RRA:AVERAGE:0.5:5760:374'
            ],
            RRD_ROOTDIR : '@varstatedir@/ganglia/rrds',
    }

    def __init__(self, cfgid):
        self.rrdpath = None
        self.cfg = None
        self.kwHandlers = None
        self._resetConfig()
        
        # The call to the parent class __init__ must be last
        GmetadPlugin.__init__(self, cfgid)

    def _resetConfig(self):
        self.rrdpath = None
        self.cfg = RRDPlugin._cfgDefaults
        
        self.kwHandlers = {
            RRDPlugin.RRAS : self._parseRRAs,
            RRDPlugin.RRD_ROOTDIR : self._parseRrdRootdir
        }
    
    def _parseConfig(self, cfgdata):
        '''Should be overridden by subclasses to parse configuration data, if any.'''
        for kw,args in cfgdata:
            if self.kwHandlers.has_key(kw):
                self.kwHandlers[kw](args)

    def _parseRrdRootdir(self, arg):
        v = arg.strip().strip('"')
        if os.path.isdir(v):
            self.cfg[RRDPlugin.RRD_ROOTDIR] = v

    def _parseRRAs(self, args):
        self.cfg[RRDPlugin.RRAS] = []
        for rraspec in args.split():
            self.cfg[RRDPlugin.RRAS].append(rraspec.strip().strip('"'))
            
    def _checkDir(self, dir):
        if not os.path.isdir(dir):
            os.mkdir(dir, 0755)
            
    def _createRRD(self, clusterNode, metricNode, rrdPath, step, summary):
        if metricNode.slope.lower() == 'positive':
            dsType = 'COUNTER'
        else:
            dsType = 'GAUGE'
            
        heartbeat = 8*step
        dsString = 'DS:sum:%s:%d:U:U'%(dsType,heartbeat)
        args = [str(rrdPath), '-b', str(clusterNode.localtime), '-s', str(step), str(dsString)]
        if summary is True:
            dsString = 'DS:num:%s:%d:U:U'%(dsType,heartbeat)
            args.append(str(dsString))
        for rra in self.cfg[RRDPlugin.RRAS]:
            args.append(rra)
        try:
            rrdtool.create(*args)
            logging.debug('Created rrd %s'%rrdPath)
        except Exception, e:
            logging.info('Error creating rrd %s - %s'%(rrdPath, str(e)))
        
    def _updateRRD(self, clusterNode, metricNode, rrdPath, summary):
        if hasattr(clusterNode, 'localtime'):
            processTime = clusterNode.localtime
        else:
            processTime = int(time())
        if summary is True:
            args = [str(rrdPath), '%s:%s:%s'%(str(processTime),str(metricNode.sum),str(metricNode.num))]
        else:
            args = [str(rrdPath), '%s:%s'%(str(processTime),str(metricNode.val))]
        try:
            rrdtool.update(*args)
            #logging.debug('Updated rrd %s with value %s'%(rrdPath, str(metricNode.val)))
        except Exception, e:
            logging.info('Error updating rrd %s - %s'%(rrdPath, str(e)))

    def start(self):
        '''Called by the engine during initialization to get the plugin going.'''
        print "RRD start called"
    
    def stop(self):
        '''Called by the engine during shutdown to allow the plugin to shutdown.'''
        print "RRD stop called"

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
        for hostNode in clusterNode:
            hostPath = '%s/%s'%(clusterPath,hostNode.name)
            self._checkDir(hostPath)
            for metricNode in hostNode:
                #if metricNode.type in ['string', 'timestamp'] or metricNode.slope == 'zero':
                if metricNode.type in ['string', 'timestamp']:
                    continue
                rrdPath = '%s/%s.rrd'%(hostPath, metricNode.name)
                if not os.path.isfile(rrdPath):
                    self._createRRD(clusterNode, metricNode, rrdPath, ds.interval, False)
                #need to do some error checking here if the createRRD failed
                self._updateRRD(clusterNode, metricNode, rrdPath, False)
        print "RRD notify called"
