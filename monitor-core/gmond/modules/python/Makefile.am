
include $(top_srcdir)/ganglia.inc

AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/include -I$(top_builddir)/lib 

if STATIC_BUILD
noinst_LTLIBRARIES    = libmodpython.la
libmodpython_la_SOURCES = mod_python.c 
libmodpython_la_LDFLAGS = -export-all-symbols
else
pkglib_LTLIBRARIES    = modpython.la
modpython_la_SOURCES = mod_python.c 
modpython_la_LDFLAGS = -module -avoid-version -lpython@PYTHON_VERSION@

EXTRA_DIST = README.in ../conf.d/modpython.conf.in
endif 

README:	README.in $(FIXCONFIG)
	$(FIXCONFIG) README.in

../conf.d/modpython.conf:	../conf.d/modpython.conf.in $(FIXCONFIG)
	$(FIXCONFIG) ../conf.d/modpython.conf.in

# Note that README is listed as a dependency to be generated, but it
# is not currently installed anywhere
install-exec-hook:	../conf.d/modpython.conf README
	mkdir -p $(DESTDIR)$(sysconfdir)/conf.d && \
	  $(INSTALL_DATA) ../conf.d/modpython.conf $(DESTDIR)$(sysconfdir)/conf.d/modpython.conf

INCLUDES = @APR_INCLUDES@ @PYTHON_INCLUDES@
