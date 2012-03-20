if STATIC_BUILD
GLDFLAGS = -static
else
GCFLAGS = 
GLDADD = 
GLDFLAGS = 
endif

AM_CFLAGS = -I$(top_builddir)/lib  -I$(top_builddir)/include $(GCFLAGS)

bin_PROGRAMS = gstat
gstat_SOURCES =  gstat.c cmdline.c cmdline.h
gstat_LDADD   = $(top_builddir)/lib/libganglia.la \
                $(top_builddir)/lib/libgetopthelper.a \
                $(GLDADD)

gstat_LDFLAGS = $(GLDFLAGS)

EXTRA_DIST = cmdline.sh

