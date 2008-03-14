#SUFFIXES=.x
#.c.x: rpcgen $<

if STATIC_BUILD
GCFLAGS = 
GLDADD =
GLDFLAGS = -static
else
GCFLAGS = -D_LARGEFILE64_SOURCE
GLDADD = 
GLDFLAGS =
endif

INCLUDES = @APR_INCLUDES@
AM_CFLAGS = -I.. -I. -I$(top_builddir)/libmetrics/ $(GCFLAGS)  

include_HEADERS = ganglia.h scoreboard.h

EXTRA_DIST=protocol.x
protocol_xdr.c: protocol.x
	rpcgen $<

lib_LTLIBRARIES       = libganglia.la
libganglia_la_SOURCES = become_a_nobody.c become_a_nobody.h \
debug_msg.c debug_msg.h daemon_init.c daemon_init.h file.c file.h \
dotconf.c dotconf.h error.c error.h ganglia_priv.h \
ganglia.c hash.c hash.h inetaddr.c llist.c llist.h \
my_inet_ntop.c my_inet_ntop.h net.h rdwr.c rdwr.h readdir.c readdir.h tcp.c \
scoreboard.c protocol_xdr.c protocol.h apr_net.c apr_net.h libgmond.c
libganglia_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) \
	-export-dynamic 
	$(GLDFLAGS)

libganglia_la_LIBADD = $(GLDADD)	

noinst_LIBRARIES = libgetopthelper.a
# A little helper for getopt functions
libgetopthelper_a_SOURCES = getopt1.c getopt.c getopt_init.c getopt.h
