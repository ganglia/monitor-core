/** @file gmond_internal.h
 *  @brief expose just enough of gmond internals to allow tightly-coupled receivers (such as sflow.c) to submit metrics
 *  @author Neil McKee */

#ifndef GMOND_INTERNAL_H
#define GMOND_INTERNAL_H

/* If we use autoconf.  */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef SOLARIS
#define fabsf(f) ((float)fabs(f))
#endif
#ifdef _AIX
#ifndef _AIX52
/* _AIX52 is defined on all versions of AIX >= 5.2
   fabsf doesn't exist on versions prior to 5.2 */
#define fabsf(f) ((float)fabs(f))
#endif
#endif

#include <apr.h>
#include <apr_strings.h>
#include <apr_hash.h>
#include <apr_time.h>
#include <apr_pools.h>
#include <apr_poll.h>
#include <apr_network_io.h>
#include <apr_signal.h>       
#include <apr_thread_proc.h>  /* for apr_proc_detach(). no threads used. */
#include <apr_tables.h>
#include <apr_dso.h>
#include <apr_version.h>

/* The "hosts" hash contains values of type "hostdata" */
struct Ganglia_host {
  /* Name of the host */
  char *hostname;
  /* The IP of this host */
  char *ip;
  /* The location of this host */
  char *location;
  /* Timestamp of when the remote host gmond started */
  unsigned int gmond_started;
  /* The pool used to malloc memory for this host */
  apr_pool_t *pool;
  /* A hash containing the full metric data from the host */
  apr_hash_t *metrics;
  /* A hash containing the last data update from the host */
  apr_hash_t *gmetrics;
  /* First heard from */
  apr_time_t first_heard_from;
  /* Last heard from */
  apr_time_t last_heard_from;
#ifdef SFLOW
  struct _SFlowAgent *sflow;
#endif
};
typedef struct Ganglia_host Ganglia_host;

Ganglia_host *Ganglia_host_get( char *remIP, apr_sockaddr_t *sa, Ganglia_metric_id *metric_id);

void Ganglia_metadata_save( Ganglia_host *host, Ganglia_metadata_msg *message );
void Ganglia_value_save( Ganglia_host *host, Ganglia_value_msg *message );
void Ganglia_update_vidals( Ganglia_host *host, Ganglia_value_msg *vmsg);
void Ganglia_metadata_check(Ganglia_host *host, Ganglia_value_msg *vmsg );

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* GMOND_INTERNAL_H */
