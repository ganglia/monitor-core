SUBDIRS = gstat

EXTRA_DIST = machines gmond.init gmond.conf

AM_CFLAGS = -I$(top_builddir)/lib -I$(top_builddir)/lib/dnet

sbin_PROGRAMS = gmond
gmond_SOURCES =  gmond.c monitor.c server.c listen.c cleanup.c node_data_t.h \
	machine.c metric.h metric_typedefs.h key_metrics.h cmdline.c cmdline.h \
	dtd.h interface.c interface.h
gmond_LDADD   = $(top_builddir)/lib/libganglia.la $(top_builddir)/lib/libgetopthelper.a
gmond_LDFLAGS = -static

distclean-local: 
	rm -f machine.c 
