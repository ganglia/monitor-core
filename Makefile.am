CFLAGS				= -O2 -Wall -D_REENTRANT -g -D_GNU_SOURCE -DDEBUG
INCLUDES			= -I.

bin_PROGRAMS		= gexec
sbin_PROGRAMS		= gexecd
lib_LIBRARIES		= libgexec.a
include_HEADERS     = gexec_lib.h

gexec_SOURCES		= gexec.c gexec.h gexec_options.c gexec_options.h \
					  msg.h
gexecd_SOURCES		= gexecd.c gexecd.h gexecd_options.c gexecd_options.h \
				      msg.h		
libgexec_a_SOURCES	= gexec_lib.c gexec_lib.h request.c request.h \
				      gexec_state.h

gexec_LDADD			= -lpthread -lgexec -le -lauth -lssl -lcrypto @GANGLIA_LIB@
gexecd_LDADD		= -lpthread -lgexec -le -lauth -lssl -lcrypto

gexec_LDFLAGS		= -L.
gexecd_LDFLAGS		= -L.

EXTRA_DIST			= config/gexec.in gexec.spec

install-data-local: $(EXTRA_DIST)
	$(INSTALL_DATA) config/gexec /etc/xinetd.d/gexec
	@echo "Sending SIGUSR2 to xinetd"
	kill -USR2 `cat /var/run/xinetd.pid`

uninstall-local: $(EXTRA_DIST)
	if [ -f /etc/xinetd.d/gexec ]; then   \
		rm -f /etc/xinetd.d/gexec       ; \
	fi
	@echo "Sending SIGUSR2 to xinetd"
	kill -USR2 `cat /var/run/xinetd.pid`
