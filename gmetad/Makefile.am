
include $(top_srcdir)/ganglia.inc

if STATIC_BUILD
GCFLAGS = -D_LARGEFILE64_SOURCE
GLDADD = 
GLDFLAGS = -static
else
GCFLAGS = -D_LARGEFILE64_SOURCE
GLDADD = 
GLDFLAGS = 
endif

INCLUDES = @APR_INCLUDES@
AM_CFLAGS = -I$(top_builddir)/lib -I$(top_builddir)/gmond -I$(top_builddir)/include $(GCFLAGS)

sbin_PROGRAMS = gmetad

cmdline.c: cmdline.c.in $(FIXCONFIG)
	$(FIXCONFIG) cmdline.c.in

gmetad_SOURCES =  gmetad.c cmdline.c.in cmdline.c cmdline.h gmetad.h data_thread.c \
   server.c process_xml.c rrd_helpers.c export_helpers.c conf.c conf.h type_hash.c \
   xml_hash.c cleanup.c rrd_helpers.h export_helpers.h daemon_init.c daemon_init.h \
	 server_priv.h circuit_breaker.c
if BUILD_RIEMANN
  gmetad_SOURCES += riemann.pb-c.h riemann.pb-c.c
endif

gmetad_LDADD   = $(top_builddir)/lib/libganglia.la -lrrd -lm \
                 $(GLDADD) $(DEPS_LIBS)

gmetad_LDFLAGS = $(GLDFLAGS)

EXTRA_DIST = gmetad.aix.init gmetad.conf.in gmetad.init gmetad.init.SuSE \
    type_hash.gperf xml_hash.gperf gmetad-default cmdline.sh gmetad.service.in

xml_hash.c: xml_hash.gperf
	gperf -l -H xml_hash -t -F ', 0' -N in_xml_list -k '1,$$' -W xml_tags xml_hash.gperf > xml_hash.c

type_hash.c: type_hash.gperf
	gperf -G -l -H type_hash -t -F ', 0' -N in_type_list -k '1,$$' -W types type_hash.gperf > type_hash.c

gmetad.conf:	gmetad.conf.in $(FIXCONFIG)
	$(FIXCONFIG) gmetad.conf.in

gmetad.service:	gmetad.service.in $(FIXCONFIG)
	$(FIXCONFIG) gmetad.service.in

install-data-hook:	gmetad.conf
	mkdir -p $(DESTDIR)$(sysconfdir) && \
	  $(INSTALL_DATA) gmetad.conf $(DESTDIR)$(sysconfdir)/gmetad.conf

CLEANFILES = cmdline.c

if HAVE_SYSTEMD
systemdsystemunit_DATA = \
        gmetad.service
endif
