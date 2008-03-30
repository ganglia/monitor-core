if STATIC_BUILD
GCFLAGS = 
GLDADD = 
GLDFLAGS = -static
else
GCFLAGS = 
GLDADD = 
GLDFLAGS = 
endif

INCLUDES = @APR_INCLUDES@
AM_CFLAGS = -O0 -D_LARGEFILE64_SOURCE -I$(top_builddir)/lib -I$(top_builddir)/gmond -I$(top_builddir)/include $(GCFLAGS)

sbin_PROGRAMS = gmetad 
gmetad_SOURCES =  gmetad.c cmdline.c cmdline.h gmetad.h data_thread.c \
   server.c process_xml.c rrd_helpers.c conf.c conf.h type_hash.c \
   xml_hash.c cleanup.c rrd_helpers.h
gmetad_LDADD   = $(top_builddir)/lib/libganglia.la -lrrd -lm \
                 $(GLDADD)

gmetad_LDFLAGS = $(GLDFLAGS)

EXTRA_DIST = gmetad.aix.init gmetad.conf gmetad.init gmetad.init.SuSE
