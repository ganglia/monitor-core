#

include $(top_srcdir)/ganglia.inc

if BUILD_GMETAD
GMETAD_SUBDIR = gmetad
else
GMETAD_SUBDIR = 
endif

EXTRA_SUBDIRS = "gmetad-python contrib solaris"
INCLUDES = @APR_INCLUDES@

AUTOMAKE_OPTIONS = dist-bzip2 dist-tarZ dist-zip
ACLOCAL_AMFLAGS = -I m4

GANGLIA_SUBDIRS = lib libmetrics tests $(GMETAD_SUBDIR) gmond gstat gmetric include mans
SUBDIRS = $(GANGLIA_SUBDIRS)
# Make sure that no matter the setting for --with-gmetad
# that the gmetad directory gets distributed
GANGLIA_SUBDIRS_DIST = lib libmetrics tests gmetad gmond gstat gmetric include mans
DIST_SUBDIRS = $(GANGLIA_SUBDIRS_DIST) 

EXTRA_DIST = BUGS README.WIN README.AIX README.GIT ganglia.spec.aix ganglia.spec ganglia.pod ganglia.html ganglia-config.in \
	WiX Makefile.WiX \
	scripts

ganglia.sub: ganglia.pod
	sed -e 's/GANGLIA_VERSION/@VERSION@/' < $(srcdir)/ganglia.pod > ganglia.sub

man_create: ganglia.sub
	pod2man --section 3 --center "ganglia" --release "@VERSION@" --date "`date`" --name ganglia ganglia.sub ganglia.man

ganglia.html: ganglia.sub
	pod2html --title="Ganglia" --infile=ganglia.sub --outfile=ganglia.html

README: ganglia.sub
	pod2text ganglia.sub README

CLEANFILES = ganglia.sub pod2htm*

dist-local: README ganglia.html

# We go through and make sure all the files have
# the same timestamp with the last command here...
dist-hook:
	for subdir in $(EXTRA_SUBDIRS); do \
	  tar -c --exclude=".libs" --exclude "core.*" \
	  --exclude="*.o"  --exclude="*.lo" \
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
	touch $(distdir)/Makefile.am
	find $(distdir) -exec touch -r $(distdir)/Makefile.am {} \;

# _SCRIPTS causes the target to be built at build time
# we want it done at install time
#bin_SCRIPTS = ganglia-config

ganglia-config:	ganglia-config.in $(FIXCONFIG)
	$(FIXCONFIG) ganglia-config.in

install-exec-hook: ganglia-config
	mkdir -p $(DESTDIR)$(bindir) && \
	  $(INSTALL_SCRIPT) ganglia-config $(DESTDIR)$(bindir)/ganglia-config

contrib/ganglia_gmond.xml: contrib/ganglia_gmond.xml.in $(FIXCONFIG)
	$(FIXCONFIG) contrib/ganglia_gmond.xml.in

solaris/pkginfo: solaris/pkginfo.in $(FIXCONFIG)
	$(FIXCONFIG) solaris/pkginfo.in

gmetad-python/gmetad-python.service: gmetad-python/gmetad-python.service.in $(FIXCONFIG)
	$(FIXCONFIG) gmetad-python/gmetad-python.service.in

# Notice that some files declared in this hook never get installed anywhere
# We use the hook to force the files to be generated, but they remain
# within the build tree for someone to inspect or manually copy to
# the desired location if needed
all: contrib/ganglia_gmond.xml solaris/pkginfo gmetad-python/gmetad-python.service
