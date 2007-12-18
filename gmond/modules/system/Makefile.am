AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/gmond -I$(top_builddir)/libmetrics -I$(top_builddir)/lib 

if STATIC_BUILD
noinst_LTLIBRARIES   = libmodsys.la
libmodsys_la_SOURCES = mod_proc.c mod_sys.c 
else
pkglib_LTLIBRARIES   = modproc.la modsys.la

modproc_la_SOURCES = mod_proc.c 
modproc_la_LDFLAGS = -module 
modproc_la_LIBADD = $(top_builddir)/libmetrics/libmetrics.la

modsys_la_SOURCES = mod_sys.c 
modsys_la_LDFLAGS = -module 
modsys_la_LIBADD = $(top_builddir)/libmetrics/libmetrics.la

endif

INCLUDES = @APR_INCLUDES@


