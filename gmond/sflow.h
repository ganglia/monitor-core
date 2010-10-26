/** @file sflow.h
 *  @brief sFlow definitions for gmond
 *  @author Neil McKee */

#ifndef SFLOW_H
#define SFLOW_H

/* If we use autoconf.  */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "my_inet_ntop.h"
#include "gmond_internal.h"

#define SFLOW_IANA_REGISTERED_PORT 6343

#define SFLOW_VERSION_5 5
#define SFLOW_COUNTERS_SAMPLE 2
#define SFLOW_COUNTERS_SAMPLE_EXPANDED 4
#define SFLOW_DSCLASS_PHYSICAL_ENTITY 2
#define SFLOW_MIN_LEN 36

#define SFLOW_MAX_LOST_SAMPLES 7
#define SFLOW_COUNTERBLOCK_HOST_HID 2000
#define SFLOW_COUNTERBLOCK_HOST_CPU 2003
#define SFLOW_COUNTERBLOCK_HOST_MEM 2004
#define SFLOW_COUNTERBLOCK_HOST_DSK 2005
#define SFLOW_COUNTERBLOCK_HOST_NIO 2006

#define SFLOW_MAX_HOSTNAME_LEN 64
#define SFLOW_MAX_IPSTR_LEN 64
#define SFLOW_MAX_SPOOFHOST_LEN SFLOW_MAX_IPSTR_LEN + SFLOW_MAX_HOSTNAME_LEN + 2
#define SFLOW_MAX_OSRELEASE_LEN 32

typedef struct _SFlowAddr {
  enum { SFLOW_ADDRTYPE_undefined=0, SFLOW_ADDRTYPE_IP4, SFLOW_ADDRTYPE_IP6 } type;
  union {
    uint32_t ip4;
    uint32_t ip6[4];
  } a;
} SFlowAddr;

typedef struct _SFlowXDR {
  /* cursor */
  uint32_t *datap;
  uint32_t i;
  uint32_t quads;
  /* agent */
  SFlowAddr agentAddr;
  uint32_t agentSubId;
  uint32_t uptime_mS;
  /* datasource */
  uint32_t dsClass;
  uint32_t dsIndex;
  /* sequence numbers */
  uint32_t datagramSeqNo;
  uint32_t csSeqNo;
} SFlowXDR;

#define SFLOWXDR_init(x,buf,len) do {  x.datap = (uint32_t *)buf; x.quads = (len >> 2); } while(0)
#define SFLOWXDR_next(x) ntohl(x.datap[x.i++])
#define SFLOWXDR_next_n(x) x.datap[x.i++]
#define SFLOWXDR_more(x,q) ((q + x.i) <= x.quads)
#define SFLOWXDR_skip(x,q) x.i += q
#define SFLOWXDR_skip_b(x,b) x.i += ((b+3)>>2)
#define SFLOWXDR_mark(x,q) x.i + q
#define SFLOWXDR_markOK(x,m) (m == x.i)
#define SFLOWXDR_off_b() (x.i << 2)
#define SFLOWXDR_setc(x,j) x.i = j
#define SFLOWXDR_str(x) (char *)(x.datap + x.i)
#define SFLOWXDR_next_float(x,pf) do { uint32_t tmp=SFLOWXDR_next(x); memcpy(pf, &tmp, 4); } while(0)
#define SFLOWXDR_next_int64(x,pi) do { (*pi) = SFLOWXDR_next(x); (*pi) <<= 32; (*pi) += SFLOWXDR_next(x); } while(0)

typedef struct _SFlowCounterState {
  apr_time_t last_sample_time;
  uint32_t datagramSeqNo;
  uint32_t csSeqNo;
  uint32_t dsIndex;
  /* strings */
  char *osrelease;
  /* CPU */
  uint32_t cpu_user;
  uint32_t cpu_nice;
  uint32_t cpu_system;
  uint32_t cpu_idle;
  uint32_t cpu_wio;
  uint32_t cpu_intr;
  uint32_t cpu_sintr;
  /* NetworkIO */
  uint64_t bytes_in;
  uint32_t pkts_in;
  uint64_t bytes_out;
  uint32_t pkts_out;
} SFlowCounterState;

typedef enum {
#define SFLOW_GMETRIC(tag,mname,units,slope,format,group,desc,title) tag,
#include "sflow_gmetric.h"
#undef SFLOW_GMETRIC
  SFLOW_NUM_GMETRICS } EnumSFLOWGMetric;

typedef struct _SFLOWGMetric {
  EnumSFLOWGMetric tag;
  char *mname;
  char *units;
  ganglia_slope_t slope;
  char *format;
  char *group;
  char *desc;
  char *title;
} SFLOWGMetric;

static const SFLOWGMetric SFLOWGMetricTable[] = {
#define SFLOW_GMETRIC(tag,mname,units,slope,format,group,desc,title) {tag,mname,units,slope,format,group,desc,title},
#include "sflow_gmetric.h"
#undef SFLOW_GMETRIC
};

bool_t process_sflow_datagram(apr_sockaddr_t *remotesa, char *buf, apr_size_t len, apr_time_t now, char **errorMsg);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* SFLOW_H */
