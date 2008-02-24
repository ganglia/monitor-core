AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/gmond -I$(top_builddir)/libmetrics -I$(top_builddir)/lib 

if STATIC_BUILD
AM_CFLAGS += -I$(top_builddir)/srclib/apr/include/ -I$(top_builddir)/srclib/confuse/src/
noinst_LTLIBRARIES    = libmoddisk.la
libmoddisk_la_SOURCES = mod_disk.c 
else
pkglib_LTLIBRARIES    = moddisk.la
moddisk_la_SOURCES = mod_disk.c 
moddisk_la_LDFLAGS = -module 
moddisk_la_LIBADD = $(top_builddir)/libmetrics/libmetrics.la

INCLUDES = @APR_INCLUDES@

endif


