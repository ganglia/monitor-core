AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/include -I$(top_builddir)/libmetrics -I$(top_builddir)/lib 

if STATIC_BUILD
noinst_LTLIBRARIES    = libmoddisk.la
libmoddisk_la_SOURCES = mod_disk.c 
libmoddisk_la_LDFLAGS = -export-all-symbols
else
pkglib_LTLIBRARIES    = moddisk.la
moddisk_la_SOURCES = mod_disk.c 
moddisk_la_LDFLAGS = -module -avoid-version
moddisk_la_LIBADD = $(top_builddir)/libmetrics/libmetrics.la
endif

INCLUDES = @APR_INCLUDES@
