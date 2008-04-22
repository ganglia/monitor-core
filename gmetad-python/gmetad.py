#!/usr/bin/env python

import sys
import socket
import os
import asyncore
import logging
import logging.handlers

from gmetad_gmondReader import GmondReader
from gmetad_xmlWriter import XmlWriter
from gmetad_config import getConfig, GmetadConfig
from gmetad_daemon import daemonize


class GmetadListenSocket(asyncore.dispatcher):
    def __init__(self):
        asyncore.dispatcher.__init__(self)
        
    def open(self, port, interface=''):
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.set_reuse_addr()
        host = 'localhost'
        if 0 < len(interface.strip()):
            host = interface
        logging.info('Opening query interface on %s:%d' % (host, port))
        self.bind((interface,port))
        self.listen(5)
        
    def _connIsAllowedFrom(self, remoteHost):
        cfg = getConfig()
        if '127.0.0.1' == remoteHost: return True
        if 'localhost' == remoteHost: return True
        if cfg[GmetadConfig.ALL_TRUSTED]: return True
        trustedHosts = cfg[GmetadConfig.TRUSTED_HOSTS]
        if trustedHosts.count(remoteHost): return True
        hostname, aliases, ips = socket.gethostbyaddr(remoteHost)
        if trustedHosts.count(hostname): return True
        for alias in aliases:
            if trustedHosts.count(alias): return True
        for ip in ips:
            if trustedHosts.count(ips): return True
        return False

class XmlSocket(GmetadListenSocket):
    def handle_accept(self):
        newsock, addr = self.accept()
        if self._connIsAllowedFrom(addr[0]):
            logging.debug('Replying to XML dump query from %s' % addr[0])
            writer = XmlWriter()
            newsock.sendall(writer.getXml())
            newsock.close()
        else:
            logging.info('XML dump query from %s rejected by rule' % addr[0])


class InteractiveSocket(GmetadListenSocket):        
    def handle_accept(self):
        newsock, addr = self.accept()
        if self._connIsAllowedFrom(addr[0]):
            logging.debug('Replying to interactive query from %s' % addr[0])
            InteractiveConnectionHandler(newsock)
        else:
            logging.info('Interactive query from %s rejected by rule' % addr[0])
        
class InteractiveConnectionHandler(asyncore.dispatcher_with_send):
    def __init__(self, sock):
        asyncore.dispatcher_with_send.__init__(self, sock)
        self.buffer = ''
        self.amt_to_write = 0
        
    def writable(self):
        return self.amt_to_write
        
    def handle_read(self):
        rbuf = self.recv(1024)
        if rbuf:
            rbuf = rbuf.strip().strip('/')
            if 0 == len(rbuf):
                rbuf = None
            queryargs = None
            if rbuf is not None and -1 != rbuf.find('?'):
                queryargs = {}
                try:
                    rbuf, query = rbuf.split('?')
                    query = query.split('&')
                    for q in query:
                        k,v = q.split('=')
                        queryargs[k] = v
                except ValueError:
                    pass
            writer = XmlWriter()
            self.buffer = writer.getXml(rbuf, queryargs)
            self.amt_to_write = len(self.buffer)
        
    def handle_write(self):
        sent = self.socket.send(self.buffer)
        self.buffer = self.buffer[sent:]
        self.amt_to_write -= sent
        if not self.amt_to_write:
            self.close()
            
def getLoggingLevel(lspec):
    levelMap = {0:logging.FATAL,
            1:logging.CRITICAL,
            2:logging.ERROR,
            3:logging.WARNING,
            4:logging.INFO,
            5:logging.DEBUG}
    try:
        return levelMap[lspec]
    except KeyError:
        if lspec < 0: return logging.FATAL
        return logging.DEBUG

if __name__ == '__main__':
    gmetadConfig = getConfig()
    
    ignore_fds = [] # Remembers log file descriptors we create, so they aren't closed when we daemonize.
    logging.basicConfig(level=getLoggingLevel(gmetadConfig[GmetadConfig.DEBUG_LEVEL]),
            format='%(levelname)-8s %(message)s')    
    syslogHandler = logging.handlers.SysLogHandler('/dev/log')
    syslogHandler.setLevel(getLoggingLevel(gmetadConfig[GmetadConfig.DEBUG_LEVEL]))
    syslogHandler.setFormatter(logging.Formatter(fmt='%(asctime)s %(levelname)-8s - GMETAD - %(message)s',
            datefmt='%a, %d %b %Y %H:%M:%S'))
    ignore_fds.append(syslogHandler.socket.fileno())
    logging.getLogger().addHandler(syslogHandler)
    if gmetadConfig[GmetadConfig.LOGFILE] is not None:
        fileHandler = logging.FileHandler(gmetadConfig[GmetadConfig.LOGFILE],'a')
        fileHandler.setLevel(getLoggingLevel(gmetadConfig[GmetadConfig.DEBUG_LEVEL]))
        fileHandler.setFormatter(logging.Formatter(fmt='%(asctime)s %(levelname)-8s %(message)s',
                datefmt='%a, %d %b %Y %H:%M:%S'))
        ignore_fds.append(fileHandler.stream.fileno())
        logging.getLogger().addHandler(fileHandler)

    if 5 > int(gmetadConfig[GmetadConfig.DEBUG_LEVEL]):
        daemonize(ignore_fds)
        
    logging.info('Gmetad application started.')
    
    pffd = None
    if gmetadConfig[GmetadConfig.PIDFILE] is not None:
        try:
            pffd = open(gmetadConfig[GmetadConfig.PIDFILE], 'w')
            pffd.write('%d\n' % os.getpid())
            logging.debug('Wrote pid %d to pidfile %s' % (os.getpid(), gmetadConfig[GmetadConfig.PIDFILE]))
            pffd.close()
            pffd = open(gmetadConfig[GmetadConfig.PIDFILE], 'r')
        except Exception, e:
            logger.error('Unable to write PID %d to %s (%s)' % (os.getpid(), gmetadConfig[GmetadConfig.PIDFILE], e))
            sys.exit()
         
         
    # load modules here, when we support modules.
    
    readers = []
    xmlSocket = XmlSocket()
    interactiveSocket = InteractiveSocket()
    try:
        try:
            for ds in gmetadConfig[GmetadConfig.DATA_SOURCE]:
                readers.append(GmondReader(ds))
                gr = readers[len(readers)-1]
                gr.start()
            xmlSocket.open(port=int(gmetadConfig[GmetadConfig.XML_PORT]))
            interactiveSocket.open(port=int(gmetadConfig[GmetadConfig.INTERACTIVE_PORT]))
            asyncore.loop()
        except KeyboardInterrupt:
            logging.info('Shutting down...')
        except Exception, e:
            logging.error('Caught exception: %s' % e)
            raise
    finally:
        logging.debug('Shutting down all data source readers...')
        for gr in readers:
            gr.shutdown()
        logging.debug('Closing all query ports...')
        xmlSocket.close()
        interactiveSocket.close()
        if pffd is not None:
            pffd.close()
            os.unlink(gmetadConfig[GmetadConfig.PIDFILE])
        

    
