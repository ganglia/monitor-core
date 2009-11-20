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
AM_CFLAGS = -I$(top_builddir)/lib -I$(top_builddir)/gmond -I$(top_builddir)/include $(GCFLAGS) -DSYSCONFDIR='"$(sysconfdir)"'

sbin_PROGRAMS = gmetad 
gmetad_SOURCES =  gmetad.c cmdline.c cmdline.h gmetad.h data_thread.c \
   server.c process_xml.c rrd_helpers.c conf.c conf.h type_hash.c \
   xml_hash.c cleanup.c rrd_helpers.h daemon_init.c daemon_init.h
gmetad_LDADD   = $(top_builddir)/lib/libganglia.la -lrrd -lm \
                 $(GLDADD)

gmetad_LDFLAGS = $(GLDFLAGS)

EXTRA_DIST = gmetad.aix.init gmetad.conf gmetad.init gmetad.init.SuSE \
    type_hash.gperf xml_hash.gperf gmetad-default

xml_hash.c: xml_hash.gperf
	gperf -l -H xml_hash -t -F ', 0' -N in_xml_list -k '1,$$' -W xml_tags xml_hash.gperf > xml_hash.c

type_hash.c: type_hash.gperf
	gperf -G -l -H type_hash -t -F ', 0' -N in_type_list -k '1,$$' -W types type_hash.gperf > type_hash.c
