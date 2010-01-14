

FIXCONFIG = $(top_srcdir)/scripts/fixconfig

# Unfortunately, we can't do this here with a pattern rule because
# that is a GNU make feature and is not fully portable
#%:	%.in $(FIXCONFIG)
#	$(FIXCONFIG) $<

# For the moment, it is necessary to provide a rule for each file
# we want to generate - see the rule for ganglia-config in Makefile.am
# for an example

