AM_CFLAGS = -I.. -I. -I./dnet

EXTRA_DIST =  xmltok_impl.c xmltok_ns.c EXPAT gangliaconf.h getopt.h

lib_LTLIBRARIES      = libganglia.la
libganglia_la_SOURCES = daemon_inetd.c daemon_init.c debug_msg.c error.c \
ganglia.c getopt1.c getopt.c getopt_init.c hash.c llist.c my_inet_ntop.c \
rdwr.c xmlparse.c xmlrole.c xmltok.c file.c gexec_funcs.c barrier.c \
become_a_nobody.c gexec_process.c gexec_vnn_ip.c gmond_config.c inetaddr.c \
mcast.c tcp.c fsusage.c dotconf.c readdir.c intf.c addr.c strlcat.c strlcpy.c \
eth-bsd.c

# Just for convenience
noinst_HEADERS = dotconf.h readdir.h dnet.h ./dnet/addr.h ./dnet/arp.h \
./dnet/blob.h ./dnet/eth.h ./dnet/fw.h ./dnet/icmp.h ./dnet/intf.h \
./dnet/ip.h ./dnet/os.h ./dnet/rand.h ./dnet/route.h ./dnet/tcp.h ./dnet/udp.h

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
