AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/gmond -I$(top_builddir)/libmetrics -I$(top_builddir)/lib 

if STATIC_BUILD
AM_CFLAGS += -I$(top_builddir)/srclib/apr/include/
noinst_LTLIBRARIES    = libmodexample.la
libmodexample_la_SOURCES = mod_example.c 
else
pkglib_LTLIBRARIES    = modexample.la
modexample_la_SOURCES = mod_example.c 
modexample_la_LDFLAGS = -module

EXTRA_DIST = ../conf.d/example.conf

INCLUDES = @APR_INCLUDES@

endif

install:
	echo
	echo "Examples should be installed manually"
	echo
