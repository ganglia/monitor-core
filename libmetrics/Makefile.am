SUBDIRS=@OS@ . tests
DIST_SUBDIRS= aix cygwin darwin freebsd hpux irix linux netbsd openbsd osf solaris tests

#AM_CFLAGS  = -I$(top_builddir)/lib 

noinst_LTLIBRARIES    = libmetrics.la
libmetrics_la_SOURCES = libmetrics.c libmetrics.h \
debug_msg.h error.c interface.c interface.h \
get_ifi_info.c unpifi.h error.h
libmetrics_la_LIBADD  = @OS@/libmetric25.la
