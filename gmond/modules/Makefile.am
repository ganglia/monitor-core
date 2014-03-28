if BUILD_PYTHON
PYTHON_SUBDIR = python
endif

if BUILD_PERL
PERL_SUBDIR = perl
endif

if BUILD_STATUS
STATUS_SUBDIR = status
endif

DIST_SUBDIRS = example
DIST_SUBDIRS += cpu disk
DIST_SUBDIRS += memory
DIST_SUBDIRS += network
DIST_SUBDIRS += system
DIST_SUBDIRS += status
DIST_SUBDIRS += python
DIST_SUBDIRS += perl
if STATIC_BUILD
SUBDIRS = cpu
SUBDIRS += disk
SUBDIRS += memory
SUBDIRS += network
SUBDIRS += system
SUBDIRS += $(STATUS_SUBDIR)
SUBDIRS += $(PYTHON_SUBDIR)
SUBDIRS += $(PERL_SUBDIR)
else
SUBDIRS = example
SUBDIRS += cpu
SUBDIRS += disk
SUBDIRS += memory
SUBDIRS += network
SUBDIRS += system
SUBDIRS += $(STATUS_SUBDIR)
SUBDIRS += $(PYTHON_SUBDIR)
SUBDIRS += $(PERL_SUBDIR)

install: install-recursive
	@rm -rf $(DESTDIR)$(pkglibdir)/*.a
	@rm -rf $(DESTDIR)$(pkglibdir)/*.la

endif
