AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/gmond -I$(top_builddir)/libmetrics -I$(top_builddir)/lib 

pkglib_LTLIBRARIES    = moddisk.la

moddisk_la_SOURCES = mod_disk.c 
moddisk_la_LDFLAGS = -module 
moddisk_la_LIBADD = $(top_builddir)/libmetrics/libmetrics.la


EXTRA_DIST = ../conf.d/disk.conf
