AM_CFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/lib 

if STATIC_BUILD
noinst_LTLIBRARIES    = libmodexample.la
libmodexample_la_SOURCES = mod_example.c 
else
pkglib_LTLIBRARIES    = modexample.la
modexample_la_SOURCES = mod_example.c 
modexample_la_LDFLAGS = -module -avoid-version

EXTRA_DIST = ../conf.d/example.conf
endif

INCLUDES = @APR_INCLUDES@

install:
	@echo
	@echo "Examples should be installed manually"
	@echo
