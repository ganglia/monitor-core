# $Id$
#

SUBDIRS = lib gmetad gmond gmetric

DIST_SUBDIRS = lib gmond gmetric

EXTRA_DIST = config gmond.init ganglia.spec gmond.init.SuSE mans lib/ganglia 

# This is a symbolic link that needs to be clean'd out
dist-hook:
	rm -f ${distdir}/gmond/machine.c
