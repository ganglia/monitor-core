#SUFFIXES=.x
#.c.x: rpcgen $<

if STATIC_BUILD
GCFLAGS = -D_LARGEFILE64_SOURCE
GLDADD =
GLDFLAGS = -static
else
GCFLAGS = -D_LARGEFILE64_SOURCE
GLDADD = 
GLDFLAGS =
endif
if BUILD_SFLOW
GCFLAGS += -DSFLOW
endif
if BUILD_CLOUD
GCFLAGS += -DCLOUD
endif

INCLUDES = @APR_INCLUDES@
AM_CFLAGS = -I.. -I. -I$(top_builddir)/include/ $(GCFLAGS) -DSYSCONFDIR='"$(sysconfdir)"'

include_HEADERS = gm_protocol.h

EXTRA_DIST=gm_protocol.x
gm_protocol_xdr.c gm_protocol.h: gm_protocol.x
	rpcgen gm_protocol.x

lib_LTLIBRARIES       = libganglia.la
libganglia_la_SOURCES = gm_protocol_xdr.c gm_protocol.h \
become_a_nobody.c become_a_nobody.h \
debug_msg.c update_pidfile.c update_pidfile.h file.c \
dotconf.c dotconf.h error_msg.c ganglia_priv.h \
ganglia.c hash.c hash.h inetaddr.c llist.c llist.h \
my_inet_ntop.c my_inet_ntop.h net.h rdwr.c rdwr.h readdir.c readdir.h tcp.c \
scoreboard.c gm_scoreboard.h apr_net.c apr_net.h libgmond.c
if BUILD_CLOUD
libganglia_la_SOURCES += cloud.h cloud.c
endif
libganglia_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) \
	-export-dynamic 
	$(GLDFLAGS)

libganglia_la_LIBADD = $(GLDADD)	

noinst_LIBRARIES = libgetopthelper.a
# A little helper for getopt functions
libgetopthelper_a_SOURCES = getopt1.c getopt.c getopt_init.c getopt.h
