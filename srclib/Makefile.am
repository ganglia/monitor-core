
if USE_DYNAMIC_APR
SUBDIRS = expat confuse

install-data-hook:
	rm $(DESTDIR)$(includedir)/confuse.h
	rm $(DESTDIR)$(includedir)/expat.h
	rm $(DESTDIR)$(libdir)/libconfuse.a
	rm $(DESTDIR)$(libdir)/libconfuse.la
	rm $(DESTDIR)$(libdir)/libexpat.a
	rm $(DESTDIR)$(libdir)/libexpat.la
	rm -r $(DESTDIR)$(libdir)/pkgconfig
	if test -f $(DESTDIR)$(bindir)/confuse-config; then rm $(DESTDIR)$(bindir)/confuse-config; fi

else
SUBDIRS = expat apr confuse

test:
	echo
	echo "Skipping tests in the srclib directory"
	echo

check:
	echo
	echo "Skipping check in the srclib directory"
	echo

install:
	echo
	echo "Nothing from the srclib directory gets installed"
	echo
endif
