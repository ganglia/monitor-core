import thread
import logging
import time

from gmetad_config import getConfig, GmetadConfig

class DataStore:
    _shared_state = {}
    _initialized = False
    lock = thread.allocate_lock()
    
    def __init__(self):
        self.__dict__ = DataStore._shared_state
        if not DataStore._initialized:
            self.lock = thread.allocate_lock()
            self.lock.acquire()
            self.rootElement = None
            # Initialize the data store with the GANGLIA_XML and GRID tags.
            # Data store should never be completely empty.  Even if there are
            # no reporting data sources the web front end depends on having
            # at least a GANGLIA_XML tag and a nested GRID tag.
            self.setNode(Element('GANGLIA_XML', {}))
            cfg = getConfig()
            self.setNode(Element('GRID', {'NAME':cfg[GmetadConfig.GRIDNAME], 'AUTHORITY':cfg[GmetadConfig.AUTHORITY], 'LOCALTIME':'%d' % time.time()}), self.rootElement)
            self.lock.release()
            DataStore._initialized = True
            
    def setNode(self, node, parent=None):
        if parent is None:
            if self.rootElement is None:
                self.rootElement = node
                self.rootElement.source = 'gmetad'
            return self.rootElement
        parent[str(node)] = node
        return parent[str(node)]
        
    def getNode(self, ancestry=[]):
        if not len(ancestry):
            return self.rootElement
        node = None
        while ancestry:
            nodeId = ancestry.pop(0)
            if node is None:
                if nodeId == str(self.rootElement):
                    node = self.rootElement
                else: return None
            else:
                try:
                    return node[nodeId]
                except KeyError:
                    pass
        return node
        
class Element:
    def generateKey(vals):
        if isinstance(vals,list):
            return ':'.join(vals)
        return vals
    generateKey = staticmethod(generateKey)
    
    def __init__(self, id, attrs):
        self.id = id
        for k,v in attrs.items():
            self.__dict__[k.lower()] = v
        self.children = {}
        
    def __setitem__(self, k, v):
        try:
            self.children[k].update(v)
        except KeyError:
            self.children[k] = v
            
    def __getitem__(self, k):
        return self.children[k]
        
    def update(self, elem):
        for k in self.__dict__.keys():
            if k == 'children' or k == 'id' or k == 'name':
                continue
            try:
                self.__dict__[k] = elem.__dict__[k]
            except ValueError:
                pass
        
    def __str__(self):
        if self.__dict__.has_key('name'):
            return Element.generateKey([self.id,self.name])
        return Element.generateKey(self.id)
