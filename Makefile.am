# $Id$
#

if BUILD_DOTCONF
DOTCONF_SUBDIR = dotconf-1.0.9
endif

if BUILD_LIBDNET
LIBDNET_SUBDIR = libdnet-1.4
endif

SUBDIRS = $(LIBDNET_SUBDIR) $(DOTCONF_SUBDIR) lib gmond gmetric

DIST_SUBDIRS = $(LIBDNET_SUBDIR) $(DOTCONF_SUBDIR) lib gmond gmetric

EXTRA_DIST = config gmond.init ganglia.spec gmond.init.SuSE mans lib/ganglia 

# This is a symbolic link that needs to be clean'd out
dist-hook:
	rm -f ${distdir}/gmond/machine.c
