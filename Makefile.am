# $Id$
#
if BUILD_GMETAD
GMETAD_SUBDIR = gmetad
else
GMETAD_SUBDIR = 
endif

AUTOMAKE_OPTIONS = dist-bzip2 dist-tarZ dist-shar dist-zip

# Make sure we run the srclib but don't distribute it.  That
# is handled in the "dist-hook" below...
GANGLIA_SUBDIRS = lib libmetrics srclib tests $(GMETAD_SUBDIR) gmond gmetric
SUBDIRS = $(GANGLIA_SUBDIRS)
# Make sure that no matter the setting for --with-gmetad
# that the gmetad directory gets distributed
GANGLIA_SUBDIRS_DIST = lib libmetrics tests gmetad gmond gmetric
DIST_SUBDIRS = $(GANGLIA_SUBDIRS_DIST) 

EXTRA_DIST = README.WIN README.AIX config ganglia.spec.aix ganglia.spec ganglia.pod ganglia.html mans

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


# We handle distributing the ./srclib directory here
# since the packages in that directory might not
# handle a "make dist" correctly
#
# We also go through and make sure all the files have
# the same timestamp with the last command here...
dist-hook:
	for subdir in "./srclib ./web"; do \
	  tar -c --exclude=".libs" --exclude "core.*" \
	  --exclude ".svn" --exclude="*.o"  --exclude="*.lo" \
	  --exclude="*.la" --exclude=".deps" \
	  --exclude="Makefile" --exclude="libtool" \
	  --exclude="web/version.php" --exclude="config.h" \
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

#
# Make sure distclean is done in the scrlib tree
#
distclean-local:
	cd srclib &&  $(MAKE) $(AM_MAKEFLAGS) distclean
	-rm -f configure ganglia.html ChangeLog README *.tar.gz aclocal.m4 Makefile.in config.h.in
	-rm config/depcomp config/missing config/config.guess config/config.sub config/install-sh
	-find . -name "autom4te.cache" -exec rm -rf {} \;
	-find gmond gmetad gmetric lib tests libmetrics -name "Makefile.in" -exec rm -f {} \;
#
bin_SCRIPTS = ganglia-config
