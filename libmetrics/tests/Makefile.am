TESTS = test-metrics

AM_CFLAGS = -I../../include

check_PROGRAMS = test-metrics

test_metrics_SOURCES = test-metrics.c
test_metrics_LDADD   = $(top_builddir)/libmetrics.la ../../lib/libganglia.la
test_metrics_LDFLAGS = -static
