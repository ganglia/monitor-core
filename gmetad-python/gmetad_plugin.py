import imp
import logging
import os
import sys

from gmetad_config import getConfig

_plugins = []

def load_plugins(pdir):
    global _plugins
    if not os.path.isdir(pdir):
        logger.warning('No such plugin directory "%s", no plugins loaded' % pdir)
        return
    sys.path.append(pdir)
    for plugin_candidate in os.listdir(pdir):
        if plugin_candidate.startswith('.'): continue
        if not plugin_candidate.endswith('.py'): continue
        plugin_name = plugin_candidate[:len(plugin_candidate)-3]
        fp, pathname, description = imp.find_module(plugin_name)
        try:
            try:
                _module = imp.load_module(plugin_name, fp, pathname, description)
                plugin = _module.get_plugin()
                if not isinstance(plugin, GmetadPlugin):
                    logging.warning('Plugin %s is not a gmetad plugin' % plugin_name)
                else:
                    _plugins.append(plugin)
            except Exception, e:
                logging.warning('Failed to load plugin %s (caught exception %s)' % (plugin_name, e))
        finally:
            if fp: fp.close()
            
class GmetadPlugin:  
    def __init__(self, cfgid):
        self.cfgid = cfgid
        self._parseConfig(getConfig().getSection(cfgid))
        
    def _parseConfig(self, cfgdata):
        '''Should be overridden by subclasses to parse configuration data, if any.'''
        pass
        
    def notify(self):
        '''Called by the engine when the internal data structure has changed.
        Should be overridden by subclasses that should be pulling data out of
        the data structure when the data structure is updated.'''
        pass
        
    def start(self):
        '''Called by the engine during initialization to get the plugin going.  Must
        be overridden by subclasses.'''
        raise Exception, 'No definition provided for plugin "start" method.'
    
    def stop(self):
        '''Called by the engine during shutdown to allow the plugin to shutdown.  Must
        be overridden by subclasses.'''
        raise Exception, 'No definition provided for plugin "stop" method.'
