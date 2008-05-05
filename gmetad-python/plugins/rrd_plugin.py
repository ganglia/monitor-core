from gmetad_plugin import GmetadPlugin

def get_plugin():
    return RRDPlugin()

class RRDPlugin(GmetadPlugin):
    def __init__(self):
        GmetadPlugin.__init__(self, 'rrd')
