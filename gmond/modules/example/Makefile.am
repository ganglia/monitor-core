AM_CFLAGS  = -I$(top_builddir)/gmond -I$(top_builddir)/libmetrics -I$(top_builddir)/lib -I$(top_builddir)/srclib/apr/include/

pkglib_LTLIBRARIES    = modexample.la
modexample_la_SOURCES = mod_example.c 
modexample_la_LDFLAGS = -module

EXTRA_DIST = ../conf.d/example.conf
