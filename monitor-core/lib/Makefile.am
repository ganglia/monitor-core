EXTRA_DIST =  xmltok_impl.c xmltok_ns.c EXPAT ganglia_private.h

lib_LTLIBRARIES      = libganglia.la
libganglia_la_SOURCES = daemon_inetd.c daemon_init.c debug_msg.c error.c \
ganglia.c hash.c llist.c my_inet_ntop.c \
rdwr.c xmlparse.c xmlrole.c xmltok.c file.c gexec_funcs.c barrier.c \
become_a_nobody.c gmond_config.c inetaddr.c \
mcast.c tcp.c fsusage.c dotconf.c readdir.c get_ifi_info.c unpifi.h unp.h addrinfo.h

noinst_HEADERS = dotconf.h readdir.h 

noinst_LIBRARIES = libgetopthelper.a
# A little helper for getopt functions
libgetopthelper_a_SOURCES = getopt1.c getopt.c getopt_init.c getopt.h

libganglia_la_LDFLAGS = -version-info 2:0:1

include_HEADERS = ganglia.h 
