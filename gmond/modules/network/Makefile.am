AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/gmond -I$(top_builddir)/libmetrics -I$(top_builddir)/lib 

pkglib_LTLIBRARIES    = modnet.la

modnet_la_SOURCES = mod_net.c 
modnet_la_LDFLAGS = -module 
modnet_la_LIBADD = $(top_builddir)/libmetrics/libmetrics.la

EXTRA_DIST = ../conf.d/net.conf
INCLUDES = @APR_INCLUDES@


