#if BUILD_RRDTOOL
#RRDTOOL_INCLUDES = -I../rrdtool-1.0.39/src
#RRDTOOL_LIBS = ../rrdtool-1.0.39/src/librrd.la
#else
#RRDTOOL_LIBS = -lrrd
#endif

AM_CFLAGS = -I../lib -I../gmond

sbin_PROGRAMS = gmetad 
gmetad_SOURCES =  gmetad.c cmdline.c cmdline.h gmetad.h data_thread.c server.c save_to_rrd.c \
	rrd_helpers.c conf.c metric_hash.c
gmetad_LDADD   = ../lib/.libs/libganglia.a -lrrd
# gmond_LDFLAGS =  -L../libdaemon -L../libhash
# gmond_CFLAGS  = -fcheck-memory-usage

distclean-local: 
	rm -f machine.c 
