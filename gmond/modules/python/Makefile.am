AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/gmond -I$(top_builddir)/libmetrics -I$(top_builddir)/lib 

if STATIC_BUILD
AM_CFLAGS += -I$(top_builddir)/srclib/apr/include/ @PYTHON_INCLUDES@
noinst_LTLIBRARIES    = libmodpython.la
libmodpython_la_SOURCES = mod_python.c 
else
pkglib_LTLIBRARIES    = modpython.la
modpython_la_SOURCES = mod_python.c 
modpython_la_LDFLAGS = -module -lpython@PYTHON_VERSION@

EXTRA_DIST = ../conf.d/modpython.conf

INCLUDES = @APR_INCLUDES@ @PYTHON_INCLUDES@

endif 


