AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/gmond -I$(top_builddir)/libmetrics -I$(top_builddir)/lib 

pkglib_LTLIBRARIES    = modmulticpu.la
modmulticpu_la_SOURCES = mod_multicpu.c 
modmulticpu_la_LDFLAGS = -module

EXTRA_DIST = ../conf.d/multicpu.conf
INCLUDES = @APR_INCLUDES@
