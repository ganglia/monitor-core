#SUFFIXES=.x
#.c.x: rpcgen $<

AM_CFLAGS = -I.. -I. -I$(top_builddir)/srclib/expat/lib/ -I$(top_builddir)/srclib/libmetrics/

include_HEADERS = ganglia.h

protocol_xdr.c: protocol.x
	rpcgen $<

lib_LTLIBRARIES      = libganglia.la
libganglia_la_SOURCES = barrier.c barrier.h become_a_nobody.c become_a_nobody.h \
daemon_inetd.c daemon_init.c daemon_init.h debug_msg.c debug_msg.h \
dotconf.c dotconf.h error.c error.h file.c file.h ganglia_priv.h \
ganglia.c hash.c hash.h inetaddr.c llist.c llist.h \
mcast.c my_inet_ntop.c my_inet_ntop.h net.h rdwr.c rdwr.h readdir.c readdir.h tcp.c \
protocol_xdr.c
libganglia_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) \
	-export-dynamic

noinst_LIBRARIES = libgetopthelper.a
# libdnet.a 
# A little helper for getopt functions
libgetopthelper_a_SOURCES = getopt1.c getopt.c getopt_init.c getopt.h
