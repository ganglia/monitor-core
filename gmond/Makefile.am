SUBDIRS = gstat 

EXTRA_DIST = machines

AM_CFLAGS = -I../lib -I../lib/dnet

sbin_PROGRAMS = gmond
gmond_SOURCES =  gmond.c monitor.c server.c listen.c node_data_t.h \
	machine.c metric.h metric_typedefs.h key_metrics.h cmdline.c cmdline.h \
	interface.c interface.h dtd.h
gmond_LDADD   = ../lib/.libs/libganglia.a ../lib/libdnet.a

distclean-local: 
	rm -f machine.c 
