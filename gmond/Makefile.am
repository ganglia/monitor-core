SUBDIRS = gstat modules python_modules

EXTRA_DIST = gmond.aix.init gmond.init gmond.init.SuSE gmond.conf.5 gmond.conf.html conf.pod
if STATIC_BUILD
GCFLAGS = -I$(top_builddir)/srclib/apr/include/ -I$(top_builddir)/srclib/expat/lib/ -I$(top_builddir)/srclib/confuse/src/
GLDADD = $(top_builddir)/srclib/apr/libapr-0.la $(top_builddir)/srclib/confuse/src/libconfuse.la \
	modules/disk/mod_disk.lo modules/cpu/mod_cpu.lo modules/cpu/mod_load.lo \
	modules/memory/mod_mem.lo modules/network/mod_net.lo \
	modules/system/mod_proc.lo modules/system/mod_sys.lo
GLDFLAGS = -static -export-dynamic
if BUILD_PYTHON
GLDADD += modules/python/mod_python.lo
GLDFLAGS += -lpython@PYTHON_VERSION@
endif
else
GCFLAGS = -D_LARGEFILE64_SOURCE
GLDADD = 
GLDFLAGS = -export-dynamic
INCLUDES = @APR_INCLUDES@
endif

AM_CFLAGS = -I$(top_builddir)/lib -I$(top_builddir)/libmetrics/ $(GCFLAGS) 

sbin_PROGRAMS = gmond
# metric.h and dtd.h are for gmetad/gmetric for now....
gmond_SOURCES = gmond.c cmdline.c cmdline.h dtd.h metric.h g25_config.c core_metrics.c g25_config.h gm_mmn.h

noinst_MANS  = gmond.conf.5

gmond.conf.5: conf.pod gmond.conf.html
	pod2man --name="gmond.conf" --section=5 conf.pod --center="Ganglia Monitoring System" --release="ganglia/@VERSION@" > gmond.conf.5

gmond.conf.html: conf.pod
	pod2html --title="Ganglia Monitoring Daemon v@VERSION@ Configuration" --header --infile=conf.pod --outfile=gmond.conf.html

#monitor.c server.c listen.c cleanup.c \
#metric.h dtd.h 
# Add -lintl??? for libconfuse on cygwin?
gmond_LDADD   = $(top_builddir)/libmetrics/libmetrics.la \
		$(top_builddir)/lib/libganglia.la \
	        $(top_builddir)/lib/libgetopthelper.a \
		$(GLDADD)

gmond_DEPENDENCIES = $(gmond_LDADD)

gmond_LDFLAGS = $(GLDFLAGS)

