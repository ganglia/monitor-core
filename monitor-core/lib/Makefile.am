EXTRA_DIST =  xmltok_impl.c xmltok_ns.c EXPAT ganglia_private.h

lib_LTLIBRARIES      = libganglia.la
libganglia_la_SOURCES = daemon_inetd.c daemon_init.c debug_msg.c error.c \
ganglia.c hash.c llist.c my_inet_ntop.c \
rdwr.c xmlparse.c xmlrole.c xmltok.c file.c gexec_funcs.c barrier.c \
become_a_nobody.c gmond_config.c inetaddr.c \
mcast.c tcp.c fsusage.c dotconf.c readdir.c get_ifi_info.c unpifi.h unp.h addrinfo.h \
interface.c interface.h wrapunix.c wrapsock.c dotconf.h readdir.h \
addrinfo.h ascii.h asciitab.h barrier.h become_a_nobody.h daemon_inetd.h daemon_init.h \
debug_msg.h dotconf.h error.h file.h fsusage.h ganglia.h ganglia_private.h getopt.h \
gexec_funcs.h gmond_config.h hash.h iasciitab.h interface.h latin1tab.h llist.h \
my_inet_ntop.h nametab.h net.h rdwr.h readdir.h unp.h unpifi.h utf8tab.h xmldef.h \
xmlparse.h xmlrole.h xmltok.h xmltok_impl.h

libganglia_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) \
	-export-dynamic

noinst_LIBRARIES = libgetopthelper.a
# A little helper for getopt functions
libgetopthelper_a_SOURCES = getopt1.c getopt.c getopt_init.c getopt.h

include_HEADERS = ganglia.h 
