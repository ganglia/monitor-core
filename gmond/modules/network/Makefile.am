AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/include -I$(top_builddir)/libmetrics -I$(top_builddir)/lib 

if STATIC_BUILD
noinst_LTLIBRARIES    = libmodnet.la
libmodnet_la_SOURCES = mod_net.c
libmodnet_la_LDFLAGS = -export-all-symbols
else
pkglib_LTLIBRARIES    = modnet.la

modnet_la_SOURCES = mod_net.c 
modnet_la_LDFLAGS = -module -avoid-version
modnet_la_LIBADD = $(top_builddir)/libmetrics/libmetrics.la
endif

INCLUDES = @APR_INCLUDES@
