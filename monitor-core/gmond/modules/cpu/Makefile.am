AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/gmond -I$(top_builddir)/libmetrics -I$(top_builddir)/lib 

if STATIC_BUILD
noinst_LTLIBRARIES    = libmodcpu.la
libmodcpu_la_SOURCES = mod_cpu.c mod_load.c 
else
pkglib_LTLIBRARIES    = modmulticpu.la modcpu.la modload.la

modmulticpu_la_SOURCES = mod_multicpu.c 
modmulticpu_la_LDFLAGS = -module

modcpu_la_SOURCES = mod_cpu.c 
modcpu_la_LDFLAGS = -module 
modcpu_la_LIBADD = $(top_builddir)/libmetrics/libmetrics.la

modload_la_SOURCES = mod_load.c 
modload_la_LDFLAGS = -module 
modload_la_LIBADD = $(top_builddir)/libmetrics/libmetrics.la

EXTRA_DIST = ../conf.d/multicpu.conf
endif

INCLUDES = @APR_INCLUDES@


