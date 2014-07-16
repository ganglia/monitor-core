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
#define METS_RECVD_ALL "gmetad_metrics_recvd_all"
#define METS_SENT_ALL "gmetad_metrics_sent_all"
#define METS_SENT_RRDTOOL "gmetad_metrics_sent_rrdtool"
#define METS_SENT_RRDCACHED "gmetad_metrics_sent_rrdcached"
#define METS_SENT_GRAPHITE "gmetad_metrics_sent_graphite"
#define METS_SENT_MEMCACHED "gmetad_metrics_sent_memcached"
#define METS_SENT_RIEMANN "gmetad_metrics_sent_riemann"

/* predefined gmetad scoreboard internal elements */
#define INTER_POLLS_NBR_ALL "gmetad_internal_polls_nbr_all"
#define INTER_POLLS_DUR_ALL "gmetad_internal_polls_dur_all"
#define INTER_POLLS_TIM_ALL "gmetad_internal_polls_tim_all"

#define INTER_EXPORTS_NBR_ALL "gmetad_internal_exports_nbr_all"
#define INTER_EXPORTS_NBR_RRDTOOLS "gmetad_internal_exports_nbr_rrdtools"
#define INTER_EXPORTS_NBR_RRDCACHED "gmetad_internal_exports_nbr_rrdcached"
#define INTER_EXPORTS_NBR_GRAPHITE "gmetad_internal_exports_nbr_graphite"
#define INTER_EXPORTS_NBR_MEMCACHED "gmetad_internal_exports_nbr_memcached"
#define INTER_EXPORTS_NBR_RIEMANN "gmetad_internal_exports_nbr_riemann"

#define INTER_EXPORTS_TIME_EXP_ALL "gmetad_internal_exports_nbr_exp_time_all"
#define INTER_EXPORTS_TIME_EXP_RRDTOOLS "gmetad_internal_exports_nbr_exp_time_rrdtools"
#define INTER_EXPORTS_TIME_EXP_RRDCACHED "gmetad_internal_exports_nbr_exp_time_rrdcached"
#define INTER_EXPORTS_TIME_EXP_GRAPHITE "gmetad_internal_exports_nbr_exp_time_graphite"
#define INTER_EXPORTS_TIME_EXP_MEMCACHED "gmetad_internal_exports_nbr_exp_time_memcached"
#define INTER_EXPORTS_TIME_EXP_RIEMANN "gmetad_internal_exports_nbr_exp_time_riemann"

#define INTER_EXPORTS_LAST_EXP_ALL "gmetad_internal_exports_nbr_last_exp_all"
#define INTER_EXPORTS_LAST_EXP_RRDTOOLS "gmetad_internal_exports_nbr_last_exp_rrdtools"
#define INTER_EXPORTS_LAST_EXP_RRDCACHED "gmetad_internal_exports_nbr_last_exp_rrdcached"
#define INTER_EXPORTS_LAST_EXP_GRAPHITE "gmetad_internal_exports_nbr_last_exp_graphite"
#define INTER_EXPORTS_LAST_EXP_MEMCACHED "gmetad_internal_exports_nbr_last_exp_memcached"
#define INTER_EXPORTS_LAST_EXP_RIEMANN "gmetad_internal_exports_nbr_last_exp_riemann"

#define INTER_REQUESTS_NBR_ALL "gmetad_internal_requests_nbr_all"
#define INTER_REQUESTS_SERV_ALL "gmetad_internal_requests_serv_all"

#define INTER_PROCESSING_SUM_ALL "gmetad_internal_processing_sum_all"
#define INTER_PROCESSING_TIME_SUM_ALL "gmetad_internal_processing_time_sum_all"
#define INTER_PROCESSING_LAST_SUM_ALL "gmetad_internal_processing_last_sum_all"

/*
#define INTER_LATENCY_TIME_ALL "gmetad_internal_latency_time_all"
#define INTER_LATENCY__ALL "gmetad_internal_latency__all"
*/

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
