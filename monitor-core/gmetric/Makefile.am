if USE_DYNAMIC_APR
APR_LIB = 
APR_INC = -D_LARGEFILE64_SOURCE
else
APR_LIB = $(top_builddir)/srclib/apr/libapr-0.la
APR_INC = -I$(top_builddir)/srclib/apr/include/
endif

AM_CFLAGS = -I../lib -I../gmond -I$(top_builddir)/libmetrics $(APR_INC)

bin_PROGRAMS = gmetric
gmetric_SOURCES =  gmetric.c cmdline.c cmdline.h
gmetric_LDADD   =  $(top_builddir)/lib/libganglia.la \
                   $(top_builddir)/lib/libgetopthelper.a \
	           $(top_builddir)/libmetrics/libmetrics.la \
                   $(top_builddir)/srclib/confuse/src/libconfuse.la \
		   $(APR_LIB)
if USE_DYNAMIC_APR
gmetric_LDFLAGS = 
else
gmetric_LDFLAGS = -static
endif
