#/*******************************************************************************
#* Copyright (C) 2008 Novell, Inc. All rights reserved.
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
#* Authors: Brad Nicholes (bnicholes novell.com)
#******************************************************************************/
''' This plugin is meant to be a very simplistic example of how a gmetad plugin could be developed
    to analyze metric data as it is read from the various monitored system and produce some
    type of alerting system.  This plugin is probably not suitable for actual production use, but
    could be reworked to create a production alerting mechanism. 
    
    example gmetad.conf alert plugin configuration section:
    
    alert {
        sendmailpath "/usr/sbin/sendmail"
        toaddress "someone@somewhere.com"
        fromaddress "monitor@somewhere.com"
        # metric rule format <metric_name> <condition> <message>
        metricrule cpu_user "$val > 55" "CPU User metric has gone bezerk"
        metricrule load_one "$val > 0.5" "One minute load looks wacko"
    }
    '''
    
import os
import logging
from time import time

from Gmetad.gmetad_plugin import GmetadPlugin
from Gmetad.gmetad_config import getConfig, GmetadConfig

def get_plugin():
    ''' All plugins are required to implement this method.  It is used as the factory
        function that instanciates a new plugin instance. '''
    # The plugin configuration ID that is passed in must match the section name 
    #  in the configuration file.
    return HealthPlugin('alert')

class HealthPlugin(GmetadPlugin):
    ''' This class implements the Health plugin that evaluates metrics and warns
        if out of bounds.'''

    SENDMAIL = 'sendmailpath'
    TO_ADDR = 'toaddress'
    FROM_ADDR = 'fromaddress'
    METRICRULE = 'metricrule'

    # Default RRAs
    _cfgDefaults = {
        SENDMAIL : '/usr/sbin/sendmail', 
        TO_ADDR : '',
        FROM_ADDR : '',
        METRICRULE: {},
    }

    def __init__(self, cfgid):
        self.sendmailPath = None
        self.cfg = None
        self.kwHandlers = None
        self._resetConfig()
        
        # The call to the parent class __init__ must be last
        GmetadPlugin.__init__(self, cfgid)

    def _resetConfig(self):
        self.sendmailPath = None
        self.cfg = HealthPlugin._cfgDefaults
        
        self.kwHandlers = {
            HealthPlugin.SENDMAIL : self._parseSendmailPath,
            HealthPlugin.TO_ADDR : self._parseToAddress,
            HealthPlugin.FROM_ADDR : self._parseFromAddress,
            HealthPlugin.METRICRULE : self._parseMetricRule
        }
    
    def _parseConfig(self, cfgdata):
        '''This method overrides the plugin base class method.  It is used to
            parse the plugin specific configuration directives.'''
        for kw,args in cfgdata:
            if self.kwHandlers.has_key(kw.lower()):
                self.kwHandlers[kw.lower()](args)

    def _parseSendmailPath(self, arg):
        ''' Parse the sendmail path directive. '''
        v = arg.strip().strip('"')
        if os.path.isfile(v):
            self.cfg[HealthPlugin.SENDMAIL] = v

    def _parseToAddress(self, arg):
        ''' Parse the from address directive. '''
        v = arg.strip().strip('"')
        self.cfg[HealthPlugin.TO_ADDR] = v

    def _parseFromAddress(self, arg):
        ''' Parse the from address directive. '''
        v = arg.strip().strip('"')
        self.cfg[HealthPlugin.FROM_ADDR] = v

    def _parseMetricRule(self, arg):
        ''' Parse the metric rule directive. '''
        args = [x.strip() for x in arg.split('"') if len(x.strip()) > 0]
        if len(args) == 3:
            self.cfg[HealthPlugin.METRICRULE][args[0]] = args
        else:
            logging.debug('Invalid metric rule directive: %s'%arg)
            
    def _sendMessage(self, node, msg):
        ''' Send a message using the sendmail program.  This will only work for
            the *nix platforms.  It should be rewritten to support multiple platforms'''
        postmsg = 'To: %s\n'%self.cfg[HealthPlugin.TO_ADDR]
        postmsg += 'From: %s\n'%self.cfg[HealthPlugin.FROM_ADDR]
        postmsg += 'Subject: Metric %s out of range\n' % node.getAttr('name')
        postmsg += '\n'
        postmsg += '%s\n'%msg[2]
        postmsg += 'metric: %s value: %s condition: %s\n'%(msg[0], node.getAttr('val'), msg[1])

        try:
            p = os.popen('%s -t' % self.cfg[HealthPlugin.SENDMAIL], 'w')
            p.write (postmsg)
            ret = p.close()
            logging.debug ('sent message : %s'%postmsg)
        except Exception, e:
            print e
        return
            
    def start(self):
        '''Called by the engine during initialization to get the plugin going.'''
        #print "Alert start called"
        pass
    
    def stop(self):
        '''Called by the engine during shutdown to allow the plugin to shutdown.'''
        #print "Alert stop called"
        pass

    def notify(self, clusterNode):
        '''Called by the engine when the internal data source has changed.'''
        if (self.cfg[HealthPlugin.TO_ADDR] is None) or (len(self.cfg[HealthPlugin.TO_ADDR]) == 0):
            pass
        for hostNode in clusterNode:
            # Query a metricNode so that we can use it to generate a key
            for metricNode in hostNode:
                # Don't evaluate metrics that aren't numeric values.
                if metricNode.getAttr('type') in ['string', 'timestamp']:
                    continue
                try:
                    # For each metric rule that was defined in the config file, build a key
                    #   and see if the current host has a matching metric.  If so then evaluate the
                    #   current value against the metric rule. 
                    for metricRule in self.cfg[HealthPlugin.METRICRULE].itervalues():
                        key = metricNode.generateKey([metricNode.id, metricRule[0]])
                        evalNode = hostNode[key]
                        if evalNode is not None:
                            # The metric rule that is pulled from the config file, should look like a  
                            #  python conditional statement and use the symbol $val as a place holder
                            #  for the actual value.
                            evalStr = metricRule[1].replace('$val', evalNode.getAttr('val'))
                            try:
                                # If the evaluation of the metric rule produces a true condition, send a message
                                #  to the email address acquired from the config file.
                                if eval (evalStr):
                                    self._sendMessage(evalNode, metricRule)
                            except SyntaxError:
                                pass
                except Exception, e:
                    print e
                break
        #print "Alert notify called"
