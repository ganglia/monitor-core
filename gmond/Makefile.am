SUBDIRS = gstat

EXTRA_DIST = gmond.init gmond.conf

AM_CFLAGS = -I$(top_builddir)/lib -I$(top_builddir)/srclib/libmetrics/ -I$(top_builddir)/srclib/expat/lib/ -I$(top_builddir)/srclib/apr/include/ -I$(top_builddir)/srclib/confuse/src/

sbin_PROGRAMS = gmond
# metric.h and dtd.h are for gmetad/gmetric for now....
gmond_SOURCES = gmond.c cmdline.c cmdline.h conf.c conf.h dtd.h metric.h

#monitor.c server.c listen.c cleanup.c \
#metric.h cmdline.c cmdline.h dtd.h conf.c conf.h
# Add -lintl??? for libconfuse on cygwin?
gmond_LDADD   = $(top_builddir)/lib/libganglia.la \
	        $(top_builddir)/lib/libgetopthelper.a \
                $(top_builddir)/srclib/libmetrics/libmetrics.la \
                $(top_builddir)/srclib/apr/libapr-0.la \
		$(top_builddir)/srclib/confuse/src/libconfuse.la
gmond_LDFLAGS = -static
gmond_DEPENDENCIES = $(gmond_LDADD)
