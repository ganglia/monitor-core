lib_LTLIBRARIES = libganglia.la
libganglia_la_SOURCES = daemon_inetd.c daemon_init.c debug_msg.c error.c \
hash.c llist.c my_inet_ntop.c rdwr.c file.c gexec_funcs.c barrier.c \
become_a_nobody.c gmond_config.c inetaddr.c mcast.c tcp.c fsusage.c \
dotconf.c readdir.c get_ifi_info.c unpifi.h unp.h addrinfo.h \
interface.c interface.h wrapunix.c wrapsock.c dotconf.h readdir.h \
addrinfo.h barrier.h become_a_nobody.h daemon_inetd.h daemon_init.h \
debug_msg.h dotconf.h error.h file.h fsusage.h ganglia.h ganglia_private.h \
getopt.h gmond_config.h hash.h interface.h llist.h \
my_inet_ntop.h net.h rdwr.h readdir.h unp.h unpifi.h ganglia_private.h
libganglia_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) \
	-export-dynamic


noinst_LIBRARIES = libgetopthelper.a
# A little helper for getopt functions
libgetopthelper_a_SOURCES = getopt1.c getopt.c getopt_init.c getopt.h

include_HEADERS = ganglia.h 
