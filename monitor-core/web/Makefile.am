
include $(top_srcdir)/ganglia.inc

conf.php:       conf.php.in $(FIXCONFIG)
	$(FIXCONFIG) conf.php.in

version.php:    version.php.in $(FIXCONFIG)
	$(FIXCONFIG) version.php.in

install:	conf.php version.php
	@echo
	@echo "================================================================================="
	@echo "     The PHP scripts are not installed by running 'make install'"
	@echo "     Instead, you need to copy the contents of ./web to your web server."
	@echo "     The file 'conf.php' may need some tweaking based on your configuration."
	@echo "================================================================================="
	@echo

dist-hook:
	tar -c --exclude="*CVS" -f - . | (cd $(distdir) && tar -xf -)
