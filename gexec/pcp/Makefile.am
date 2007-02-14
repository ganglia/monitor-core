CFLAGS				= -O2 -Wall -D_REENTRANT -g
INCLUDES			= -I.

bin_PROGRAMS		= pcp
sbin_PROGRAMS		= pcpd 
lib_LIBRARIES		= libpcp.a
include_HEADERS		= pcp_lib.h

pcp_SOURCES		= pcp.c pcp.h pcp_options.c pcp_options.h msg.h
pcpd_SOURCES		= pcpd.c pcpd.h pcpd_options.c pcpd_options.h msg.h
libpcp_a_SOURCES	= pcp_lib.c pcp_lib.h

pcp_LDADD		= -lpthread -lssl -lcrypto -lpcp -le -lauth
pcpd_LDADD		= -lpthread -lssl -lcrypto -le -lauth

pcp_LDFLAGS		= -L.
pcpd_LDFLAGS		= -L.

EXTRA_DIST		= config/pcpd.in pcp.spec

install-data-local: $(EXTRA_DIST)
	$(INSTALL_SCRIPT) config/pcpd /etc/init.d/pcpd

uninstall-local: $(EXTRA_DIST)
	if [ -f /etc/init.d/pcpd ]; then   \
		/etc/init.d/pcpd stop        ; \
		rm -f /etc/init.d/pcpd       ; \
	fi
