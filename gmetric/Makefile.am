include $(top_srcdir)/ganglia.inc

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

AM_CFLAGS = -I../lib -I../include $(GCFLAGS)

bin_PROGRAMS = gmetric

cmdline.c: cmdline.c.in $(FIXCONFIG)
	$(FIXCONFIG) cmdline.c.in

gmetric_SOURCES =  gmetric.c cmdline.c.in cmdline.c cmdline.h
gmetric_LDADD   =  $(top_builddir)/lib/libganglia.la \
                   $(top_builddir)/lib/libgetopthelper.a \
	           $(top_builddir)/libmetrics/libmetrics.la \
		   $(GLDADD)

gmetric_LDFLAGS = $(GLDFLAGS)

CLEANFILES = cmdline.c

EXTRA_DIST = cmdline.sh

