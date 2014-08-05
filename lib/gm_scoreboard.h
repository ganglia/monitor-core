#ifndef GM_SCOREBOARD_H
#define GM_SCOREBOARD_H 1

#include <apr_pools.h>

/* Author: Brad Nicholes (bnicholes novell.com) */

enum ganglia_scoreboard_types {
	GSB_UNKNOWN = 0,
	GSB_COUNTER = 1,
	GSB_READ_RESET = 2,
	GSB_STATE = 3
};
typedef enum ganglia_scoreboard_types ganglia_scoreboard_types;

/* predefined gmetad scoreboard elements */
#define METS_RECVD_ALL "metrics_recvd_all"
#define METS_SENT_ALL "metrics_sent_all"
#define METS_SENT_RRDTOOL "metrics_sent_rrdtool"
#define METS_SENT_RRDCACHED "metrics_sent_rrdcached"
#define METS_SENT_GRAPHITE "metrics_sent_graphite"
#define METS_SENT_MEMCACHED "metrics_sent_memcached"
#define METS_SENT_RIEMANN "metrics_sent_riemann"
#define METS_ALL_DURATION "metrics_all_duration"
#define METS_RRDTOOLS_DURATION "metrics_rrdtools_duration"
#define METS_RRDCACHED_DURATION "metrics_rrdcached_duration"
#define METS_GRAPHITE_DURATION "metrics_graphite_duration"
#define METS_MEMCACHED_DURATION "metrics_memcached_duration"
#define METS_RIEMANN_DURATION "metrics_riemann_duration"
#define METS_SUMRZ_ROOT "metrics_summarized_root"
#define METS_SUMRZ_CLUSTER "metrics_summarized_cluster"
#define METS_SUMRZ_DURATION "metrics_summarized_duration"
#define DS_POLL_OK_REQS "datasource_poll_ok_requests"
#define DS_POLL_OK_DURATION "datasource_poll_ok_duration"
#define DS_POLL_FAILED_REQS "datasource_poll_failed_requests"
#define DS_POLL_FAILED_DURATION "datasource_poll_failed_duration"
#define DS_POLL_MISS "datasource_poll_miss"
#define TCP_REQS_ALL "tcp_requests_all"
#define TCP_REQS_ALL_DURATION "tcp_requests_all_duration"
#define TCP_REQS_XML "tcp_requests_xml"
#define TCP_REQS_XML_DURATION "tcp_requests_xml_duration"
#define TCP_REQS_INTXML "tcp_requests_intxml"
#define TCP_REQS_INTXML_DURATION "tcp_requests_intxml_duration"

/* predefined gmond scoreboard elements */
#define PKTS_RECVD_ALL "gmond_pkts_recvd_all"
#define PKTS_RECVD_FAILED "gmond_pkts_recvd_failed"
#define PKTS_RECVD_IGNORED "gmond_pkts_recvd_ignored"
#define PKTS_RECVD_METADATA "gmond_pkts_recvd_metadata"
#define PKTS_RECVD_VALUE "gmond_pkts_recvd_value"
#define PKTS_RECVD_REQUEST "gmond_pkts_recvd_request"
#define PKTS_SENT_ALL "gmond_pkts_sent_all"
#define PKTS_SENT_FAILED "gmond_pkts_sent_failed"
#define PKTS_SENT_METADATA "gmond_pkts_sent_metadata"
#define PKTS_SENT_VALUE "gmond_pkts_sent_value"
#define PKTS_SENT_REQUEST "gmond_pkts_sent_request"
#define PKTS_SENT_FAILED "gmond_pkts_sent_failed"

/* The scoreboard is only enabled when --enable-status is set on configure */
#ifdef GSTATUS
void ganglia_scoreboard_init(apr_pool_t *pool);
void* ganglia_scoreboard_iterator();
char* ganglia_scoreboard_next(void **intr);
void ganglia_scoreboard_add(char *name, ganglia_scoreboard_types type);
int ganglia_scoreboard_get(char *name);
void ganglia_scoreboard_set(char *name, int val);
void ganglia_scoreboard_reset(char *name);
int ganglia_scoreboard_inc(char *name);
int ganglia_scoreboard_incby(char *name, int val);
void ganglia_scoreboard_dec(char *name);
ganglia_scoreboard_types ganglia_scoreboard_type(char *name);
#else
#define ganglia_scoreboard_init(p)
#define ganglia_scoreboard_iterator() (NULL)
#define ganglia_scoreboard_next(i) (NULL)
#define ganglia_scoreboard_add(n,t) 
#define ganglia_scoreboard_get(n) (0)
#define ganglia_scoreboard_set(n,v) 
#define ganglia_scoreboard_reset(n) 
#define ganglia_scoreboard_inc(n)
#define ganglia_scoreboard_incby(n,v)
#define ganglia_scoreboard_dec(n) 
#define ganglia_scoreboard_type(n) (GSB_UNKNOWN)
#endif

#endif /*GM_SCOREBOARD_H*/
