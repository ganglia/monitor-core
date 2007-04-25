SUBDIRS = gstat modules

EXTRA_DIST = gmond.aix.init gmond.init gmond.init.SuSE gmond.conf.5 gmond.conf.html conf.pod

if USE_DYNAMIC_APR
APR_LIB = 
APR_INC = -D_LARGEFILE64_SOURCE
else
APR_LIB = $(top_builddir)/srclib/apr/libapr-0.la
APR_INC = -I$(top_builddir)/srclib/apr/include/
endif

AM_CFLAGS = -I$(top_builddir)/lib -I$(top_builddir)/libmetrics/ -I$(top_builddir)/srclib/expat/lib/ $(APR_INC) -I$(top_builddir)/srclib/confuse/src/

sbin_PROGRAMS = gmond
# metric.h and dtd.h are for gmetad/gmetric for now....
gmond_SOURCES = gmond.c cmdline.c cmdline.h dtd.h metric.h g25_config.c g25_config.h gm_mmn.h

noinst_MANS  = gmond.conf.5

gmond.conf.5: conf.pod gmond.conf.html
	pod2man --name="gmond.conf" --section=5 conf.pod --center="Ganglia Monitoring System" --release="ganglia/@VERSION@" > gmond.conf.5

gmond.conf.html: conf.pod
	pod2html --title="Ganglia Monitoring Daemon v@VERSION@ Configuration" --header --infile=conf.pod --outfile=gmond.conf.html

#monitor.c server.c listen.c cleanup.c \
#metric.h dtd.h 
# Add -lintl??? for libconfuse on cygwin?
gmond_LDADD   = $(top_builddir)/libmetrics/libmetrics.la \
		$(top_builddir)/lib/libganglia.la \
	        $(top_builddir)/lib/libgetopthelper.a \
		$(top_builddir)/srclib/confuse/src/libconfuse.la \
		$(APR_LIB)

gmond_DEPENDENCIES = $(gmond_LDADD)

if USE_DYNAMIC_APR
gmond_LDFLAGS = 
else
gmond_LDFLAGS = -static
endif

