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

import os
import pwd
import sys
import resource

from gmetad_config import getConfig, GmetadConfig

def setuid():
    cfg = getConfig()
    setuid_user = None
    if cfg[GmetadConfig.SETUID]:
        setuid_user = cfg[GmetadConfig.SETUID_USERNAME]
    if setuid_user is not None:
        try:
            os.setuid(pwd.getpwnam(setuid_user)[2])
        except Exception:
            print 'Unable to setuid to user "%s", exiting' % setuid_user
            sys.exit()
        
def daemonize(ignore_fds=[]):
    UMASK=0
    WORKDIR = '/'
    MAXFD = 1024
    REDIRECT_TO = '/dev/null'
    if hasattr(os, 'devnull'):
        REDIRECT_TO = os.devnull
        
    try:
        pid = os.fork()
    except OSError, e:
        raise Exception, 'Daemonize error: %d (%s)' % (e.errno, e.strerror)
    if pid == 0:
        # first child
        os.setsid()
        try:
            pid = os.fork()
        except OSError, e:
            raise Exception, 'Daemonize error: %d (%s)' % (e.errno, e.strerror)
        if pid == 0:
            # second child
            os.chdir(WORKDIR)
            os.umask(UMASK)
        else:
            os._exit(0)
    else:
        os._exit(0)
        
        
    maxfd = resource.getrlimit(resource.RLIMIT_NOFILE)[1]
    if resource.RLIM_INFINITY == maxfd:
        maxfd = MAXFD
    for fd in range(0,maxfd):
        if fd in ignore_fds: continue
        try:
            os.close(fd)
        except OSError:
            pass
            
    os.open(REDIRECT_TO, os.O_RDWR)
    os.dup2(0,1)
    os.dup2(0,2)
    
