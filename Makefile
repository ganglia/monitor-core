# Location where gweb should be installed to
DESTDIR = /var/www/html/gweb

APACHE_USER = apache

# Gweb version
GWEB_MAJOR_VERSION = 2
GWEB_MINOR_VERSION = 0
GWEB_MICRO_VERSION = 0

# When set to "yes", GWEB_VERSION will include SVN revision
SNAPSHOT = yes

# Gweb statedir (where RRD files, Dwoo templates are stored)
GWEB_STATEDIR = /var/lib

# Dwoo compile directory
GWEB_DWOO = $(GWEB_STATEDIR)/ganglia/dwoo

ifeq ($(SNAPSHOT),yes)
	GWEB_NANO_VERSION = $(shell svnversion .)
	GWEB_VERSION = ${GWEB_MAJOR_VERSION}.${GWEB_MINOR_VERSION}.${GWEB_MICRO_VERSION}.${GWEB_NANO_VERSION}
else
	GWEB_VERSION = $(GWEB_MAJOR_VERSION).$(GWEB_MINOR_VERSION).$(GWEB_MICRO_VERSION)
endif

DIST_DIR = gweb-$(GWEB_VERSION)
DIST_TARBALL = $(DIST_DIR).tar.gz

TARGETS = conf_default.php gweb.spec version.php .htaccess

default:	$(TARGETS)

clean:
	rm -rf $(TARGETS) $(DIST_DIR) $(DIST_TARBALL)

conf_default.php:	conf_default.php.in
	sed -e "s|@varstatedir@|$(GWEB_STATEDIR)|" conf_default.php.in > conf_default.php

gweb.spec:	gweb.spec.in
	sed -e s/@GWEB_VERSION@/$(GWEB_VERSION)/ -e "s|@varstatedir@|$(GWEB_STATEDIR)|" gweb.spec.in > gweb.spec

version.php:	version.php.in
	sed -e s/@GWEB_VERSION@/$(GWEB_VERSION)/ version.php.in > version.php

dist-dir:	default
	rsync --exclude "$(DIST_DIR)" --exclude ".svn" --exclude ".git*" -a . $(DIST_DIR) && \
	cp -a $(TARGETS) $(DIST_DIR)

.htaccess:	.htaccess.in
	secret=`php -r 'echo sha1(rand().microtime());'` && \
	sed -e "s/@ganglia_secret@/$$secret/" .htaccess.in > .htaccess

install:	dist-dir
	mkdir -p $(DESTDIR) $(DESTDIR)/conf $(GWEB_DWOO) && \
	mv $(DIST_DIR)/conf $(GWEB_STATEDIR)/ganglia && \
	cp -a $(DIST_DIR)/* $(DESTDIR) && \
	chown -R $(APACHE_USER):$(APACHE_USER) $(GWEB_DWOO) $(GWEB_STATEDIR)/ganglia/conf

dist-gzip:	dist-dir
	if [ -f $(DIST_TARBALL) ]; then \
	rm -rf $(DIST_TARBALL) ;\
	fi ;\
	tar -czf $(DIST_TARBALL) $(DIST_DIR)/*

uninstall:
	rm -rf $(DESTDIR) $(GWEB_DWOO)
