CFLAGS				= -O2 -Wall -D_REENTRANT -g
INCLUDES			= -I.

sbin_PROGRAMS		= authd
lib_LIBRARIES		= libauth.a
include_HEADERS		= auth.h

authd_SOURCES		= authd.c authd.h authd_options.c authd_options.h \
				  	  auth.h
libauth_a_SOURCES	= auth.c auth.h

authd_LDADD		= -lpthread -lssl -le

authd_LDFLAGS		= -L.

EXTRA_DIST		= config/gexec-authd gexec-authd.spec

install-data-local: $(EXTRA_DIST)
	$(INSTALL_SCRIPT) config/gexec-authd /etc/init.d/gexec-authd

uninstall-local: $(EXTRA_DIST)
	if [ -f /etc/init.d/gexec-authd ]; then  \
		/etc/init.d/gexec-authd stop        ; \
		rm -f /etc/init.d/gexec-authd       ; \
	fi

