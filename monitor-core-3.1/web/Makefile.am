install:
	@echo
	@echo "================================================================================="
	@echo "     The PHP scripts are not installed by running 'make install'"
	@echo "     Instead, you need to copy the contents of ./web to your web server."
	@echo "     The file 'conf.php' may need some tweaking based on your configuration."
	@echo "================================================================================="
	@echo

dist-hook:
	tar -c --exclude="*CVS" -f - . | (cd $(distdir) && tar -xf -)
