AM_CFLAGS = -I../lib -I../gmond -I../lib/dnet 

bin_PROGRAMS = gmetric
gmetric_SOURCES =  gmetric.c cmdline.c cmdline.h
gmetric_LDADD   =  ../lib/.libs/libganglia.a ../lib/libdnet.a ../gmond/interface.o
