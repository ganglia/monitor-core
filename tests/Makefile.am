#TESTS = test-httpd test-tpool

noinst_PROGRAMS = test-httpd test-tpool

test_tpool_SOURCES = test-tpool.c
test_tpool_LDADD   = ../lib/libganglia.la
test_tpool_LDFLAGS = -static

test_httpd_SOURCES = test-httpd.c
test_httpd_LDADD = ../lib/libganglia.la
test_httpd_LDFLAGS = -static
