AM_CFLAGS = -I$(top_builddir)/lib -I$(top_builddir)/gmond 

bin_PROGRAMS = gmetric
gmetric_SOURCES =  gmetric.c cmdline.c cmdline.h
gmetric_LDADD   =  $(top_builddir)/lib/libganglia.la $(top_builddir)/lib/libgetopthelper.a
gmetric_LDFLAGS = -static
