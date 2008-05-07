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
