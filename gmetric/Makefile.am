AM_CFLAGS = -I../lib -I../gmond -I$(top_builddir)/srclib/libmetrics

bin_PROGRAMS = gmetric
gmetric_SOURCES =  gmetric.c cmdline.c cmdline.h
gmetric_LDADD   =  $(top_builddir)/lib/libganglia.la \
                   $(top_builddir)/lib/libgetopthelper.a \
	           $(top_builddir)/srclib/libmetrics/libmetrics.la \
                   $(top_builddir)/srclib/confuse/src/libconfuse.la \
		   $(top_builddir)/srclib/apr/libapr-0.la
gmetric_LDFLAGS = -static
