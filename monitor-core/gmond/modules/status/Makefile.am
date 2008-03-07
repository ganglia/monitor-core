AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/gmond -I$(top_builddir)/libmetrics -I$(top_builddir)/lib 

if STATIC_BUILD
AM_CFLAGS += -I$(top_builddir)/srclib/apr/include/ -I$(top_builddir)/srclib/confuse/src/
noinst_LTLIBRARIES   = libmodstatus.la
libmodstatus_la_SOURCES = mod_gstatus.c 
else
pkglib_LTLIBRARIES    = modgstatus.la
modgstatus_la_SOURCES = mod_gstatus.c 
modgstatus_la_LDFLAGS = -module

EXTRA_DIST = ../conf.d/modgstatus.conf

INCLUDES = @APR_INCLUDES@

endif
