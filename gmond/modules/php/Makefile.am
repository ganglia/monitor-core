
include $(top_srcdir)/ganglia.inc

AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/include -I$(top_builddir)/lib

if STATIC_BUILD
noinst_LTLIBRARIES    = libmodphp.la
libmodphp_la_SOURCES = mod_php.c 
libmodphp_la_LDFLAGS = -export-all-symbols
else
pkglib_LTLIBRARIES    = modphp.la
modphp_la_SOURCES = mod_php.c 
modphp_la_LDFLAGS = -module -avoid-version -lphp@PHP_VERSION@

EXTRA_DIST = README.in ../conf.d/modphp.conf.in
endif 

README:	README.in $(FIXCONFIG)
	$(FIXCONFIG) README.in

../conf.d/modphp.conf:	../conf.d/modphp.conf.in $(FIXCONFIG)
	$(FIXCONFIG) ../conf.d/modphp.conf.in

# Note that README is listed as a dependency to be generated, but it
# is not currently installed anywhere
install-exec-hook:	../conf.d/modphp.conf README
	mkdir -p $(DESTDIR)$(sysconfdir)/conf.d && \
	  $(INSTALL_DATA) ../conf.d/modphp.conf $(DESTDIR)$(sysconfdir)/conf.d/modphp.conf

INCLUDES = @APR_INCLUDES@ @PHP_INCLUDES@
