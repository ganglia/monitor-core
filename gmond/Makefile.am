SUBDIRS = gstat 

EXTRA_DIST = machines

if BUILD_DOTCONF 
DOTCONF_INCLUDES = -I../dotconf-1.0.9/src
DOTCONF_LIBS = ../dotconf-1.0.9/src/dotconf.lo 
else
DOTCONF_LIBS = -ldotconf
endif

if BUILD_LIBDNET
LIBDNET_INCLUDES = -I../libdnet-1.4/include
LIBDNET_LIBS = ../libdnet-1.4/src/libdnet.a
else
LIBDNET_LIBS = -ldnet
endif

AM_CFLAGS = -I../lib $(DOTCONF_INCLUDES) $(LIBDNET_INCLUDES)

sbin_PROGRAMS = gmond
gmond_SOURCES =  gmond.c monitor.c server.c listen.c node_data_t.h \
	machine.c metric.h metric_typedefs.h key_metrics.h cmdline.c cmdline.h \
	interface.c
gmond_LDADD   = ../lib/.libs/libganglia.a $(DOTCONF_LIBS) $(LIBDNET_LIBS)
# gmond_LDFLAGS =  -L../libdaemon -L../libhash
# gmond_CFLAGS  = -fcheck-memory-usage

distclean-local: 
	rm -f machine.c 
