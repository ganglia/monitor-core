# $Id$
#
if BUILD_GMETAD
GMETAD_SUBDIR = gmetad
else
GMETAD_SUBDIR = 
endif

GANGLIA_SUBDIRS = lib tests $(GMETAD_SUBDIR) gmond gmetric

AUTOMAKE_OPTIONS = dist-bzip2 dist-tarZ dist-shar dist-zip

# Make sure we run the srclib but don't distribute it.  That
# is handled in the "dist-hook" below...
SUBDIRS = srclib $(GANGLIA_SUBDIRS)
# Make sure that no matter the setting for --with-gmetad
# that the gmetad directory gets distributed
DIST_SUBDIRS = $(GANGLIA_SUBDIRS) gmetad

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
	touch ChangeLog
#	$(CHANGE_LOG_GEN_BIN) $(CHANGE_LOG_GEN_FLAGS)

CLEANFILES = ganglia.sub pod2htm*

dist-local: README ganglia.html ChangeLog


# We handle distributing the ./srclib directory here
# since the packages in that directory might not
# handle a "make dist" correctly
dist-hook:
	for subdir in "./srclib ./web"; do \
	  tar -c --exclude=".libs" --exclude "core.*" --exclude="*CVS" \
	  --exclude="*.o"  --exclude="*.lo" \
	  --exclude="*.la" --exclude=".deps" \
	  --exclude="*.tar.gz" \
	  -f - $$subdir | (cd $(distdir) && tar -xf -)\
	done;

bin_SCRIPTS = ganglia-config
