AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/gmond -I$(top_builddir)/libmetrics -I$(top_builddir)/lib 

pkglib_LTLIBRARIES    = modpython.la
modpython_la_SOURCES = mod_python.c 
modpython_la_LDFLAGS = -module -lpython2.4

EXTRA_DIST = ../conf.d/modpython.conf
