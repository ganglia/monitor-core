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

import threading
import cPickle as pickle
import zlib
import logging

from gmetad_element import Element
from gmetad_config import getConfig, GmetadConfig
from gmetad_plugin import load_plugins, start_plugins, stop_plugins, notify_plugins

_decode = lambda x: (pickle.loads(zlib.decompress(x)))
_encode = lambda x: buffer(zlib.compress(pickle.dumps(x, pickle.HIGHEST_PROTOCOL)))

class GmetadNotifier(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)

        gmetadConfig = getConfig()
        load_plugins(gmetadConfig[GmetadConfig.PLUGINS_DIR])
        start_plugins()
    
        self._cond = threading.Condition()
        self._running = False
        self._shuttingDown = False
        self._transQueue = []
        
    def insertTransaction(self, node):
        if node is not None:
            transNode = _encode(node)
            self._transQueue.append( transNode)
            logging.debug('Inserted transaction %s in to the queue' % str(node))

    def run(self):
        if self._running:
            return
        self._running = True
        while not self._shuttingDown:
            self._cond.acquire()
            self._cond.wait(1)
            if len(self._transQueue) > 0:
                transNode = self._transQueue.pop(0)
                node = _decode(transNode)
                logging.debug('Processing transaction %s' % str(node))
                notify_plugins(node)
            self._cond.release()        
            
    def shutdown(self):
        self._shuttingDown = True
        self._cond.acquire()
        self._cond.notifyAll()
        self._cond.release()
        self.join()
        stop_plugins()
