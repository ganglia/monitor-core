AM_CFLAGS = -I../lib -I../gmond

sbin_PROGRAMS = gmetad 
gmetad_SOURCES =  gmetad.c cmdline.c cmdline.h gmetad.h data_thread.c server.c process_xml.c \
	rrd_helpers.c conf.c metric_hash.c xml_hash.c
gmetad_LDADD   = ../lib/.libs/libganglia.a -lrrd
EXTRA_DIST = gmetad.conf
