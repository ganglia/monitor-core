AM_CFLAGS = -I$(top_builddir)/lib  -I$(top_builddir)/srclib/expat/lib/

bin_PROGRAMS = gstat
gstat_SOURCES =  gstat.c cmdline.c cmdline.h
gstat_LDADD   = $(top_builddir)/lib/libganglia.la \
                $(top_builddir)/lib/libgetopthelper.a \
                $(top_builddir)/srclib/expat/lib/libexpat.la
gstat_LDFLAGS = -static
