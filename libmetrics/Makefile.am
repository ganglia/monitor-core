SUBDIRS = @OS@ tests
DIST_SUBDIRS = aix cygwin darwin dragonfly freebsd hpux irix linux mingw netbsd openbsd osf solaris tests

EXTRA_DIST = bootstrap

AM_CFLAGS = -I../lib -I../include

noinst_LTLIBRARIES    = libmetrics.la
libmetrics_la_SOURCES = libmetrics.c libmetrics.h \
                        interface.c interface.h \
                        get_ifi_info.c unpifi.h
libmetrics_la_LIBADD  = @OS@/libmetric25.la
