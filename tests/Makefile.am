#TESTS = test-httpd test-tpool

noinst_PROGRAMS = test-tpool test-gzio

test_tpool_SOURCES = test-tpool.c
test_tpool_LDADD   = ../lib/libganglia.la
test_tpool_LDFLAGS = -static

test_gzio_SOURCES = test-gzio.c
test_gzio_LDADD = ../lib/libganglia.la -lz
test_gzio_LDFLAGS = -static
