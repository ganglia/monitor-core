import thread
import time

from gmetad_config import GmetadConfig
from gmetad_data import DataStore, Element

class XmlWriter:
    _xml_starttag = '<?xml version="1.0" encoding="ISO-8859-1" standalone="yes"?>'
    _xml_dtd = '''<!DOCTYPE GANGLIA_XML [
   <!ELEMENT GANGLIA_XML (GRID|CLUSTER|HOST)*>
      <!ATTLIST GANGLIA_XML VERSION CDATA #REQUIRED>
      <!ATTLIST GANGLIA_XML SOURCE CDATA #REQUIRED>
   <!ELEMENT GRID (CLUSTER | GRID | HOSTS | METRICS)*>
      <!ATTLIST GRID NAME CDATA #REQUIRED>
      <!ATTLIST GRID AUTHORITY CDATA #REQUIRED>
      <!ATTLIST GRID LOCALTIME CDATA #IMPLIED>
   <!ELEMENT CLUSTER (HOST | HOSTS | METRICS)*>
      <!ATTLIST CLUSTER NAME CDATA #REQUIRED>
      <!ATTLIST CLUSTER OWNER CDATA #IMPLIED>
      <!ATTLIST CLUSTER LATLONG CDATA #IMPLIED>
      <!ATTLIST CLUSTER URL CDATA #IMPLIED>
      <!ATTLIST CLUSTER LOCALTIME CDATA #REQUIRED>
   <!ELEMENT HOST (METRIC)*>
      <!ATTLIST HOST NAME CDATA #REQUIRED>
      <!ATTLIST HOST IP CDATA #REQUIRED>
      <!ATTLIST HOST LOCATION CDATA #IMPLIED>
      <!ATTLIST HOST REPORTED CDATA #REQUIRED>
      <!ATTLIST HOST TN CDATA #IMPLIED>
      <!ATTLIST HOST TMAX CDATA #IMPLIED>
      <!ATTLIST HOST DMAX CDATA #IMPLIED>
      <!ATTLIST HOST GMOND_STARTED CDATA #IMPLIED>
   <!ELEMENT METRIC (EXTRA_DATA*)>
      <!ATTLIST METRIC NAME CDATA #REQUIRED>
      <!ATTLIST METRIC VAL CDATA #REQUIRED>
      <!ATTLIST METRIC TYPE (string | int8 | uint8 | int16 | uint16 | int32 | uint32 | float | double | timestamp) #REQUIRED>
      <!ATTLIST METRIC UNITS CDATA #IMPLIED>
      <!ATTLIST METRIC TN CDATA #IMPLIED>
      <!ATTLIST METRIC TMAX CDATA #IMPLIED>
      <!ATTLIST METRIC DMAX CDATA #IMPLIED>
      <!ATTLIST METRIC SLOPE (zero | positive | negative | both | unspecified) #IMPLIED>
      <!ATTLIST METRIC SOURCE (gmond | gmetric) #REQUIRED>
   <!ELEMENT EXTRA_DATA (EXTRA_ELEMENT*)>
   <!ELEMENT EXTRA_ELEMENT EMPTY>
      <!ATTLIST METRIC NAME CDATA #REQUIRED>
      <!ATTLIST METRIC VAL CDATA #REQUIRED>
   <!ELEMENT HOSTS EMPTY>
      <!ATTLIST HOSTS UP CDATA #REQUIRED>
      <!ATTLIST HOSTS DOWN CDATA #REQUIRED>
      <!ATTLIST HOSTS SOURCE (gmond | gmetric | gmetad) #REQUIRED>
   <!ELEMENT METRICS EMPTY>
      <!ATTLIST METRICS NAME CDATA #REQUIRED>
      <!ATTLIST METRICS SUM CDATA #REQUIRED>
      <!ATTLIST METRICS NUM CDATA #REQUIRED>
      <!ATTLIST METRICS TYPE (string | int8 | uint8 | int16 | uint16 | int32 | uint32 | float | double | timestamp) #REQUIRED>
      <!ATTLIST METRICS UNITS CDATA #IMPLIED>
      <!ATTLIST METRICS SLOPE (zero | positive | negative | both | unspecified) #IMPLIED>
      <!ATTLIST METRICS SOURCE (gmond | gmetric) #REQUIRED>
]>'''
    _pcid_map = {'GANGLIA_XML' : 'GRID',
            'GRID' : 'CLUSTER',
            'CLUSTER' : 'HOST',
            'HOST' : 'METRIC'
    }
    
    def __init__(self):
        cfg = GmetadConfig()
        self.gridname = cfg[GmetadConfig.GRIDNAME]
        self.authority = cfg[GmetadConfig.AUTHORITY]
        self.localtime = time.time()
        
    def _getXmlImpl(self, element, filterList=None):
        rbuf = '<%s' % element.id
        foundName = False
        try:
            rbuf += ' NAME="%s"' % element.name
            foundName = True
        except AttributeError:
            pass
        for k,v in element.__dict__.items():
            if k == 'id' or k == 'children' or (foundName and k == 'name'):
                continue
            rbuf += ' %s="%s"' % (k.upper(), v)
        if 0 < len(element.children):
            rbuf += '>\n'
            showAllChildren = True
            if filterList is not None and len(filterList):
                try:
                    key = Element.generateKey([self._pcid_map[element.id], filterList[0]])
                    rbuf += self._getXmlImpl(element.children[key], filterList[1:])
                    showAllChildren = False
                except KeyError:
                    pass
            if showAllChildren:
                for c in element.children.values():
                    rbuf += self._getXmlImpl(c, filterList)
            rbuf += '</%s>\n' % element.id
        else:
            rbuf += ' />\n'
        return rbuf
            
    def getXml(self, filter=None):
        if filter is None:
            filterList = None
        elif not len(filter.strip()):
            filterList = None
        else:
            filterList = filter.split('/')
        rbuf = '%s\n%s\n' % (self._xml_starttag, self._xml_dtd)
        if DataStore().rootElement is not None:
            rbuf += self._getXmlImpl(DataStore().rootElement, filterList)
        return rbuf
