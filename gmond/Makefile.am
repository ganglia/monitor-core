SUBDIRS = gstat

EXTRA_DIST = gmond.init gmond.conf.5 gmond.conf.html conf.pod

AM_CFLAGS = -I$(top_builddir)/lib -I$(top_builddir)/srclib/libmetrics/ -I$(top_builddir)/srclib/expat/lib/ -I$(top_builddir)/srclib/apr/include/ -I$(top_builddir)/srclib/confuse/src/

sbin_PROGRAMS = gmond
# metric.h and dtd.h are for gmetad/gmetric for now....
gmond_SOURCES = gmond.c cmdline.c cmdline.h dtd.h metric.h g25_config.c g25_config.h

man_MANS  = gmond.conf.5

gmond.conf.5: conf.pod gmond.conf.html
	pod2man --name="gmond.conf" --section=5 conf.pod --center="Ganglia Monitoring System" --release="ganglia/@VERSION@" > gmond.conf.5

gmond.conf.html: conf.pod
	pod2html --title="Ganglia Monitoring Daemon v@VERSION@ Configuration" --header --infile=conf.pod --outfile=gmond.conf.html

#monitor.c server.c listen.c cleanup.c \
#metric.h dtd.h 
# Add -lintl??? for libconfuse on cygwin?
gmond_LDADD   = $(top_builddir)/lib/libganglia.la \
	        $(top_builddir)/lib/libgetopthelper.a \
                $(top_builddir)/srclib/libmetrics/libmetrics.la \
		$(top_builddir)/srclib/confuse/src/libconfuse.la \
		$(top_builddir)/srclib/apr/libapr-0.la
gmond_LDFLAGS = -static
gmond_DEPENDENCIES = $(gmond_LDADD)
