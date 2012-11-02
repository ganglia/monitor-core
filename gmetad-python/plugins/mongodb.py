#!/usr/bin/env python

#/*******************************************************************************
# Copyright (c) 2012 IBM Corp.

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#     http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#/*******************************************************************************

'''
    Created on Dec 15, 2011

    A plugin to write ganglia metrics directly to a MongoDB database

    @author: Michael R. Hines, Marcio A. Silva
'''

import os
import logging
from time import time
from Gmetad.gmetad_plugin import GmetadPlugin
from Gmetad.gmetad_config import getConfig, GmetadConfig
from sys import path

def get_plugin():
    ''' 
    All plugins are required to implement this method.  It is used as the factory
    function that instanciates a new plugin instance.
    '''
    # The plugin configuration ID that is passed in must match the section name 
    #  in the configuration file.
    return MongodbPlugin('mongodb')

class MongodbPlugin(GmetadPlugin) :
    '''
    TBD
    '''
    def __init__(self, cfgid):
        self.path = None
        self.api = None

        # The call to the parent class __init__ must be last
        GmetadPlugin.__init__(self, cfgid)

    def _resetConfig(self):
        pass

    def _parseConfig(self, cfgdata):
        '''This method overrides the plugin base class method.  It is used to
            parse the plugin specific configuration directives.'''
 
        for kw,args in cfgdata:
            if kw == "path" :
                self.path = args
            elif kw == "api" :
                self.api = args

        path.insert(0, self.path)
        from lib.operations.api_service_client import CloudBenchClient

        self.cloudbench = CloudBenchClient(self.api)
        self.cloudbench.monitoring_conn_check()
        self.msci = self.cloudbench.msci

        self.obj_cache = {}
        self.expid = self.cloudbench.cldshow("time")["experiment_id"]
        self.username = self.cloudbench.username
        
        self.latest_collection = {"HOST" : "latest_runtime_os_HOST_" + self.username, \
                                  "VM" : "latest_runtime_os_VM_" + self.username}
        self.data_collection = {"HOST" : "runtime_os_HOST_" + self.username, \
                                  "VM" : "runtime_os_VM_" + self.username}

        self.manage_collection = {"HOST" : "management_HOST_" + self.username, \
                                "VM" : "management_VM_" + self.username}

        _msg = "CloudBench contacted successfully. The current experiment id is \""
        _msg += self.expid + "\" and the current username is \"" + self.username
        _msg += "\"."
        logging.debug(_msg)

    def find_object(self, ip, plugin) :
        '''
        TBD
        '''
        msci = plugin.msci
        if ip in plugin.obj_cache :
        #    logging.debug("Cache hit for ip " + ip)
            return plugin.obj_cache[ip]

        #logging.debug("Cache miss for ip " + ip)

        try :
            vm = True
            obj = msci.find_document(plugin.manage_collection["VM"], {"cloud_ip" : ip, "mgt_903_deprovisioning_request_completed" : { "$exists" : False}})
            #obj = msci.find_document(plugin.manage_collection["VM"], {"cloud_ip" : ip})

            if obj is not None : 
                obj = plugin.cloudbench.vmshow(obj["_id"])
            else :
                vm = False
                obj = msci.find_document(plugin.manage_collection["HOST"], {"cloud_ip" : ip})
                if obj is not None : 
                    obj = plugin.cloudbench.hostshow(obj["_id"])
                else :
                    vm = None
                    obj = None

        except Exception, e :
            _status = 23
            _fmsg = str(e)

            obj = None
            vm = None
            logging.debug("Can't get VM or HOST object for ip: " + ip)

        finally :
            plugin.obj_cache[ip] = (vm, obj)

            return vm, obj

    def start(self):
        '''Called by the engine during initialization to get the plugin going.'''
        logging.debug("MongodbWriter start called")

    def stop(self):
        '''Called by the engine during shutdown to allow the plugin to shutdown.'''
        logging.debug("MongodbWriter stop called")        

    def notify(self, clusterNode):
        from lib.operations.api_service_client import CloudBenchClient, makeTimestamp

        for hostNode in clusterNode:

            '''
            Available hostNode keys:
            gmond_started, name, tags, ip, tmax, tn, reported, location, dmax
            '''

            # Cache object attributes using an ordered dictionary

            vm, obj = self.find_object(hostNode.getAttr('ip'), self)

            if vm is None : # error during lookup
                continue

            _data = {"uuid" : obj["uuid"], "expid" : self.expid}
            obj_type = "VM" if vm else "HOST"
            empty = True 

            for metricNode in hostNode:
                '''
                Available metricNode keys:
                name, val, tmax, tn, source, units, dmax, type, slope
                '''
                # Don't evaluate metrics that aren't numeric values.
                if metricNode.getAttr('type') in ['string', 'timestamp']:
                    continue

                attrs = metricNode.getAttrs()
                last = attrs['tn']

                if attrs["units"].strip() == "" :
                    attrs["units"] = "N/A"

                _data[attrs["name"]] = attrs 

                empty = False

            if not empty :
                _now = int(time())
                _data["time_h"] = makeTimestamp(_now)
                _data["time"] = _now 
                _data["latest_update"] = _now

                try :
                    self.msci.add_document(self.data_collection[obj_type], _data)
                    _data["_id"] = obj["uuid"]
                    self.msci.update_document(self.latest_collection[obj_type], _data)

                except Exception, e :
                    _status = 23
                    _fmsg = str(e)
                    logging.error("Could not write to metric store: " + str(_fmsg))