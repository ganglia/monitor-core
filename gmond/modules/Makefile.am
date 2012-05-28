if BUILD_PYTHON
PYTHON_SUBDIR = python
endif

if BUILD_PHP
PHP_SUBDIR = php
endif

if BUILD_STATUS
STATUS_SUBDIR = status
endif

DIST_SUBDIRS = example cpu disk memory network system status python php
if STATIC_BUILD
SUBDIRS = cpu disk memory network system $(STATUS_SUBDIR) $(PYTHON_SUBDIR) $(PHP_SUBDIR)
else
SUBDIRS = example cpu disk memory network system $(STATUS_SUBDIR) $(PYTHON_SUBDIR) $(PHP_SUBDIR)

install: install-recursive
	@rm -rf $(DESTDIR)$(pkglibdir)/*.a
	@rm -rf $(DESTDIR)$(pkglibdir)/*.la

endif
