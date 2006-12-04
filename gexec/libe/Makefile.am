AM_CFLAGS			= -O2 -Wall -D_REENTRANT -g
INCLUDES			= -I.

pkgincludedir		= ${prefix}/include/e

lib_LIBRARIES		= libe.a
pkginclude_HEADERS	= barrier.h bitmask.h bytes.h cpa.h e_error.h hash.h \
					  io.h net.h safe.h token_array.h tree.h xmalloc.h 	 \
					  llist.h

libe_a_SOURCES		= barrier.c barrier.h bitmask.c bitmask.h bytes.h 	 \
					  cpa.c cpa.h e_error.c e_error.h hash.c hash.h io.c \
					  io.h net.c net.h safe.c safe.h token_array.c		 \
					  token_array.h tree.c tree.h xmalloc.c xmalloc.h 	 \
					  llist.c llist.h pack.c pack.h anet.h anet.c

EXTRA_DIST			 = libe.spec
SUBDIRS				 = . test
