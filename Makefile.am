# $Id$
#
if BUILD_GMETAD
GMETAD_SUBDIR = gmetad
else
GMETAD_SUBDIR = 
endif

EXTRA_SUBDIRS = "./web"
EXTRA_DIST_FILES = 
INCLUDES = @APR_INCLUDES@

AUTOMAKE_OPTIONS = dist-bzip2 dist-tarZ dist-shar dist-zip

GANGLIA_SUBDIRS = lib libmetrics tests $(GMETAD_SUBDIR) gmond gstat gmetric include
SUBDIRS = $(GANGLIA_SUBDIRS)
# Make sure that no matter the setting for --with-gmetad
# that the gmetad directory gets distributed
GANGLIA_SUBDIRS_DIST = lib libmetrics tests gmetad gmond gstat gmetric include
DIST_SUBDIRS = $(GANGLIA_SUBDIRS_DIST) 

EXTRA_DIST = README.WIN README.AIX config ganglia.spec.aix ganglia.spec ganglia.pod ganglia.html mans $(EXTRA_DIST_FILES)

ganglia.sub: ganglia.pod
	sed -e 's/GANGLIA_VERSION/@VERSION@/' < $(srcdir)/ganglia.pod > ganglia.sub

man_create: ganglia.sub
	pod2man --section 3 --center "ganglia" --release "@VERSION@" --date "`date`" --name ganglia ganglia.sub ganglia.man

ganglia.html: ganglia.sub
	pod2html --title="Ganglia" --infile=ganglia.sub --outfile=ganglia.html

README: ganglia.sub
	pod2text ganglia.sub README

CHANGE_LOG_GEN_BIN = scripts/svn2cl.sh
CHANGE_LOG_GEN_FLAGS = --revision HEAD:1

ChangeLog::
	touch ChangeLog
	$(CHANGE_LOG_GEN_BIN) $(CHANGE_LOG_GEN_FLAGS)

CLEANFILES = ganglia.sub pod2htm*

dist-local: README ganglia.html ChangeLog


# We go through and make sure all the files have
# the same timestamp with the last command here...
dist-hook:
	for subdir in $(EXTRA_SUBDIRS); do \
	  tar -c --exclude=".libs" --exclude "core.*" \
	  --exclude ".svn" --exclude="*.o"  --exclude="*.lo" \
	  --exclude="*.la" --exclude=".deps" \
	  --exclude="Makefile" --exclude="libtool" \
	  --exclude="config.h" \
	  --exclude="*.spec" --exclude="stamp-h1" --exclude="*-config" \
	  --exclude="autom4te.cache" --exclude="test-metrics" \
	  --exclude="*.tar.gz" --exclude="config.status" \
	  --exclude="config.log" --exclude="config.cache" \
	  --exclude="configure.lineno" --exclude="configure.status.lineno" \
	  -f - $$subdir | (cd $(distdir) && tar -xf -)\
	done;
	find $(distdir) -name ".svn" -type d -prune -exec rm -rf {} \;
	touch $(distdir)/Makefile.am
	find $(distdir) -exec touch -r $(distdir)/Makefile.am {} \;

bin_SCRIPTS = ganglia-config
