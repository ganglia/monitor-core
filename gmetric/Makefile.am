if BUILD_LIBDNET
LIBDNET_INCLUDES = -I../libdnet-1.4/include
LIBDNET_LIBS = ../libdnet-1.4/src/libdnet.a
else
LIBDNET_LIBS = -ldnet
endif

AM_CFLAGS = -I../lib -I../gmond $(LIBDNET_INCLUDES) 

bin_PROGRAMS = gmetric
gmetric_SOURCES =  gmetric.c cmdline.c cmdline.h
gmetric_LDADD   =  ../lib/.libs/libganglia.a $(LIBDNET_LIBS) ../gmond/interface.o
