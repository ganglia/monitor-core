#SUFFIXES=.x
#.c.x: rpcgen $<

AM_CFLAGS = -I.. -I. -I$(top_builddir)/srclib/expat/lib/ -I$(top_builddir)/srclib/libmetrics/ -I$(top_builddir)/srclib/apr/include/ -I$(top_builddir)/srclib/apr/include/arch/unix/ -I$(top_builddir)/srclib/confuse/src

include_HEADERS = ganglia.h

EXTRA_DIST=protocol.x
protocol_xdr.c: protocol.x
	rpcgen $<

lib_LTLIBRARIES       = libganglia.la
libganglia_la_SOURCES = become_a_nobody.c become_a_nobody.h \
debug_msg.c debug_msg.h daemon_init.c daemon_init.h file.c file.h \
dotconf.c dotconf.h error.c error.h ganglia_priv.h \
ganglia.c hash.c hash.h inetaddr.c llist.c llist.h \
my_inet_ntop.c my_inet_ntop.h net.h rdwr.c rdwr.h readdir.c readdir.h tcp.c \
protocol_xdr.c protocol.h apr_net.c apr_net.h libgmond.c
libganglia_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) \
	-export-dynamic 
libganglia_la_LIBADD = $(top_builddir)/srclib/apr/libapr-0.la 

noinst_LIBRARIES = libgetopthelper.a
# A little helper for getopt functions
libgetopthelper_a_SOURCES = getopt1.c getopt.c getopt_init.c getopt.h
