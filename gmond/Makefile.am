SUBDIRS = gstat 

EXTRA_DIST = machines

AM_CFLAGS = -I../lib -I../dotconf-1.0.9/src

sbin_PROGRAMS = gmond
gmond_SOURCES =  gmond.c monitor.c server.c listen.c node_data_t.h \
	machine.c metric.h metric_typedefs.h key_metrics.h cmdline.c cmdline.h 
gmond_LDADD   = ../lib/.libs/libganglia.a ../dotconf-1.0.9/src/.libs/libdotconf.a
# gmond_LDFLAGS =  -L../libdaemon -L../libhash
# gmond_CFLAGS  = -fcheck-memory-usage

distclean-local: 
	rm -f machine.c 
