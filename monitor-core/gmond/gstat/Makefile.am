AM_CFLAGS = -I$(top_builddir)/lib

bin_PROGRAMS = gstat
gstat_SOURCES =  gstat.c cmdline.c cmdline.h
gstat_LDADD   = $(top_builddir)/lib/libganglia.la $(top_builddir)/lib/libgetopthelper.a -lexpat -lz
gstat_LDFLAGS = -static
