# $Id$
#
if BUILD_GMETAD
GMETAD_SUBDIR = gmetad
else
GMETAD_SUBDIR = 
endif

SUBDIRS = lib $(GMETAD_SUBDIR) gmond gmetric

DIST_SUBDIRS = lib gmetad gmond gmetric

EXTRA_DIST = config ganglia.spec ganglia.pod ganglia.man ganglia.html

man3_MANS = ganglia.man

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

dist-local: man_create README ganglia.html ChangeLog

# This is a symbolic link that needs to be clean'd out
dist-hook:
	rm -f ${distdir}/gmond/machine.c
