AM_CFLAGS = -I.. -I. -I./dnet

EXTRA_DIST =  xmltok_impl.c xmltok_ns.c EXPAT gangliaconf.h

lib_LTLIBRARIES      = libganglia.la
libganglia_la_SOURCES = daemon_inetd.c daemon_init.c debug_msg.c error.c \
ganglia.c hash.c llist.c my_inet_ntop.c \
rdwr.c xmlparse.c xmlrole.c xmltok.c file.c gexec_funcs.c barrier.c \
become_a_nobody.c gexec_process.c gexec_vnn_ip.c gmond_config.c inetaddr.c \
mcast.c tcp.c fsusage.c dotconf.c readdir.c

noinst_HEADERS = dotconf.h readdir.h dnet.h dnet/addr.h dnet/arp.h \
dnet/blob.h dnet/eth.h dnet/fw.h dnet/icmp.h dnet/intf.h \
dnet/ip.h dnet/os.h dnet/rand.h dnet/route.h dnet/tcp.h dnet/udp.h

noinst_LIBRARIES = libgetopthelper.a libdnet.a
# A little helper for getopt functions
libgetopthelper_a_SOURCES = getopt1.c getopt.c getopt_init.c getopt.h

# Making a light version of the dnet library (we only need the intf/eth functions)
libdnet_a_SOURCES = addr.c  strlcat.c strlcpy.c
libdnet_a_LIBADD  = @DNET_OBJS@
libdnet_a_DEPENDENCIES = @DNET_OBJS@
EXTRA_libdnet_a_SOURCES = intf.c intf-win32.c \
eth-bsd.c eth-dlpi.c eth-linux.c eth-ndd.c eth-none.c eth-snoop.c eth-win32.c

libganglia_la_LIBADD  = $(DOTCONF_LIBS)

libganglia_la_LDFLAGS = -version-info 2:0:1

include_HEADERS = ganglia.h ganglia/ascii.h ganglia/asciitab.h ganglia/daemon_inetd.h \
ganglia/daemon_init.h ganglia/debug_msg.h ganglia/error.h ganglia/file.h ganglia/hash.h \
ganglia/iasciitab.h ganglia/latin1tab.h ganglia/llist.h ganglia/my_inet_ntop.h \
ganglia/nametab.h ganglia/net.h ganglia/rdwr.h ganglia/utf8tab.h ganglia/xmldef.h \
ganglia/xmlparse.h ganglia/xmlrole.h ganglia/xmltok.h ganglia/xmltok_impl.h ganglia/gexec_funcs.h \
ganglia/gexec_process.h ganglia/gmond_config.h ganglia/fsusage.h

install-exec-hook:
	mkdir -p ${includedir}/ganglia
