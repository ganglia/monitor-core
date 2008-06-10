AM_CFLAGS			= -O2 -Wall -D_REENTRANT -g -D_GNU_SOURCE -DDEBUG
INCLUDES			= -I.

bin_PROGRAMS		= gexec
sbin_PROGRAMS		= gexecd
lib_LIBRARIES		= libgexec.a
include_HEADERS     	= gexec_lib.h

gexec_SOURCES		= gexec.c gexec.h gexec_options.c gexec_options.h \
					  msg.h
gexecd_SOURCES		= gexecd.c gexecd.h gexecd_options.c gexecd_options.h \
				      msg.h		
libgexec_a_SOURCES	= gexec_lib.c gexec_lib.h request.c request.h \
		 	     gexec_state.h

gexec_LDADD		= -lpthread -lgexec -le -lauth -lssl -lcrypto -lexpat @GANGLIA_LIB@
gexecd_LDADD		= -lpthread -lgexec -le -lauth -lssl -lcrypto -lexpat

gexec_LDFLAGS		= -L.
gexecd_LDFLAGS		= -L.

EXTRA_DIST		= config/gexecd.in gexec.spec

install-data-local: $(EXTRA_DIST)
	chmod +x config/gexecd; \
	$(INSTALL_DATA) config/gexecd /etc/init.d/gexecd

uninstall-local: $(EXTRA_DIST)
	if [ -f /etc/init.d/gexecd ]; then   \
		/etc/init.d/gexecd stop        ; \
		rm -f /etc/init.d/gexecd       ; \
	fi

