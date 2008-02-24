AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/gmond -I$(top_builddir)/libmetrics -I$(top_builddir)/lib 

if STATIC_BUILD
AM_CFLAGS += -I$(top_builddir)/srclib/apr/include/ -I$(top_builddir)/srclib/confuse/src/
noinst_LTLIBRARIES    = libmodmem.la
libmodmem_la_SOURCES = mod_mem.c 
else
pkglib_LTLIBRARIES    = modmem.la

modmem_la_SOURCES = mod_mem.c 
modmem_la_LDFLAGS = -module 
modmem_la_LIBADD = $(top_builddir)/libmetrics/libmetrics.la

INCLUDES = @APR_INCLUDES@

endif


