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
    
def start_plugins():
    global _plugins
    
    for plugin in _plugins:
        plugin.start()

def stop_plugins():
    global _plugins
    
    for plugin in _plugins:
        plugin.stop()
    
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
