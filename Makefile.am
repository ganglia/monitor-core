# $Id$
#

if BUILD_DOTCONF
DOTCONF_SUBDIR = dotconf-1.0.9
endif

SUBDIRS = $(DOTCONF_SUBDIR) lib gmond gmetric

DIST_SUBDIRS = $(DOTCONF_SUBDIR) lib gmond gmetric

EXTRA_DIST = config gmond.init ganglia.spec gmond.init.SuSE mans lib/ganglia 

# This is a symbolic link that needs to be clean'd out
dist-hook:
	rm -f ${distdir}/gmond/machine.c
