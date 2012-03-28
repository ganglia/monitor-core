
include $(top_srcdir)/ganglia.inc

SUBDIRS = modules python_modules

EXTRA_DIST = gmond.aix.init gmond.solaris.init.in gmond.init gmond.init.SuSE gmond.conf.5 gmond.conf.html conf.pod cmdline.sh
if STATIC_BUILD
GCFLAGS = -D_LARGEFILE64_SOURCE
if BUILD_STATUS
MOD_GSTATUS = modules/status/mod_gstatus.lo
endif
GLDADD = modules/disk/mod_disk.lo \
         modules/cpu/mod_cpu.lo modules/cpu/mod_load.lo \
         modules/memory/mod_mem.lo modules/network/mod_net.lo \
         modules/system/mod_proc.lo modules/system/mod_sys.lo \
         modules/cpu/mod_multicpu.lo \
         $(MOD_GSTATUS) @STATIC_USER_MODULES@
GLDFLAGS = -static @EXPORT_SYMBOLS@
if BUILD_PYTHON
GLDADD += modules/python/mod_python.lo
GLDFLAGS += -lpython@PYTHON_VERSION@
endif
else
GCFLAGS = -D_LARGEFILE64_SOURCE
GLDADD = 
GLDFLAGS = @EXPORT_SYMBOLS_DYNAMIC@
endif
if BUILD_SFLOW
GCFLAGS += -DSFLOW
endif

INCLUDES = @APR_INCLUDES@
AM_CFLAGS = -I$(top_builddir)/lib -I$(top_builddir)/include/ -I$(top_builddir)/libmetrics $(GCFLAGS)

sbin_PROGRAMS = gmond

cmdline.c: cmdline.c.in $(FIXCONFIG)
	$(FIXCONFIG) cmdline.c.in

# metric.h and dtd.h are for gmetad/gmetric for now....
gmond_SOURCES = gmond.c cmdline.c.in cmdline.c cmdline.h dtd.h g25_config.c core_metrics.c g25_config.h gmond_internal.h
if BUILD_SFLOW
gmond_SOURCES += sflow.h sflow.c sflow_gmetric.h
endif
dist_man_MANS  = gmond.conf.5

gmond.conf.5: conf.pod gmond.conf.html
	pod2man --name="gmond.conf" --section=5 conf.pod --center="Ganglia Monitoring System" --release="ganglia/@VERSION@" > gmond.conf.5

gmond.conf.html: conf.pod
	pod2html --title="Ganglia Monitoring Daemon v@VERSION@ Configuration" --header --infile=conf.pod --outfile=gmond.conf.html

#monitor.c server.c listen.c cleanup.c \
#metric.h dtd.h 
gmond_LDADD = $(top_builddir)/libmetrics/libmetrics.la \
              $(top_builddir)/lib/libganglia.la \
              $(top_builddir)/lib/libgetopthelper.a \
              $(GLDADD)

gmond_DEPENDENCIES = $(gmond_LDADD)

gmond_LDFLAGS = $(GLDFLAGS)

gmond.solaris.init:	gmond.solaris.init.in $(FIXCONFIG)
	$(FIXCONFIG) gmond.solaris.init.in

# we declare the hook so that gmond.solaris.init will be created within the
# build tree during the install phase
# however, the action to install it is commented out, as it is likely
# the SA will use some other script to put this file into a package or
# something
install-exec-hook:	gmond.solaris.init
#	$(INSTALL_SCRIPT) gmond.solaris.init

CLEANFILES = cmdline.c	
