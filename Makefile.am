# $Id$
#
SUBDIRS = lib gmetad gmond gmetric

#DIST_SUBDIRS = lib gmetad gmond gmetric

EXTRA_DIST = config ganglia.spec ganglia.pod ganglia.html mans

ganglia.sub: ganglia.pod
	sed -e 's/GANGLIA_VERSION/@VERSION@/' < $(srcdir)/ganglia.pod > ganglia.sub

man_create: ganglia.sub
	pod2man --section 3 --center "ganglia" --release "@VERSION@" --date "`date`" --name ganglia ganglia.sub ganglia.man

ganglia.html: ganglia.sub
	pod2html --title="Ganglia" --infile=ganglia.sub --outfile=ganglia.html

README: ganglia.sub
	pod2text ganglia.sub README

CHANGE_LOG_GEN_BIN = scripts/cvs2cl.pl
CHANGE_LOG_GEN_FLAGS = -t -r -b --accum -I ChangeLog --gmt --prune

ChangeLog::
	$(CHANGE_LOG_GEN_BIN) $(CHANGE_LOG_GEN_FLAGS)

CLEANFILES = ganglia.sub pod2htm*

dist-local: README ganglia.html ChangeLog

# This is a symbolic link that needs to be clean'd out
dist-hook:
	rm -f ${distdir}/gmond/machine.c
