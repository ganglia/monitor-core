SUBDIRS = gstat

EXTRA_DIST = gmond.init gmond.conf

AM_CFLAGS = -I$(top_builddir)/lib -I$(top_builddir)/srclib/libmetrics/

sbin_PROGRAMS = gmond
gmond_SOURCES =  gmond.c monitor.c server.c listen.c cleanup.c node_data_t.h \
	metric.h metric_typedefs.h key_metrics.h cmdline.c cmdline.h \
	dtd.h
gmond_LDADD   = $(top_builddir)/lib/libganglia.la \
	$(top_builddir)/lib/libgetopthelper.a $(top_builddir)/srclib/libmetrics/libmetrics.la
gmond_LDFLAGS = -static

# distclean-local: 
#	rm -f machine.c 
