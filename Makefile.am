SUBDIRS = dotconf-1.0.9 lib gmond gmetric

EXTRA_DIST = config gmond.init ganglia.spec gmond.init.SuSE docs mans lib/ganglia 

# This is a symbolic link that needs to be clean'd out
dist-hook:
	rm -f ${distdir}/gmond/machine.c
