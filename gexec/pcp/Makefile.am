CFLAGS				= -O2 -Wall -D_REENTRANT -g
INCLUDES			= -I.

bin_PROGRAMS		= gexec-pcp
sbin_PROGRAMS		= gexec-pcpd
lib_LIBRARIES		= libpcp.a
include_HEADERS		= pcp_lib.h

gexec_pcp_SOURCES	= pcp.c pcp.h pcp_options.c pcp_options.h msg.h
gexec_pcpd_SOURCES	= pcpd.c pcpd.h pcpd_options.c pcpd_options.h msg.h
libpcp_a_SOURCES	= pcp_lib.c pcp_lib.h

gexec_pcp_LDADD		= -lpthread -lssl -lcrypto -lpcp -le -lauth
gexec_pcpd_LDADD		= -lpthread -lssl -lcrypto -le -lauth

gexec_pcp_LDFLAGS	= -L.
gexec_pcpd_LDFLAGS	= -L.

EXTRA_DIST		= config/gexec-pcpd.in gexec-pcp.spec

install-data-local: $(EXTRA_DIST)
	$(INSTALL_SCRIPT) config/gexec-pcpd /etc/init.d/gexec-pcpd

uninstall-local: $(EXTRA_DIST)
	if [ -f /etc/init.d/gexec-pcpd ]; then   \
		/etc/init.d/gexec-pcpd stop        ; \
		rm -f /etc/init.d/gexec-pcpd       ; \
	fi
