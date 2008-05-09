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
from gmetad_plugin import GmetadPlugin

def get_plugin():
    return RRDPlugin()

class GmetadRRA:
    def __init__(self, args):
        self.args = args
   
class RRDPlugin(GmetadPlugin):

    RRAS = 'RRAs'
    RRD_ROOTDIR = 'rrd_rootdir'

    _cfgDefaults = {
            RRAS : [
                    GmetadRRA('AVERAGE:0.5:1:244'),
                    GmetadRRA('AVERAGE:0.5:24:244'),
                    GmetadRRA('AVERAGE:0.5:168:244'),
                    GmetadRRA('AVERAGE:0.5:672:244'),
                    GmetadRRA('AVERAGE:0.5:5760:374')
            ],
            RRD_ROOTDIR : '@varstatedir@/ganglia/rrds',
    }

    def __init__(self):
        self.rrdpath = None
        self.cfg = RRDPlugin._cfgDefaults
        
        self.kwHandlers = {
            RRDPlugin.RRAS : self._parseRRAs,
            RRDPlugin.RRD_ROOTDIR : self._parseRrdRootdir
        }
        # The call to the parent class __init__ must be last
        GmetadPlugin.__init__(self, 'rrd')

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
            self.cfg[RRDPlugin.RRAS].append(GmetadRRA(rraspec.strip().strip('"').split(':',1)[1]))

    def start(self):
        '''Called by the engine during initialization to get the plugin going.'''
        print "RRD start called"
    
    def stop(self):
        '''Called by the engine during shutdown to allow the plugin to shutdown.'''
        print "RRD stop called"

    def notify(self, clusterNode):
        '''Called by the engine when the internal data structure has changed.'''
        print "RRD notify called"
