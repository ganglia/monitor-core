
include $(top_srcdir)/ganglia.inc

AM_CFLAGS  = -D_LARGEFILE64_SOURCE -I$(top_builddir)/include -I$(top_builddir)/lib 

if STATIC_BUILD
noinst_LTLIBRARIES    = libmodperl.la
libmodperl_la_SOURCES = mod_perl.c 
libmodperl_la_LDFLAGS = -export-all-symbols
else
pkglib_LTLIBRARIES    = modperl.la
modperl_la_SOURCES = mod_perl.c
#modperl_la_CFLAGS = @PERL_CFLAGS@
modperl_la_LDFLAGS = -module -avoid-version @PERL_LDFLAGS@

EXTRA_DIST = README.in ../conf.d/modperl.conf.in
endif 

README:	README.in $(FIXCONFIG)
	$(FIXCONFIG) $< $@

$(builddir)/../conf.d/modperl.conf:	$(srcdir)/../conf.d/modperl.conf.in $(FIXCONFIG)
	mkdir -p $(builddir)/../conf.d && \
	  $(FIXCONFIG) $< $@

# Note that README is listed as a dependency to be generated, but it
# is not currently installed anywhere
install-exec-hook:	$(builddir)/../conf.d/modperl.conf README
	mkdir -p $(DESTDIR)$(sysconfdir)/conf.d && \
	  $(INSTALL_DATA) $(builddir)/../conf.d/modperl.conf $(DESTDIR)$(sysconfdir)/conf.d/modperl.conf

CLEANFILES = $(builddir)/../conf.d/modperl.conf README

uninstall-hook:
	rm $(DESTDIR)$(sysconfdir)/conf.d/modperl.conf

INCLUDES = @APR_INCLUDES@ @PERL_INCLUDES@
