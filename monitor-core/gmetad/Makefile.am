sbin_PROGRAMS = gmetad 

gmetad_SOURCES =  gmetad.c cmdline.c cmdline.h gmetad.h data_thread.c server.c process_xml.c \
	rrd_helpers.c conf.c conf.h type_hash.c xml_hash.c
gmetad_LDADD   = $(top_builddir)/lib/libganglia.la -lrrd -lm -lexpat
gmetad_LDFLAGS = -static

EXTRA_DIST = gmetad.conf gmetad.init
