AM_CFLAGS = -O0 -I$(top_builddir)/lib -I$(top_builddir)/gmond

sbin_PROGRAMS = gmetad 
gmetad_SOURCES =  gmetad.c cmdline.c cmdline.h gmetad.h data_thread.c \
   server.c process_xml.c rrd_helpers.c conf.c conf.h type_hash.c \
   xml_hash.c cleanup.c
gmetad_LDADD   = $(top_builddir)/lib/libganglia.la -lrrd -lm \
                 $(top_builddir)/srclib/expat/lib/libexpat.la
gmetad_LDFLAGS = -static
EXTRA_DIST = gmetad.conf gmetad.init
