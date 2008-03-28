AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/include -I$(top_builddir)/libmetrics -I$(top_builddir)/lib 

if STATIC_BUILD
noinst_LTLIBRARIES    = libmodmem.la
libmodmem_la_SOURCES = mod_mem.c 
libmodmem_la_LDFLAGS = -export-all-symbols
else
pkglib_LTLIBRARIES    = modmem.la

modmem_la_SOURCES = mod_mem.c 
modmem_la_LDFLAGS = -module -avoid-version
modmem_la_LIBADD = $(top_builddir)/libmetrics/libmetrics.la
endif

INCLUDES = @APR_INCLUDES@
