AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/include -I$(top_builddir)/libmetrics -I$(top_builddir)/lib 

EXTRA_DIST = ../conf.d/multicpu.conf

if STATIC_BUILD
noinst_LTLIBRARIES    = libmodcpu.la libmodmulticpu.la
libmodcpu_la_SOURCES = mod_cpu.c mod_load.c 
libmodcpu_la_LDFLAGS = -export-all-symbols
libmodmulticpu_la_SOURCES = mod_multicpu.c
libmodmulticpu_la_LDFLAGS = -export-all-symbols

else
pkglib_LTLIBRARIES    = modmulticpu.la modcpu.la modload.la

modmulticpu_la_SOURCES = mod_multicpu.c 
modmulticpu_la_LDFLAGS = -module -avoid-version

modcpu_la_SOURCES = mod_cpu.c 
modcpu_la_LDFLAGS = -module -avoid-version
modcpu_la_LIBADD = $(top_builddir)/libmetrics/libmetrics.la

modload_la_SOURCES = mod_load.c 
modload_la_LDFLAGS = -module -avoid-version
modload_la_LIBADD = $(top_builddir)/libmetrics/libmetrics.la

endif

INCLUDES = @APR_INCLUDES@
