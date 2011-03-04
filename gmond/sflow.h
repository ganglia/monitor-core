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
#define SFLOW_COUNTERBLOCK_HOST_PARENT 2002
#define SFLOW_COUNTERBLOCK_HOST_CPU 2003
#define SFLOW_COUNTERBLOCK_HOST_MEM 2004
#define SFLOW_COUNTERBLOCK_HOST_DSK 2005
#define SFLOW_COUNTERBLOCK_HOST_NIO 2006
#define SFLOW_COUNTERBLOCK_HOST_VNODE 2100
#define SFLOW_COUNTERBLOCK_HOST_VCPU 2101
#define SFLOW_COUNTERBLOCK_HOST_VMEM 2102
#define SFLOW_COUNTERBLOCK_HOST_VDSK 2103
#define SFLOW_COUNTERBLOCK_HOST_VNIO 2104

#define SFLOW_MAX_HOSTNAME_LEN 64
#define SFLOW_MAX_FQDN_LEN 256
#define SFLOW_MAX_IPSTR_LEN 64
#define SFLOW_MAX_SPOOFHOST_LEN SFLOW_MAX_IPSTR_LEN + SFLOW_MAX_HOSTNAME_LEN + 2
#define SFLOW_MAX_OSRELEASE_LEN 32
#define SFLOW_MAX_UUIDSTR_LEN 37
#define SFLOW_MAX_INT32STR_LEN 10
#define SFLOW_MAX_DSI_LEN (SFLOW_MAX_IPSTR_LEN + SFLOW_MAX_INT32STR_LEN + 1 + SFLOW_MAX_INT32STR_LEN)

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
  /* timestamp */
  apr_time_t now;
  /* agent */
  SFlowAddr agentAddr;
  char agentipstr[SFLOW_MAX_IPSTR_LEN];
  uint32_t agentSubId;
  uint32_t uptime_mS;
  /* socket address */
  apr_sockaddr_t *remotesa;
  /* datasource */
  uint32_t dsClass;
  uint32_t dsIndex;
  /* sequence numbers */
  uint32_t datagramSeqNo;
  uint32_t csSeqNo;
  /* delta accumulation control */
  bool_t counterDeltas;
  /* structure offsets */
  struct {
    uint32_t HID;
    uint32_t PARENT;
    uint32_t CPU;
    uint32_t MEM;
    uint32_t DSK;
    uint32_t NIO;
    uint32_t VNODE;
    uint32_t VCPU;
    uint32_t VMEM;
    uint32_t VDSK;
    uint32_t VNIO;
    uint32_t foundPH;
    uint32_t foundVM;
  } offset;
} SFlowXDR;

#define SFLOWXDR_init(x,buf,len) do {  x->datap = (uint32_t *)buf; x->quads = (len >> 2); } while(0)
#define SFLOWXDR_next(x) ntohl(x->datap[x->i++])
#define SFLOWXDR_next_n(x) x->datap[x->i++]
#define SFLOWXDR_more(x,q) ((q + x->i) <= x->quads)
#define SFLOWXDR_skip(x,q) x->i += q
#define SFLOWXDR_skip_b(x,b) x->i += ((b+3)>>2)
#define SFLOWXDR_mark(x,q) x->i + q
#define SFLOWXDR_markOK(x,m) (m == x->i)
#define SFLOWXDR_off_b() (x->i << 2)
#define SFLOWXDR_setc(x,j) x->i = j
#define SFLOWXDR_str(x) (char *)(x->datap + x->i)
#define SFLOWXDR_next_float(x,pf) do { uint32_t tmp=SFLOWXDR_next(x); memcpy(pf, &tmp, 4); } while(0)
#define SFLOWXDR_next_int64(x,pi) do { (*pi) = SFLOWXDR_next(x); (*pi) <<= 32; (*pi) += SFLOWXDR_next(x); } while(0)

typedef struct _SFlowCounterState {
  apr_time_t last_sample_time;
  uint32_t datagramSeqNo;
  uint32_t csSeqNo;
  uint32_t dsIndex;
  /* strings */
  char *osrelease;
  char *uuidstr;
  char *dsi;
  char *parent_dsi;
  /* CPU */
  uint32_t cpu_user;
  uint32_t cpu_nice;
  uint32_t cpu_system;
  uint32_t cpu_idle;
  uint32_t cpu_wio;
  uint32_t cpu_intr;
  uint32_t cpu_sintr;
  uint32_t interrupts;
  uint32_t contexts;
  /* memory */
  uint32_t page_in;
  uint32_t page_out;
  uint32_t swap_in;
  uint32_t swap_out;
  /* disk I/O */
  uint32_t reads;
  uint64_t bytes_read;
  uint32_t read_time;
  uint32_t writes;
  uint64_t bytes_written;
  uint32_t write_time;
  
  /* NetworkIO */
  uint64_t bytes_in;
  uint32_t pkts_in;
  uint32_t errs_in;
  uint32_t drops_in;
  uint64_t bytes_out;
  uint32_t pkts_out;
  uint32_t errs_out;
  uint32_t drops_out;

  /* VM CPU */
  uint32_t vcpu_mS;

  /* VM Disk */
  uint32_t vreads;
  uint64_t vbytes_read;
  uint32_t vwrites;
  uint64_t vbytes_written;
  uint32_t vdskerrs;
  
  /* VM NetworkIO */
  uint64_t vbytes_in;
  uint32_t vpkts_in;
  uint32_t verrs_in;
  uint32_t vdrops_in;
  uint64_t vbytes_out;
  uint32_t vpkts_out;
  uint32_t verrs_out;
  uint32_t vdrops_out;

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

uint16_t init_sflow(cfg_t *config_file);
bool_t process_sflow_datagram(apr_sockaddr_t *remotesa, char *buf, apr_size_t len, apr_time_t now, char **errorMsg);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* SFLOW_H */
