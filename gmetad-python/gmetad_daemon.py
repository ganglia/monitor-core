import os
import pwd
import sys
import resource

from gmetad_config import getConfig, GmetadConfig

def daemonize(ignore_fds=[]):
    UMASK=0
    WORKDIR = '/'
    MAXFD = 1024
    REDIRECT_TO = '/dev/null'
    if hasattr(os, 'devnull'):
        REDIRECT_TO = os.devnull
        
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
    
