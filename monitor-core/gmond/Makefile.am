SUBDIRS = gstat

EXTRA_DIST = machines gmond.init gmond.conf

sbin_PROGRAMS = gmond

gmond_SOURCES =  conf.c conf.h gmond.c monitor.c server.c listen.c cleanup.c node_data_t.h \
	machine.c metric.h metric_typedefs.h key_metrics.h cmdline.c cmdline.h dtd.h
gmond_LDADD   = $(top_builddir)/lib/libganglia.la $(top_builddir)/lib/libgetopthelper.a -lz
gmond_LDFLAGS = -static

distclean-local: 
	rm -f machine.c 
