AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/gmond -I$(top_builddir)/libmetrics -I$(top_builddir)/lib 

pkglib_LTLIBRARIES    = modmem.la

modmem_la_SOURCES = mod_mem.c 
modmem_la_LDFLAGS = -module 
modmem_la_LIBADD = $(top_builddir)/libmetrics/libmetrics.la

EXTRA_DIST = ../conf.d/mem.conf
INCLUDES = @APR_INCLUDES@


