lib_LTLIBRARIES = libganglia.la
libganglia_la_SOURCES = debug_msg.c \
hash.c llist.c rdwr.c file.c gexec_funcs.c barrier.c \
become_a_nobody.c fsusage.c \
dotconf.c readdir.c interface.c interface.h dotconf.h readdir.h \
barrier.h become_a_nobody.h \
debug_msg.h dotconf.h file.h fsusage.h ganglia.h ganglia_private.h \
getopt.h hash.h interface.h llist.h \
rdwr.h ganglia_private.h zio.c zio.h \
heap.c heap.h tpool.c tpool.h memmanage.c memmanage.h faststack.c faststack.h \
web.c web.h
libganglia_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) \
	-export-dynamic
libganglia_la_LIBADD = $(top_builddir)/libhttpd/libhttpd.la \
	$(top_builddir)/libunp/libunp.la



noinst_LIBRARIES = libgetopthelper.a
# A little helper for getopt functions
libgetopthelper_a_SOURCES = getopt1.c getopt.c getopt_init.c getopt.h

include_HEADERS = ganglia.h 
