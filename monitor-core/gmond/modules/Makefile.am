
if BUILD_PYTHON
PYTHON_SUBDIR = python
endif

if STATIC_BUILD
SUBDIRS = cpu disk memory network system $(PYTHON_SUBDIR)
else
if BUILD_STATUS
STATUS_SUBDIR = status
endif
SUBDIRS = example cpu disk memory network system $(STATUS_SUBDIR) $(PYTHON_SUBDIR)
endif

