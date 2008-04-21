import threading
import xml.sax
import socket
import time
import logging

from gmetad_config import GmetadConfig
from gmetad_random import getRandomInterval
from gmetad_data import DataStore, Element

class GmondContentHandler(xml.sax.ContentHandler):
    def __init__(self):
        xml.sax.ContentHandler.__init__(self)
        self._elemStack = []
        self._elemStackLen = 0
        
    def startElement(self, tag, attrs):
        ds = DataStore()
        e = Element(tag, attrs)
        if 'GANGLIA_XML' == tag:
            ds.lock.acquire()
            self._elemStack.append(ds.setNode(e))
            self._elemStackLen += 1
            cfg = GmetadConfig()
            e = Element('GRID', {'NAME':cfg[GmetadConfig.GRIDNAME], 'AUTHORITY':cfg[GmetadConfig.AUTHORITY], 'LOCALTIME':'%d' % time.time()})
        self._elemStack.append(ds.setNode(e, self._elemStack[self._elemStackLen-1]))
        self._elemStackLen += 1
        
    def endElement(self, tag):
        if tag == 'GANGLIA_XML':
            DataStore().lock.release()
        self._elemStack.pop()
        self._elemStackLen -= 1

class GmondReader(threading.Thread):
    def __init__(self,dataSource,name=None,target=None,args=(),kwargs={}):
        threading.Thread.__init__(self,name,target,args,kwargs)
        self._cond = threading.Condition()
        self._shuttingDown = False
        self.dataSource = dataSource
        self.lastKnownGoodHost = 0
        logging.debug('Reader created for cluster %s' % self.dataSource.name)
        
    def _getEndpoint(self, hostspec, port=8649):
        hostinfo = hostspec.split(':')
        if len(hostinfo) > 1:
            port = int(hostinfo[1])
        return (hostinfo[0], port)
        
    def run(self):
        while not self._shuttingDown:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            try:
                sock.connect( self._getEndpoint(self.dataSource.hosts[self.lastKnownGoodHost]) )
            except socket.error:
                curidx = self.lastKnownGoodHost
                connected=False
                while True:
                    curidx += 1
                    if curidx >= len(self.dataSource.hosts):
                        curidx = 0
                    if curidx == self.lastKnownGoodHost: break
                    try:
                        sock.connect( self._getEndpoint(self.dataSource.hosts[curidx]) )
                        self.lastKnownGoodHost = curidx
                        connected=True
                        break
                    except socket.error:
                        pass
                if not connected:
                    logging.error('Could not connect to any host for data source %s' % self.dataSource.name)
                    return
            logging.info('Quering data source %s via host %s' % (self.dataSource.name, self.dataSource.hosts[self.lastKnownGoodHost]))
            xmlbuf = ''
            while True:
                buf = sock.recv(8192)
                if not buf:
                    break
                xmlbuf += buf
            sock.close()
            if self._shuttingDown:
                break
            xml.sax.parseString(xmlbuf, GmondContentHandler())
            self._cond.acquire()
            self._cond.wait(getRandomInterval(self.dataSource.interval))
            self._cond.release()        
            
    def shutdown(self):
        self._shuttingDown = True
        self._cond.acquire()
        self._cond.notifyAll()
        self._cond.release()
        self.join()
