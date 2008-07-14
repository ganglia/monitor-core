AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/include -I$(top_builddir)/lib 

if STATIC_BUILD
noinst_LTLIBRARIES   = libmodstatus.la
libmodstatus_la_SOURCES = mod_gstatus.c 
else
pkglib_LTLIBRARIES    = modgstatus.la
modgstatus_la_SOURCES = mod_gstatus.c 
modgstatus_la_LDFLAGS = -module -avoid-version

EXTRA_DIST = ../conf.d/modgstatus.conf
endif

INCLUDES = @APR_INCLUDES@
