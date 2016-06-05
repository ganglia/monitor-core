TESTS = test-metrics

AM_CFLAGS = -I$(top_builddir)/../include -I$(top_srcdir)

check_PROGRAMS = test-metrics

test_metrics_SOURCES = test-metrics.c
test_metrics_LDADD   = $(top_builddir)/libmetrics.la ../../lib/libganglia.la
test_metrics_LDFLAGS = -static
