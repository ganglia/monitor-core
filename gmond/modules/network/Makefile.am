AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/gmond -I$(top_builddir)/libmetrics -I$(top_builddir)/lib 

if STATIC_BUILD
AM_CFLAGS += -I$(top_builddir)/srclib/apr/include/ -I$(top_builddir)/srclib/confuse/src/
noinst_LTLIBRARIES    = libmodnet.la
libmodnet_la_SOURCES = mod_net.c
else
pkglib_LTLIBRARIES    = modnet.la

modnet_la_SOURCES = mod_net.c 
modnet_la_LDFLAGS = -module 
modnet_la_LIBADD = $(top_builddir)/libmetrics/libmetrics.la

INCLUDES = @APR_INCLUDES@

endif


