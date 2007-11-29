AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/gmond -I$(top_builddir)/libmetrics -I$(top_builddir)/lib 

pkglib_LTLIBRARIES    = modgstatus.la
modgstatus_la_SOURCES = mod_gstatus.c 
modgstatus_la_LDFLAGS = -module

EXTRA_DIST = ../conf.d/modgstatus.conf
INCLUDES = @APR_INCLUDES@
