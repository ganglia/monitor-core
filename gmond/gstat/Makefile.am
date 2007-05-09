if STATIC_BUILD
GCFLAGS = -I$(top_builddir)/srclib/expat/lib/
GLDADD = $(top_builddir)/srclib/expat/lib/libexpat.la
GLDFLAGS = -static
else
GCFLAGS = 
GLDADD = 
GLDFLAGS = 
endif

AM_CFLAGS = -I$(top_builddir)/lib  $(GCFLAGS)

bin_PROGRAMS = gstat
gstat_SOURCES =  gstat.c cmdline.c cmdline.h
gstat_LDADD   = $(top_builddir)/lib/libganglia.la \
                $(top_builddir)/lib/libgetopthelper.a \
                $(GLDADD)

gstat_LDFLAGS = $(GLDFLAGS)
