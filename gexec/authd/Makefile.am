CFLAGS				= -O2 -Wall -D_REENTRANT -g
INCLUDES			= -I.

sbin_PROGRAMS		= authd
lib_LIBRARIES		= libauth.a
include_HEADERS		= auth.h

authd_SOURCES		= authd.c authd.h authd_options.c authd_options.h \
				  	  auth.h
libauth_a_SOURCES	= auth.c auth.h

authd_LDADD			= -lpthread -lssl -le

authd_LDFLAGS		= -L.

EXTRA_DIST			= config/authd.in authd.spec

install-data-local: $(EXTRA_DIST)
	$(INSTALL_SCRIPT) config/authd /etc/init.d/authd
	/etc/init.d/authd start

uninstall-local: $(EXTRA_DIST)
	if [ -f /etc/init.d/authd ]; then  \
		/etc/init.d/authd stop        ; \
		rm -f /etc/init.d/authd       ; \
	fi

