/** @file sflow.h
 *  @brief sFlow collector for gmond
 *  @author Neil McKee */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <ganglia.h> /* for the libgmond messaging */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <confuse.h>

#define SFLOW_MEMCACHE_2200 1
#include "sflow.h"

/* the Ganglia host table */
extern apr_hash_t *hosts;

static const char *SFLOWMachineTypes[] = {
  "unknown",
  "other",
  "x86",
  "x86_64",
  "ia64",
  "sparc",
  "alpha",
  "powerpc",
  "m68k",
  "mips",
  "arm",
  "hppa",
  "s390",
};
#define SFLOW_MAX_MACHINE_TYPE 12

static const char *SFLOWOSNames[] = {
  "Unknown",
  "Other",
  "Linux",
  "Windows",
  "Darwin",
  "HP-UX",
  "AIX",
  "DragonflyBSD",
  "FreeBSD",
  "NetBSD",
  "OpenBSD",
  "OSF1",
  "SunOS",
};
#define SFLOW_MAX_OS_NAME 12

static const char *SFLOWVirDomainStateNames[] = {
  "Unknown",
  "Running",
  "Blocked",
  "Paused",
  "Shutdown",
  "Shutoff",
  "Crashed",
};
#define SFLOW_MAX_DOMAINSTATE_NAME 6
static struct {
  bool_t submit_null_int;
  bool_t submit_null_float;
  bool_t submit_null_str;
  bool_t submit_all_physical;
  bool_t submit_all_virtual;
  uint32_t null_int;
  float null_float;
  char *null_str;
  bool_t submit_http;
  bool_t multiple_http;
  bool_t submit_memcache;
  bool_t multiple_memcache;
  bool_t submit_jvm;
  bool_t multiple_jvm;
} sflowCFG = { 0 };

uint16_t
init_sflow(cfg_t *config_file)
{
  uint16_t port = 0;
  cfg_t *cfg = cfg_getsec(config_file, "sflow");
  if(cfg) {
    /* udp_port: set this to use non-standard port */
    port = cfg_getint(cfg, "udp_port");

    /* "accept_all_physical" - FALSE here means we only accept the
     * standard set of metrics that libmetrics currently offers. TRUE
     * means we accept all physical/hypervisor metrics. VM metrics are
     * controlled by another setting.
     */
    sflowCFG.submit_all_physical = TRUE;

    /* "accept_vm_metrics": accept the VM fields too */
    sflowCFG.submit_all_virtual = cfg_getbool(cfg, "accept_vm_metrics");
    /* separate gating for http, memcache and jvm,  but for these you have to
       specify if you expect multiple instances. */
    sflowCFG.submit_http = cfg_getbool(cfg, "accept_http_metrics");
    sflowCFG.multiple_http = cfg_getbool(cfg, "multiple_http_instances");

    sflowCFG.submit_memcache = cfg_getbool(cfg, "accept_memcache_metrics");
    sflowCFG.multiple_memcache = cfg_getbool(cfg, "multiple_memcache_instances");

    sflowCFG.submit_jvm = cfg_getbool(cfg, "accept_jvm_metrics");
    sflowCFG.multiple_jvm = cfg_getbool(cfg, "multiple_jvm_instances");

    /* options to submit 'null' values for metrics that are
     * missing, rather than leaving them out altogether.
     */
    sflowCFG.null_int = 0; 
    sflowCFG.submit_null_int = FALSE;
    sflowCFG.null_float = 0.0;
    sflowCFG.submit_null_float = FALSE;
    sflowCFG.null_str = NULL;
    sflowCFG.submit_null_str = FALSE;
  }
  return port;
}

static u_char bin2hex(int nib) {
  return (nib < 10) ? ('0' + nib) : ('a' - 10 + nib);
}

static int printHex(const char *a, int len, char *buf, int bufLen)
{
  int b = 0;
  int i;
  for(i = 0; i < len; i++) {
    if(b > (bufLen - 2)) return 0;
    u_char byte = a[i];
    buf[b++] = bin2hex(byte >> 4);
    buf[b++] = bin2hex(byte & 0x0f);
  }
  return b;
}

static int printUUID(const char *a, char *buf, int bufLen)
{
  int b = 0;
  b += printHex(a, 4, buf, bufLen);
  buf[b++] = '-';
  b += printHex(a + 4, 2, buf + b, bufLen - b);
  buf[b++] = '-';
  b += printHex(a + 6, 2, buf + b, bufLen - b);
  buf[b++] = '-';
  b += printHex(a + 8, 2, buf + b, bufLen - b);
  buf[b++] = '-';
  b += printHex(a + 10, 6, buf + b, bufLen - b);
  buf[b] = '\0';
  return b;
}

static void keepString(char **pstr, char *latest)
{
  if((*pstr) != NULL && (latest==NULL || strcmp((*pstr), latest))) {
    /* It changed. Free stored copy */
    free(*pstr);
    *pstr = NULL;
  }
  if((*pstr) == NULL && latest != NULL) {
    /* New value. Store copy of it. */
    *pstr = strdup(latest);
  }
}

 
static void
submit_sflow_gmetric(Ganglia_host *hostdata, char *metric_name, char *metric_title, EnumSFLOWGMetric tag, Ganglia_metadata_msg *fmsg, Ganglia_value_msg *vmsg)
{
  /* add the rest of the metadata */
  struct Ganglia_extra_data extra_array[3];
  char *group, *desc;
  Ganglia_metadatadef *gfull = &fmsg->Ganglia_metadata_msg_u.gfull;
  gfull->metric_id.name = metric_name;
  gfull->metric.name = metric_name;
  gfull->metric.units = SFLOWGMetricTable[tag].units;
  gfull->metric.slope = SFLOWGMetricTable[tag].slope;
  gfull->metric.tmax = 60; /* (secs) we expect to get new data at least this often */
  gfull->metric.dmax = 600; /* (secs) OK to delete metric if not updated for this long */

  /* extra metadata */
  group = SFLOWGMetricTable[tag].group;
  desc = SFLOWGMetricTable[tag].desc;
  if(group == NULL) group = "sflow";
  if(metric_title == NULL) metric_title = metric_name;
  if(desc == NULL) desc = metric_title;
  extra_array[0].name = "GROUP";
  extra_array[0].data = group;
  extra_array[1].name = "DESC";
  extra_array[1].data = desc;
  extra_array[2].name = "TITLE";
  extra_array[2].data = metric_title;
  gfull->metric.metadata.metadata_len = 3;
  gfull->metric.metadata.metadata_val = extra_array;
  
  /* submit  - do we need to do all these steps every time? */
  Ganglia_metadata_save(hostdata, fmsg);
  Ganglia_value_save(hostdata, vmsg);
  Ganglia_update_vidals(hostdata, vmsg);
  Ganglia_metadata_check(hostdata, vmsg);
}

static void
set_metric_name_and_title(char **mname, char **mtitle, char *mname_buf, char *mtitle_buf, char *metric_prefix, EnumSFLOWGMetric tag)
{
  *mname =  SFLOWGMetricTable[tag].mname;
  *mtitle =  SFLOWGMetricTable[tag].title;
  if(metric_prefix) {
    snprintf(mname_buf, SFLOW_MAX_METRIC_NAME_LEN, "%s.%s", metric_prefix, *mname);
    *mname = mname_buf;
    snprintf(mtitle_buf, SFLOW_MAX_METRIC_NAME_LEN, "%s: %s", metric_prefix, *mtitle);
    *mtitle = mtitle_buf;
  }
}

static void
submit_sflow_float(Ganglia_host *hostdata, char *metric_prefix, EnumSFLOWGMetric tag, float val, bool_t ok)
{
  Ganglia_metadata_msg fmsg = { 0 };
  Ganglia_value_msg vmsg = { 0 };
  char *mname, *mtitle;
  char mname_buf[SFLOW_MAX_METRIC_NAME_LEN];
  char mtitle_buf[SFLOW_MAX_METRIC_NAME_LEN];
  if(ok || sflowCFG.submit_null_float) {
    set_metric_name_and_title(&mname, &mtitle, mname_buf, mtitle_buf, metric_prefix, tag);
    fmsg.id = vmsg.id = gmetric_float;
    fmsg.Ganglia_metadata_msg_u.gfull.metric.type = "float";
    vmsg.Ganglia_value_msg_u.gf.metric_id.name = mname;
    vmsg.Ganglia_value_msg_u.gf.f = (ok ? val : sflowCFG.null_float);
    vmsg.Ganglia_value_msg_u.gf.fmt = SFLOWGMetricTable[tag].format;
    submit_sflow_gmetric(hostdata, mname, mtitle, tag, &fmsg, &vmsg);
  }
}

static void
submit_sflow_double(Ganglia_host *hostdata, char *metric_prefix, EnumSFLOWGMetric tag, double val, bool_t ok)
{
  Ganglia_metadata_msg fmsg = { 0 };
  Ganglia_value_msg vmsg = { 0 };
  char *mname, *mtitle;
  char mname_buf[SFLOW_MAX_METRIC_NAME_LEN];
  char mtitle_buf[SFLOW_MAX_METRIC_NAME_LEN];
  if(ok || sflowCFG.submit_null_float) {
    set_metric_name_and_title(&mname, &mtitle, mname_buf, mtitle_buf, metric_prefix, tag);
    fmsg.id = vmsg.id = gmetric_double;
    fmsg.Ganglia_metadata_msg_u.gfull.metric.type = "double";
    vmsg.Ganglia_value_msg_u.gd.metric_id.name = mname;
    vmsg.Ganglia_value_msg_u.gd.d = (ok ? val : (double)sflowCFG.null_float);
    vmsg.Ganglia_value_msg_u.gd.fmt = SFLOWGMetricTable[tag].format;
    submit_sflow_gmetric(hostdata, mname, mtitle, tag, &fmsg, &vmsg);
  }
}

static void
submit_sflow_uint16(Ganglia_host *hostdata, char *metric_prefix, EnumSFLOWGMetric tag, uint16_t val, bool_t ok)
{
  Ganglia_metadata_msg fmsg = { 0 };
  Ganglia_value_msg vmsg = { 0 };
  char *mname, *mtitle;
  char mname_buf[SFLOW_MAX_METRIC_NAME_LEN];
  char mtitle_buf[SFLOW_MAX_METRIC_NAME_LEN];
  if(ok || sflowCFG.submit_null_int) {
    set_metric_name_and_title(&mname, &mtitle, mname_buf, mtitle_buf, metric_prefix, tag);
    fmsg.id = vmsg.id = gmetric_ushort;
    fmsg.Ganglia_metadata_msg_u.gfull.metric.type = "uint16";
    vmsg.Ganglia_value_msg_u.gu_short.metric_id.name = mname;
    vmsg.Ganglia_value_msg_u.gu_short.us = (ok ? val : (uint16_t)sflowCFG.null_int);
    vmsg.Ganglia_value_msg_u.gu_short.fmt = SFLOWGMetricTable[tag].format;
    submit_sflow_gmetric(hostdata, mname, mtitle, tag, &fmsg, &vmsg);
  }
}

static void
submit_sflow_uint32(Ganglia_host *hostdata, char *metric_prefix, EnumSFLOWGMetric tag, uint32_t val, bool_t ok)
{
  Ganglia_metadata_msg fmsg = { 0 };
  Ganglia_value_msg vmsg = { 0 };
  char *mname, *mtitle;
  char mname_buf[SFLOW_MAX_METRIC_NAME_LEN];
  char mtitle_buf[SFLOW_MAX_METRIC_NAME_LEN];
  if(ok || sflowCFG.submit_null_int) {
    set_metric_name_and_title(&mname, &mtitle, mname_buf, mtitle_buf, metric_prefix, tag);
    fmsg.id = vmsg.id = gmetric_uint;
    fmsg.Ganglia_metadata_msg_u.gfull.metric.type = "uint32";
    vmsg.Ganglia_value_msg_u.gu_int.metric_id.name = mname;
    vmsg.Ganglia_value_msg_u.gu_int.ui = (ok ? val : sflowCFG.null_int);
    vmsg.Ganglia_value_msg_u.gu_int.fmt = SFLOWGMetricTable[tag].format;
    submit_sflow_gmetric(hostdata, mname, mtitle, tag, &fmsg, &vmsg);
  }
}

static void
submit_sflow_string(Ganglia_host *hostdata, char *metric_prefix, EnumSFLOWGMetric tag, const char *val, bool_t ok)
{
  Ganglia_metadata_msg fmsg = { 0 };
  Ganglia_value_msg vmsg = { 0 };
  char *mname, *mtitle;
  char mname_buf[SFLOW_MAX_METRIC_NAME_LEN];
  char mtitle_buf[SFLOW_MAX_METRIC_NAME_LEN];
  if(ok || sflowCFG.submit_null_str) {
    set_metric_name_and_title(&mname, &mtitle, mname_buf, mtitle_buf, metric_prefix, tag);
    fmsg.id = vmsg.id = gmetric_uint;
    fmsg.Ganglia_metadata_msg_u.gfull.metric.type = "string";
    vmsg.Ganglia_value_msg_u.gstr.metric_id.name = mname;
    vmsg.Ganglia_value_msg_u.gstr.str = (ok ? (char *)val : sflowCFG.null_str);
    vmsg.Ganglia_value_msg_u.gstr.fmt = SFLOWGMetricTable[tag].format;
    submit_sflow_gmetric(hostdata, mname, mtitle, tag, &fmsg, &vmsg);
  }
}

#define SFLOW_MEM_KB(bytes) (float)(bytes / 1024)

  /* convenience macros for handling counters */
#define SFLOW_CTR_LATCH(ds,field) ds->counterState.field = field
#define SFLOW_CTR_DELTA(ds,field) (field - ds->counterState.field)
#define SFLOW_CTR_RATE(ds, field, mS) ((mS) ? ((float)(SFLOW_CTR_DELTA(ds, field)) * (float)1000.0 / (float)mS) : 0)
#define SFLOW_CTR_MS_PC(ds, field, mS) ((mS) ? ((float)(SFLOW_CTR_DELTA(ds, field)) * (float)100.0 / (float)mS) : 0)
#define SFLOW_CTR_DIVIDE(ds, num, denom) (SFLOW_CTR_DELTA(ds, denom) ? (float)(SFLOW_CTR_DELTA(ds, num)) / (float)(SFLOW_CTR_DELTA(ds, denom)) : 0)

/* #define SFLOW_GAUGE_DIVIDE(num, denom) ((denom) ? ((double)(num) / (double)(denom)) : (double)0) */

  /* metrics may be marked as "unsupported" by the sender,  so check for those reserved values */
#define SFLOW_OK_FLOAT(field) (field != (float)-1)
#define SFLOW_OK_GAUGE32(field) (field != (uint32_t)-1)
#define SFLOW_OK_GAUGE64(field) (field != (uint64_t)-1)
#define SFLOW_OK_COUNTER32(field) (field != (uint32_t)-1)
#define SFLOW_OK_COUNTER64(field) (field != (uint64_t)-1)

static void
process_struct_CPU(SFlowXDR *x, SFlowDataSource *dataSource, Ganglia_host *hostdata, char *metric_prefix, apr_time_t ctr_ival_mS)
{
  float load_one,load_five,load_fifteen;
  uint32_t proc_run,proc_total,cpu_num,cpu_speed,cpu_uptime,cpu_user,cpu_nice,cpu_system,cpu_idle,cpu_wio,cpu_intr,cpu_sintr;
  uint32_t interrupts,contexts;
  SFLOWXDR_next_float(x,&load_one);
  SFLOWXDR_next_float(x,&load_five);
  SFLOWXDR_next_float(x,&load_fifteen);
  proc_run   = SFLOWXDR_next(x);
  proc_total = SFLOWXDR_next(x);
  cpu_num    = SFLOWXDR_next(x);
  cpu_speed  = SFLOWXDR_next(x);
  cpu_uptime = SFLOWXDR_next(x);
  cpu_user   = SFLOWXDR_next(x);
  cpu_nice   = SFLOWXDR_next(x);
  cpu_system = SFLOWXDR_next(x);
  cpu_idle   = SFLOWXDR_next(x);
  cpu_wio    = SFLOWXDR_next(x);
  cpu_intr   = SFLOWXDR_next(x);
  cpu_sintr  = SFLOWXDR_next(x);
  interrupts = SFLOWXDR_next(x);
  contexts   = SFLOWXDR_next(x);
  
  submit_sflow_float(hostdata, metric_prefix, SFLOW_M_load_one, load_one, SFLOW_OK_FLOAT(load_one));
  submit_sflow_float(hostdata, metric_prefix, SFLOW_M_load_five, load_five, SFLOW_OK_FLOAT(load_five));
  submit_sflow_float(hostdata, metric_prefix, SFLOW_M_load_fifteen, load_fifteen, SFLOW_OK_FLOAT(load_fifteen));
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_proc_run, proc_run, SFLOW_OK_GAUGE32(proc_run));
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_proc_total, proc_total, SFLOW_OK_GAUGE32(proc_total));
  submit_sflow_uint16(hostdata, metric_prefix, SFLOW_M_cpu_num, cpu_num, SFLOW_OK_GAUGE32(cpu_num));
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_cpu_speed, cpu_speed, SFLOW_OK_GAUGE32(cpu_speed));
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_cpu_boottime, (apr_time_as_msec(x->now) / 1000) - cpu_uptime, SFLOW_OK_GAUGE32(cpu_uptime));
  
  if(x->counterDeltas) {
    uint32_t delta_cpu_user =   SFLOW_CTR_DELTA(dataSource, cpu_user);
    uint32_t delta_cpu_nice =   SFLOW_CTR_DELTA(dataSource, cpu_nice);
    uint32_t delta_cpu_system = SFLOW_CTR_DELTA(dataSource, cpu_system);
    uint32_t delta_cpu_idle =   SFLOW_CTR_DELTA(dataSource, cpu_idle);
    uint32_t delta_cpu_wio =    SFLOW_CTR_DELTA(dataSource, cpu_wio);
    uint32_t delta_cpu_intr =   SFLOW_CTR_DELTA(dataSource, cpu_intr);
    uint32_t delta_cpu_sintr =  SFLOW_CTR_DELTA(dataSource, cpu_sintr);
      
    uint32_t cpu_total = \
      delta_cpu_user +
      delta_cpu_nice +
      delta_cpu_system +
      delta_cpu_idle +
      delta_cpu_wio +
      delta_cpu_intr +
      delta_cpu_sintr;

#define SFLOW_CTR_CPU_PC(field) (cpu_total ? ((float)field * 100.0 / (float)cpu_total) : 0)
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_cpu_user, SFLOW_CTR_CPU_PC(delta_cpu_user), SFLOW_OK_COUNTER32(cpu_user));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_cpu_nice, SFLOW_CTR_CPU_PC(delta_cpu_nice), SFLOW_OK_COUNTER32(cpu_nice));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_cpu_system, SFLOW_CTR_CPU_PC(delta_cpu_system), SFLOW_OK_COUNTER32(cpu_system));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_cpu_idle, SFLOW_CTR_CPU_PC(delta_cpu_idle), SFLOW_OK_COUNTER32(cpu_idle));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_cpu_wio, SFLOW_CTR_CPU_PC(delta_cpu_wio), SFLOW_OK_COUNTER32(cpu_wio));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_cpu_intr, SFLOW_CTR_CPU_PC(delta_cpu_intr), SFLOW_OK_COUNTER32(cpu_intr));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_cpu_sintr, SFLOW_CTR_CPU_PC(delta_cpu_sintr), SFLOW_OK_COUNTER32(cpu_sintr));

    if(sflowCFG.submit_all_physical) {
      submit_sflow_float(hostdata, metric_prefix, SFLOW_M_interrupts, SFLOW_CTR_RATE(dataSource, interrupts, ctr_ival_mS), SFLOW_OK_COUNTER32(interrupts));
      submit_sflow_float(hostdata, metric_prefix, SFLOW_M_contexts, SFLOW_CTR_RATE(dataSource, contexts, ctr_ival_mS), SFLOW_OK_COUNTER32(contexts));
    }
  }

    
  SFLOW_CTR_LATCH(dataSource, cpu_user);
  SFLOW_CTR_LATCH(dataSource, cpu_nice);
  SFLOW_CTR_LATCH(dataSource, cpu_system);
  SFLOW_CTR_LATCH(dataSource, cpu_idle);
  SFLOW_CTR_LATCH(dataSource, cpu_wio);
  SFLOW_CTR_LATCH(dataSource, cpu_intr);
  SFLOW_CTR_LATCH(dataSource, cpu_sintr);
  SFLOW_CTR_LATCH(dataSource, interrupts);
  SFLOW_CTR_LATCH(dataSource, contexts);
}

static void
process_struct_MEM(SFlowXDR *x, SFlowDataSource *dataSource, Ganglia_host *hostdata, char *metric_prefix, apr_time_t ctr_ival_mS)
{
  uint64_t mem_total,mem_free,mem_shared,mem_buffers,mem_cached,swap_total,swap_free;
  uint32_t page_in,page_out,swap_in,swap_out;
  SFLOWXDR_next_int64(x,&mem_total);
  SFLOWXDR_next_int64(x,&mem_free);
  SFLOWXDR_next_int64(x,&mem_shared);
  SFLOWXDR_next_int64(x,&mem_buffers);
  SFLOWXDR_next_int64(x,&mem_cached);
  SFLOWXDR_next_int64(x,&swap_total);
  SFLOWXDR_next_int64(x,&swap_free);
  page_in = SFLOWXDR_next(x);
  page_out = SFLOWXDR_next(x);
  swap_in = SFLOWXDR_next(x);
  swap_out = SFLOWXDR_next(x);
  
  submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mem_total, SFLOW_MEM_KB(mem_total), SFLOW_OK_GAUGE64(mem_total));
  submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mem_free, SFLOW_MEM_KB(mem_free), SFLOW_OK_GAUGE64(mem_free));
  submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mem_shared, SFLOW_MEM_KB(mem_shared), SFLOW_OK_GAUGE64(mem_shared));
  submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mem_buffers, SFLOW_MEM_KB(mem_buffers), SFLOW_OK_GAUGE64(mem_buffers));
  submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mem_cached, SFLOW_MEM_KB(mem_cached), SFLOW_OK_GAUGE64(mem_cached));
  submit_sflow_float(hostdata, metric_prefix, SFLOW_M_swap_total, SFLOW_MEM_KB(swap_total), SFLOW_OK_GAUGE64(swap_total));
  submit_sflow_float(hostdata, metric_prefix, SFLOW_M_swap_free, SFLOW_MEM_KB(swap_free), SFLOW_OK_GAUGE64(swap_free));
  
  if(x->counterDeltas) {
    if(sflowCFG.submit_all_physical) {
      submit_sflow_float(hostdata, metric_prefix, SFLOW_M_page_in, SFLOW_CTR_RATE(dataSource, page_in, ctr_ival_mS), SFLOW_OK_COUNTER32(page_in));
      submit_sflow_float(hostdata, metric_prefix, SFLOW_M_page_out, SFLOW_CTR_RATE(dataSource, page_out, ctr_ival_mS), SFLOW_OK_COUNTER32(page_out));
      submit_sflow_float(hostdata, metric_prefix, SFLOW_M_swap_in, SFLOW_CTR_RATE(dataSource, swap_in, ctr_ival_mS), SFLOW_OK_COUNTER32(swap_in));
      submit_sflow_float(hostdata, metric_prefix, SFLOW_M_swap_out, SFLOW_CTR_RATE(dataSource, swap_out, ctr_ival_mS), SFLOW_OK_COUNTER32(swap_out));
    }
  }
  SFLOW_CTR_LATCH(dataSource, page_in);
  SFLOW_CTR_LATCH(dataSource, page_out);
  SFLOW_CTR_LATCH(dataSource, swap_in);
  SFLOW_CTR_LATCH(dataSource, swap_out);
}

static void
process_struct_DSK(SFlowXDR *x, SFlowDataSource *dataSource, Ganglia_host *hostdata, char *metric_prefix, apr_time_t ctr_ival_mS)
{
  uint64_t disk_total, disk_free;
  uint32_t part_max_used;
  uint32_t reads, read_time, writes, write_time;
  uint64_t bytes_read, bytes_written;
  SFLOWXDR_next_int64(x,&disk_total);
  SFLOWXDR_next_int64(x,&disk_free);
  part_max_used = SFLOWXDR_next(x);
  reads = SFLOWXDR_next(x);
  SFLOWXDR_next_int64(x, &bytes_read);
  read_time = SFLOWXDR_next(x);
  writes = SFLOWXDR_next(x);
  SFLOWXDR_next_int64(x, &bytes_written);
  write_time = SFLOWXDR_next(x);

  /* convert bytes to GB (1024*1024*1024=1073741824) */
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_disk_total, (double)disk_total / 1073741824.0, SFLOW_OK_GAUGE64(disk_total));
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_disk_free, (double)disk_free / 1073741824.0, SFLOW_OK_GAUGE64(disk_free));
  submit_sflow_float(hostdata, metric_prefix, SFLOW_M_part_max_used, (float)part_max_used / 100.0, SFLOW_OK_GAUGE32(part_max_used));

  if(x->counterDeltas) {
    if(sflowCFG.submit_all_physical) {
      submit_sflow_float(hostdata, metric_prefix, SFLOW_M_reads, SFLOW_CTR_RATE(dataSource, reads, ctr_ival_mS), SFLOW_OK_COUNTER32(reads));
      submit_sflow_float(hostdata, metric_prefix, SFLOW_M_bytes_read, SFLOW_CTR_RATE(dataSource, bytes_read, ctr_ival_mS), SFLOW_OK_COUNTER64(bytes_read));
      submit_sflow_float(hostdata, metric_prefix, SFLOW_M_read_time, SFLOW_CTR_DIVIDE(dataSource, read_time, reads), SFLOW_OK_COUNTER32(read_time));
      submit_sflow_float(hostdata, metric_prefix, SFLOW_M_writes, SFLOW_CTR_RATE(dataSource, writes, ctr_ival_mS), SFLOW_OK_COUNTER32(writes));
      submit_sflow_float(hostdata, metric_prefix, SFLOW_M_bytes_written, SFLOW_CTR_RATE(dataSource, bytes_written, ctr_ival_mS), SFLOW_OK_COUNTER64(bytes_written));
      submit_sflow_float(hostdata, metric_prefix, SFLOW_M_write_time, SFLOW_CTR_DIVIDE(dataSource, write_time, writes), SFLOW_OK_COUNTER32(write_time));
    }
  }

  SFLOW_CTR_LATCH(dataSource, reads);
  SFLOW_CTR_LATCH(dataSource, bytes_read);
  SFLOW_CTR_LATCH(dataSource, read_time);
  SFLOW_CTR_LATCH(dataSource, writes);
  SFLOW_CTR_LATCH(dataSource, bytes_written);
  SFLOW_CTR_LATCH(dataSource, write_time);
}

static void
process_struct_NIO(SFlowXDR *x, SFlowDataSource *dataSource, Ganglia_host *hostdata, char *metric_prefix, apr_time_t ctr_ival_mS)
{
  uint64_t bytes_in, bytes_out;
  uint32_t pkts_in, pkts_out;
  uint32_t errs_in, errs_out;
  uint32_t drops_in, drops_out;

  SFLOWXDR_next_int64(x,&bytes_in);
  pkts_in =  SFLOWXDR_next(x);
  errs_in = SFLOWXDR_next(x);
  drops_in = SFLOWXDR_next(x);
  SFLOWXDR_next_int64(x,&bytes_out);
  pkts_out = SFLOWXDR_next(x);
  errs_out = SFLOWXDR_next(x);
  drops_out = SFLOWXDR_next(x);

    
  if(x->counterDeltas) {
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_bytes_in, SFLOW_CTR_RATE(dataSource, bytes_in, ctr_ival_mS), SFLOW_OK_COUNTER64(bytes_in));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_pkts_in, SFLOW_CTR_RATE(dataSource, pkts_in, ctr_ival_mS), SFLOW_OK_COUNTER32(pkts_in));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_bytes_out, SFLOW_CTR_RATE(dataSource, bytes_out, ctr_ival_mS), SFLOW_OK_COUNTER64(bytes_out));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_pkts_out, SFLOW_CTR_RATE(dataSource, pkts_out, ctr_ival_mS), SFLOW_OK_COUNTER32(pkts_out));
    if(sflowCFG.submit_all_physical) {
      submit_sflow_float(hostdata, metric_prefix, SFLOW_M_errs_in, SFLOW_CTR_RATE(dataSource, errs_in, ctr_ival_mS), SFLOW_OK_COUNTER32(errs_in));
      submit_sflow_float(hostdata, metric_prefix, SFLOW_M_drops_in, SFLOW_CTR_RATE(dataSource, drops_in, ctr_ival_mS), SFLOW_OK_COUNTER32(drops_in));
      submit_sflow_float(hostdata, metric_prefix, SFLOW_M_errs_out, SFLOW_CTR_RATE(dataSource, errs_out, ctr_ival_mS), SFLOW_OK_COUNTER32(errs_out));
      submit_sflow_float(hostdata, metric_prefix, SFLOW_M_drops_out, SFLOW_CTR_RATE(dataSource, drops_out, ctr_ival_mS), SFLOW_OK_COUNTER32(drops_out));
    }
  }
    
  SFLOW_CTR_LATCH(dataSource, bytes_in);
  SFLOW_CTR_LATCH(dataSource, pkts_in);
  SFLOW_CTR_LATCH(dataSource, errs_in);
  SFLOW_CTR_LATCH(dataSource, drops_in);
  SFLOW_CTR_LATCH(dataSource, bytes_out);
  SFLOW_CTR_LATCH(dataSource, pkts_out);
  SFLOW_CTR_LATCH(dataSource, errs_out);
  SFLOW_CTR_LATCH(dataSource, drops_out);
}

static void
process_struct_VNODE(SFlowXDR *x, SFlowDataSource *dataSource, Ganglia_host *hostdata, char *metric_prefix, apr_time_t ctr_ival_mS)
{
  uint32_t mhz, cpus, domains;
  uint64_t memory, memory_free;
  mhz =  SFLOWXDR_next(x);
  cpus = SFLOWXDR_next(x);
  SFLOWXDR_next_int64(x,&memory);
  SFLOWXDR_next_int64(x,&memory_free);
  domains = SFLOWXDR_next(x);

  if(sflowCFG.submit_all_physical) {
    submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_vnode_cpu_speed, mhz, SFLOW_OK_GAUGE32(mhz));
    submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_vnode_cpu_num, cpus, SFLOW_OK_GAUGE32(cpus));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_vnode_mem_total, SFLOW_MEM_KB(memory), SFLOW_OK_GAUGE64(memory));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_vnode_mem_free, SFLOW_MEM_KB(memory_free), SFLOW_OK_GAUGE64(memory_free));
    submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_vnode_domains, domains, SFLOW_OK_GAUGE32(domains));
  }
}

static void
process_struct_VCPU(SFlowXDR *x, SFlowDataSource *dataSource, Ganglia_host *hostdata, char *metric_prefix, apr_time_t ctr_ival_mS)
{
  uint32_t vstate, vcpu_mS, vcpus;
  vstate =  SFLOWXDR_next(x);
  vcpu_mS = SFLOWXDR_next(x);
  vcpus = SFLOWXDR_next(x);

  if(vstate <= SFLOW_MAX_DOMAINSTATE_NAME) {
    submit_sflow_string(hostdata, metric_prefix, SFLOW_M_vcpu_state, SFLOWVirDomainStateNames[vstate], TRUE);
  }
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_vcpu_num, vcpus, SFLOW_OK_GAUGE32(vcpus));
  
  if(x->counterDeltas) {
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_vcpu_util, SFLOW_CTR_MS_PC(dataSource, vcpu_mS, ctr_ival_mS), SFLOW_OK_COUNTER32(vcpu_mS));
  }
  SFLOW_CTR_LATCH(dataSource, vcpu_mS);
}

// VCPU structure can also be send by a JVM agent - but we want to map it into different Ganglia metrics
static void
process_struct_JVM_VCPU(SFlowXDR *x, SFlowDataSource *dataSource, Ganglia_host *hostdata, char *metric_prefix, apr_time_t ctr_ival_mS)
{
  uint32_t jvm_vstate, jvm_vcpu_mS, jvm_vcpus;
  jvm_vstate =  SFLOWXDR_next(x);
  jvm_vcpu_mS = SFLOWXDR_next(x);
  jvm_vcpus = SFLOWXDR_next(x);
  // only take the vcpu_mS
  if(x->counterDeltas) {
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_jvm_vcpu_util, SFLOW_CTR_MS_PC(dataSource, jvm_vcpu_mS, ctr_ival_mS), SFLOW_OK_COUNTER32(jvm_vcpu_mS));
  }
  SFLOW_CTR_LATCH(dataSource, jvm_vcpu_mS);
}

static void
process_struct_VMEM(SFlowXDR *x, SFlowDataSource *dataSource, Ganglia_host *hostdata, char *metric_prefix, apr_time_t ctr_ival_mS)
{
  uint64_t memory, memory_max;
  SFLOWXDR_next_int64(x,&memory);
  SFLOWXDR_next_int64(x,&memory_max);

  submit_sflow_float(hostdata, metric_prefix, SFLOW_M_vmem_total, SFLOW_MEM_KB(memory_max), SFLOW_OK_GAUGE64(memory_max));
  if(memory_max > 0) {
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_vmem_util, (float)memory * (float)100.0 / (float)memory_max, SFLOW_OK_GAUGE64(memory));
  }
}

// VMEM structure can also be send by a JVM agent - but we want to map it into different Ganglia metrics
static void
process_struct_JVM_VMEM(SFlowXDR *x, SFlowDataSource *dataSource, Ganglia_host *hostdata, char *metric_prefix, apr_time_t ctr_ival_mS)
{
  uint64_t memory, memory_max;
  SFLOWXDR_next_int64(x,&memory);
  SFLOWXDR_next_int64(x,&memory_max);

  submit_sflow_float(hostdata, metric_prefix, SFLOW_M_jvm_vmem_total, SFLOW_MEM_KB(memory_max), SFLOW_OK_GAUGE64(memory_max));
  if(memory_max > 0) {
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_jvm_vmem_util, (float)memory * (float)100.0 / (float)memory_max, SFLOW_OK_GAUGE64(memory));
  }
}

static void
process_struct_VDSK(SFlowXDR *x, SFlowDataSource *dataSource, Ganglia_host *hostdata, char *metric_prefix, apr_time_t ctr_ival_mS)
{
  uint64_t disk_capacity, disk_allocation, disk_free;
  uint32_t vreads, vwrites;
  uint64_t vbytes_read, vbytes_written;
  uint32_t vdskerrs;
  SFLOWXDR_next_int64(x,&disk_capacity);
  SFLOWXDR_next_int64(x,&disk_allocation);
  SFLOWXDR_next_int64(x,&disk_free);
  vreads = SFLOWXDR_next(x);
  SFLOWXDR_next_int64(x, &vbytes_read);
  vwrites = SFLOWXDR_next(x);
  SFLOWXDR_next_int64(x, &vbytes_written);
  vdskerrs = SFLOWXDR_next(x);

  /* convert bytes to GB (1024*1024*1024=1073741824) */
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_vdisk_capacity, (double)disk_capacity / 1073741824.0, SFLOW_OK_GAUGE64(disk_capacity));
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_vdisk_total, (double)disk_allocation / 1073741824.0, SFLOW_OK_GAUGE64(disk_allocation));
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_vdisk_free, (double)disk_free / 1073741824.0, SFLOW_OK_GAUGE64(disk_free));

  if(x->counterDeltas) {
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_vdisk_reads, SFLOW_CTR_RATE(dataSource, vreads, ctr_ival_mS), SFLOW_OK_COUNTER32(vreads));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_vdisk_bytes_read, SFLOW_CTR_RATE(dataSource, vbytes_read, ctr_ival_mS), SFLOW_OK_COUNTER64(vbytes_read));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_vdisk_writes, SFLOW_CTR_RATE(dataSource, vwrites, ctr_ival_mS), SFLOW_OK_COUNTER32(vwrites));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_vdisk_bytes_written, SFLOW_CTR_RATE(dataSource, vbytes_written, ctr_ival_mS), SFLOW_OK_COUNTER64(vbytes_written));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_vdisk_errs, SFLOW_CTR_RATE(dataSource, vdskerrs, ctr_ival_mS), SFLOW_OK_COUNTER32(vdskerrs));
  }

  SFLOW_CTR_LATCH(dataSource, vreads);
  SFLOW_CTR_LATCH(dataSource, vbytes_read);
  SFLOW_CTR_LATCH(dataSource, vwrites);
  SFLOW_CTR_LATCH(dataSource, vbytes_written);
  SFLOW_CTR_LATCH(dataSource, vdskerrs);
}

static void
process_struct_VNIO(SFlowXDR *x, SFlowDataSource *dataSource, Ganglia_host *hostdata, char *metric_prefix, apr_time_t ctr_ival_mS)
{
  uint64_t vbytes_in, vbytes_out;
  uint32_t vpkts_in, vpkts_out;
  uint32_t verrs_in, verrs_out;
  uint32_t vdrops_in, vdrops_out;
  SFLOWXDR_next_int64(x,&vbytes_in);
  vpkts_in =  SFLOWXDR_next(x);
  verrs_in = SFLOWXDR_next(x);
  vdrops_in = SFLOWXDR_next(x);
  SFLOWXDR_next_int64(x,&vbytes_out);
  vpkts_out = SFLOWXDR_next(x);
  verrs_out = SFLOWXDR_next(x);
  vdrops_out = SFLOWXDR_next(x);

    
  if(x->counterDeltas) {
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_vbytes_in, SFLOW_CTR_RATE(dataSource, vbytes_in, ctr_ival_mS), SFLOW_OK_COUNTER64(vbytes_in));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_vpkts_in, SFLOW_CTR_RATE(dataSource, vpkts_in, ctr_ival_mS), SFLOW_OK_COUNTER32(vpkts_in));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_vbytes_out, SFLOW_CTR_RATE(dataSource, vbytes_out, ctr_ival_mS), SFLOW_OK_COUNTER64(vbytes_out));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_vpkts_out, SFLOW_CTR_RATE(dataSource, vpkts_out, ctr_ival_mS), SFLOW_OK_COUNTER32(vpkts_out));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_verrs_in, SFLOW_CTR_RATE(dataSource, verrs_in, ctr_ival_mS), SFLOW_OK_COUNTER32(verrs_in));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_vdrops_in, SFLOW_CTR_RATE(dataSource, vdrops_in, ctr_ival_mS), SFLOW_OK_COUNTER32(vdrops_in));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_verrs_out, SFLOW_CTR_RATE(dataSource, verrs_out, ctr_ival_mS), SFLOW_OK_COUNTER32(verrs_out));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_vdrops_out, SFLOW_CTR_RATE(dataSource, vdrops_out, ctr_ival_mS), SFLOW_OK_COUNTER32(vdrops_out));
  }
    
  SFLOW_CTR_LATCH(dataSource, vbytes_in);
  SFLOW_CTR_LATCH(dataSource, vpkts_in);
  SFLOW_CTR_LATCH(dataSource, verrs_in);
  SFLOW_CTR_LATCH(dataSource, vdrops_in);
  SFLOW_CTR_LATCH(dataSource, vbytes_out);
  SFLOW_CTR_LATCH(dataSource, vpkts_out);
  SFLOW_CTR_LATCH(dataSource, verrs_out);
  SFLOW_CTR_LATCH(dataSource, vdrops_out);
}

#ifdef SFLOW_MEMCACHE_2200
static void
process_struct_MEMCACHE_2200(SFlowXDR *x, SFlowDataSource *dataSource, Ganglia_host *hostdata, char *metric_prefix, apr_time_t ctr_ival_mS)
{
  uint32_t mc_uptime,mc_rusage_user,mc_rusage_system,mc_curr_conns,mc_total_conns,mc_conn_structs,mc_cmd_get,mc_cmd_set,mc_cmd_flush,mc_get_hits,mc_get_misses,mc_delete_misses,mc_delete_hits,mc_incr_misses,mc_incr_hits,mc_decr_misses,mc_decr_hits,mc_cas_misses,mc_cas_hits,mc_cas_badval,mc_auth_cmds,mc_auth_errors,mc_limit_maxbytes,mc_accepting_conns,mc_listen_disabled_num,mc_threads,mc_conn_yields,mc_curr_items,mc_total_items,mc_evictions;
  uint64_t mc_bytes_read,mc_bytes_written,mc_bytes;

  debug_msg("process_struct_MEMCACHE");

  mc_uptime = SFLOWXDR_next(x);
  mc_rusage_user = SFLOWXDR_next(x);
  mc_rusage_system = SFLOWXDR_next(x);
  mc_curr_conns = SFLOWXDR_next(x);
  mc_total_conns = SFLOWXDR_next(x);
  mc_conn_structs = SFLOWXDR_next(x);
  mc_cmd_get = SFLOWXDR_next(x);
  mc_cmd_set = SFLOWXDR_next(x);
  mc_cmd_flush = SFLOWXDR_next(x);
  mc_get_hits = SFLOWXDR_next(x);
  mc_get_misses = SFLOWXDR_next(x);
  mc_delete_misses = SFLOWXDR_next(x);
  mc_delete_hits = SFLOWXDR_next(x);
  mc_incr_misses = SFLOWXDR_next(x);
  mc_incr_hits = SFLOWXDR_next(x);
  mc_decr_misses = SFLOWXDR_next(x);
  mc_decr_hits = SFLOWXDR_next(x);
  mc_cas_misses = SFLOWXDR_next(x);
  mc_cas_hits = SFLOWXDR_next(x);
  mc_cas_badval = SFLOWXDR_next(x);
  mc_auth_cmds = SFLOWXDR_next(x);
  mc_auth_errors = SFLOWXDR_next(x);
  SFLOWXDR_next_int64(x,&mc_bytes_read);
  SFLOWXDR_next_int64(x,&mc_bytes_written);
  mc_limit_maxbytes = SFLOWXDR_next(x);
  mc_accepting_conns = SFLOWXDR_next(x);
  mc_listen_disabled_num = SFLOWXDR_next(x);
  mc_threads = SFLOWXDR_next(x);
  mc_conn_yields = SFLOWXDR_next(x);
  SFLOWXDR_next_int64(x,&mc_bytes);
  mc_curr_items = SFLOWXDR_next(x);
  mc_total_items = SFLOWXDR_next(x);
  mc_evictions = SFLOWXDR_next(x);
    
  if(x->counterDeltas) {
    debug_msg("process_struct_MEMCACHE - accumulate counters");
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_rusage_user, SFLOW_CTR_RATE(dataSource, mc_rusage_user, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_rusage_user));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_rusage_system, SFLOW_CTR_RATE(dataSource, mc_rusage_system, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_rusage_system));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_total_conns, SFLOW_CTR_RATE(dataSource, mc_total_conns, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_total_conns));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_cmd_get, SFLOW_CTR_RATE(dataSource, mc_cmd_get, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_cmd_get));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_cmd_set, SFLOW_CTR_RATE(dataSource, mc_cmd_set, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_cmd_set));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_cmd_flush, SFLOW_CTR_RATE(dataSource, mc_cmd_flush, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_cmd_flush));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_get_hits, SFLOW_CTR_RATE(dataSource, mc_get_hits, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_get_hits));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_get_misses, SFLOW_CTR_RATE(dataSource, mc_get_misses, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_get_misses));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_delete_misses, SFLOW_CTR_RATE(dataSource, mc_delete_misses, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_delete_misses));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_delete_hits, SFLOW_CTR_RATE(dataSource, mc_delete_hits, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_delete_hits));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_incr_misses, SFLOW_CTR_RATE(dataSource, mc_incr_misses, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_incr_misses));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_incr_hits, SFLOW_CTR_RATE(dataSource, mc_incr_hits, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_incr_hits));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_decr_misses, SFLOW_CTR_RATE(dataSource, mc_decr_misses, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_decr_misses));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_decr_hits, SFLOW_CTR_RATE(dataSource, mc_decr_hits, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_decr_hits));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_cas_misses, SFLOW_CTR_RATE(dataSource, mc_cas_misses, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_cas_misses));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_cas_hits, SFLOW_CTR_RATE(dataSource, mc_cas_hits, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_cas_hits));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_cas_badval, SFLOW_CTR_RATE(dataSource, mc_cas_badval, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_cas_badval));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_auth_cmds, SFLOW_CTR_RATE(dataSource, mc_auth_cmds, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_auth_cmds));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_auth_errors, SFLOW_CTR_RATE(dataSource, mc_auth_errors, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_auth_errors));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_bytes_read, SFLOW_CTR_RATE(dataSource, mc_bytes_read, ctr_ival_mS), SFLOW_OK_COUNTER64(mc_bytes_read));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_bytes_written, SFLOW_CTR_RATE(dataSource, mc_bytes_written, ctr_ival_mS), SFLOW_OK_COUNTER64(mc_bytes_written));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_conn_yields, SFLOW_CTR_RATE(dataSource, mc_conn_yields, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_conn_yields));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_total_items, SFLOW_CTR_RATE(dataSource, mc_total_items, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_total_items));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_evictions, SFLOW_CTR_RATE(dataSource, mc_evictions, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_evictions));
  }


  SFLOW_CTR_LATCH(dataSource, mc_rusage_user);
  SFLOW_CTR_LATCH(dataSource, mc_rusage_system);
  SFLOW_CTR_LATCH(dataSource, mc_total_conns);
  SFLOW_CTR_LATCH(dataSource, mc_cmd_get);
  SFLOW_CTR_LATCH(dataSource, mc_cmd_set);
  SFLOW_CTR_LATCH(dataSource, mc_cmd_flush);
  SFLOW_CTR_LATCH(dataSource, mc_get_hits);
  SFLOW_CTR_LATCH(dataSource, mc_get_misses);
  SFLOW_CTR_LATCH(dataSource, mc_delete_misses);
  SFLOW_CTR_LATCH(dataSource, mc_delete_hits);
  SFLOW_CTR_LATCH(dataSource, mc_incr_misses);
  SFLOW_CTR_LATCH(dataSource, mc_incr_hits);
  SFLOW_CTR_LATCH(dataSource, mc_decr_misses);
  SFLOW_CTR_LATCH(dataSource, mc_decr_hits);
  SFLOW_CTR_LATCH(dataSource, mc_cas_misses);
  SFLOW_CTR_LATCH(dataSource, mc_cas_hits);
  SFLOW_CTR_LATCH(dataSource, mc_cas_badval);
  SFLOW_CTR_LATCH(dataSource, mc_auth_cmds);
  SFLOW_CTR_LATCH(dataSource, mc_auth_errors);
  SFLOW_CTR_LATCH(dataSource, mc_bytes_read);
  SFLOW_CTR_LATCH(dataSource, mc_bytes_written);
  SFLOW_CTR_LATCH(dataSource, mc_conn_yields);
  SFLOW_CTR_LATCH(dataSource, mc_total_items);
  SFLOW_CTR_LATCH(dataSource, mc_evictions);

  /* and the GAUGES too */
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_mc_boottime, (apr_time_as_msec(x->now) / 1000) - mc_uptime, SFLOW_OK_GAUGE32(mc_uptime));
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_mc_curr_conns, mc_curr_conns, SFLOW_OK_GAUGE32(mc_curr_conns));
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_mc_curr_items, mc_curr_items, SFLOW_OK_GAUGE32(mc_curr_items));
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_mc_limit_maxbytes, (double)mc_limit_maxbytes, SFLOW_OK_GAUGE32(mc_limit_maxbytes));
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_mc_threads, mc_threads, SFLOW_OK_GAUGE32(mc_threads));
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_mc_conn_structs, mc_conn_structs, SFLOW_OK_GAUGE32(mc_conn_structs));
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_mc_accepting_conns, mc_accepting_conns, SFLOW_OK_GAUGE32(mc_accepting_conns));
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_mc_listen_disabled_num, mc_listen_disabled_num, SFLOW_OK_GAUGE32(mc_listen_disabled_num));
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_mc_bytes, (double)mc_bytes, SFLOW_OK_GAUGE64(mc_bytes));
}
#endif /* SFLOW_MEMCACHE_2200 */

static void
process_struct_MEMCACHE(SFlowXDR *x, SFlowDataSource *dataSource, Ganglia_host *hostdata, char *metric_prefix, apr_time_t ctr_ival_mS)
{
  uint32_t /* mc_uptime,mc_rusage_user,mc_rusage_system, */ mc_curr_conns,mc_total_conns,mc_conn_structs/* ,mc_cmd_get */,mc_cmd_set,mc_cmd_flush,mc_get_hits,mc_get_misses,mc_delete_misses,mc_delete_hits,mc_incr_misses,mc_incr_hits,mc_decr_misses,mc_decr_hits,mc_cas_misses,mc_cas_hits,mc_cas_badval,mc_auth_cmds,mc_auth_errors,/*mc_limit_maxbytes,mc_accepting_conns,*/ mc_listen_disabled_num,mc_threads,mc_conn_yields,mc_curr_items,mc_total_items,mc_evictions;
  uint64_t mc_bytes_read,mc_bytes_written,mc_bytes;
  /* added in 2204 */
  uint32_t mc_cmd_touch, mc_rejected_conns, mc_reclaimed;
  uint64_t mc_limit_maxbytes;

  debug_msg("process_struct_MEMCACHE");

  mc_cmd_set = SFLOWXDR_next(x);
  mc_cmd_touch = SFLOWXDR_next(x);
  mc_cmd_flush = SFLOWXDR_next(x);
  mc_get_hits = SFLOWXDR_next(x);
  mc_get_misses = SFLOWXDR_next(x);
  mc_delete_hits = SFLOWXDR_next(x);
  mc_delete_misses = SFLOWXDR_next(x);
  mc_incr_hits = SFLOWXDR_next(x);
  mc_incr_misses = SFLOWXDR_next(x);
  mc_decr_hits = SFLOWXDR_next(x);
  mc_decr_misses = SFLOWXDR_next(x);
  mc_cas_hits = SFLOWXDR_next(x);
  mc_cas_misses = SFLOWXDR_next(x);
  mc_cas_badval = SFLOWXDR_next(x);
  mc_auth_cmds = SFLOWXDR_next(x);
  mc_auth_errors = SFLOWXDR_next(x);
  mc_threads = SFLOWXDR_next(x);
  mc_conn_yields = SFLOWXDR_next(x);
  mc_listen_disabled_num = SFLOWXDR_next(x);
  mc_curr_conns = SFLOWXDR_next(x);
  mc_rejected_conns = SFLOWXDR_next(x);
  mc_total_conns = SFLOWXDR_next(x);
  mc_conn_structs = SFLOWXDR_next(x);
  mc_evictions = SFLOWXDR_next(x);
  mc_reclaimed = SFLOWXDR_next(x);
  mc_curr_items = SFLOWXDR_next(x);
  mc_total_items = SFLOWXDR_next(x);
  SFLOWXDR_next_int64(x,&mc_bytes_read);
  SFLOWXDR_next_int64(x,&mc_bytes_written);
  SFLOWXDR_next_int64(x,&mc_bytes);
  SFLOWXDR_next_int64(x,&mc_limit_maxbytes);
    
  if(x->counterDeltas) {
    debug_msg("process_struct_MEMCACHE - accumulate counters");
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_cmd_set, SFLOW_CTR_RATE(dataSource, mc_cmd_set, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_cmd_set));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_cmd_touch, SFLOW_CTR_RATE(dataSource, mc_cmd_touch, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_cmd_touch));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_cmd_flush, SFLOW_CTR_RATE(dataSource, mc_cmd_flush, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_cmd_flush));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_get_hits, SFLOW_CTR_RATE(dataSource, mc_get_hits, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_get_hits));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_get_misses, SFLOW_CTR_RATE(dataSource, mc_get_misses, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_get_misses));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_delete_hits, SFLOW_CTR_RATE(dataSource, mc_delete_hits, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_delete_hits));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_delete_misses, SFLOW_CTR_RATE(dataSource, mc_delete_misses, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_delete_misses));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_incr_hits, SFLOW_CTR_RATE(dataSource, mc_incr_hits, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_incr_hits));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_incr_misses, SFLOW_CTR_RATE(dataSource, mc_incr_misses, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_incr_misses));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_decr_hits, SFLOW_CTR_RATE(dataSource, mc_decr_hits, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_decr_hits));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_decr_misses, SFLOW_CTR_RATE(dataSource, mc_decr_misses, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_decr_misses));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_cas_hits, SFLOW_CTR_RATE(dataSource, mc_cas_hits, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_cas_hits));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_cas_misses, SFLOW_CTR_RATE(dataSource, mc_cas_misses, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_cas_misses));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_cas_badval, SFLOW_CTR_RATE(dataSource, mc_cas_badval, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_cas_badval));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_auth_cmds, SFLOW_CTR_RATE(dataSource, mc_auth_cmds, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_auth_cmds));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_auth_errors, SFLOW_CTR_RATE(dataSource, mc_auth_errors, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_auth_errors));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_conn_yields, SFLOW_CTR_RATE(dataSource, mc_conn_yields, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_conn_yields));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_rejected_conns, SFLOW_CTR_RATE(dataSource, mc_rejected_conns, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_rejected_conns));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_total_conns, SFLOW_CTR_RATE(dataSource, mc_total_conns, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_total_conns));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_evictions, SFLOW_CTR_RATE(dataSource, mc_evictions, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_evictions));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_reclaimed, SFLOW_CTR_RATE(dataSource, mc_reclaimed, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_reclaimed));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_total_items, SFLOW_CTR_RATE(dataSource, mc_total_items, ctr_ival_mS), SFLOW_OK_COUNTER32(mc_total_items));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_bytes_read, SFLOW_CTR_RATE(dataSource, mc_bytes_read, ctr_ival_mS), SFLOW_OK_COUNTER64(mc_bytes_read));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_mc_bytes_written, SFLOW_CTR_RATE(dataSource, mc_bytes_written, ctr_ival_mS), SFLOW_OK_COUNTER64(mc_bytes_written));
  }


  SFLOW_CTR_LATCH(dataSource, mc_cmd_set);
  SFLOW_CTR_LATCH(dataSource, mc_cmd_touch);
  SFLOW_CTR_LATCH(dataSource, mc_cmd_flush);
  SFLOW_CTR_LATCH(dataSource, mc_get_hits);
  SFLOW_CTR_LATCH(dataSource, mc_get_misses);
  SFLOW_CTR_LATCH(dataSource, mc_delete_hits);
  SFLOW_CTR_LATCH(dataSource, mc_delete_misses);
  SFLOW_CTR_LATCH(dataSource, mc_incr_hits);
  SFLOW_CTR_LATCH(dataSource, mc_incr_misses);
  SFLOW_CTR_LATCH(dataSource, mc_decr_hits);
  SFLOW_CTR_LATCH(dataSource, mc_decr_misses);
  SFLOW_CTR_LATCH(dataSource, mc_cas_hits);
  SFLOW_CTR_LATCH(dataSource, mc_cas_misses);
  SFLOW_CTR_LATCH(dataSource, mc_cas_badval);
  SFLOW_CTR_LATCH(dataSource, mc_auth_cmds);
  SFLOW_CTR_LATCH(dataSource, mc_auth_errors);
  SFLOW_CTR_LATCH(dataSource, mc_conn_yields);
  SFLOW_CTR_LATCH(dataSource, mc_rejected_conns);
  SFLOW_CTR_LATCH(dataSource, mc_total_conns);
  SFLOW_CTR_LATCH(dataSource, mc_evictions);
  SFLOW_CTR_LATCH(dataSource, mc_reclaimed);
  SFLOW_CTR_LATCH(dataSource, mc_total_items);
  SFLOW_CTR_LATCH(dataSource, mc_bytes_read);
  SFLOW_CTR_LATCH(dataSource, mc_bytes_written);

  /* and the GAUGES too */
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_mc_threads, mc_threads, SFLOW_OK_GAUGE32(mc_threads));
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_mc_listen_disabled_num, mc_listen_disabled_num, SFLOW_OK_GAUGE32(mc_listen_disabled_num));
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_mc_curr_conns, mc_curr_conns, SFLOW_OK_GAUGE32(mc_curr_conns));
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_mc_curr_items, mc_curr_items, SFLOW_OK_GAUGE32(mc_curr_items));
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_mc_conn_structs, mc_conn_structs, SFLOW_OK_GAUGE32(mc_conn_structs));
  /* there does not seem to be a way to submit a 64-bit integer value, so cast this one to double */
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_mc_limit_maxbytes, (double)mc_limit_maxbytes, SFLOW_OK_GAUGE64(mc_limit_maxbytes));
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_mc_bytes, (double)mc_bytes, SFLOW_OK_GAUGE64(mc_bytes));
}

static void
process_struct_HTTP(SFlowXDR *x, SFlowDataSource *dataSource, Ganglia_host *hostdata, char *metric_prefix, apr_time_t ctr_ival_mS)
{
  uint32_t http_meth_option,http_meth_get,http_meth_head,http_meth_post,http_meth_put,http_meth_delete,http_meth_trace,http_meth_connect,http_meth_other,http_status_1XX,http_status_2XX,http_status_3XX,http_status_4XX,http_status_5XX,http_status_other;

  debug_msg("process_struct_HTTP");

  http_meth_option = SFLOWXDR_next(x);
  http_meth_get = SFLOWXDR_next(x);
  http_meth_head = SFLOWXDR_next(x);
  http_meth_post = SFLOWXDR_next(x);
  http_meth_put = SFLOWXDR_next(x);
  http_meth_delete = SFLOWXDR_next(x);
  http_meth_trace = SFLOWXDR_next(x);
  http_meth_connect = SFLOWXDR_next(x);
  http_meth_other = SFLOWXDR_next(x);
  http_status_1XX = SFLOWXDR_next(x);
  http_status_2XX = SFLOWXDR_next(x);
  http_status_3XX = SFLOWXDR_next(x);
  http_status_4XX = SFLOWXDR_next(x);
  http_status_5XX = SFLOWXDR_next(x);
  http_status_other = SFLOWXDR_next(x);
    
  if(x->counterDeltas) {
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_http_meth_option, SFLOW_CTR_RATE(dataSource, http_meth_option, ctr_ival_mS), SFLOW_OK_COUNTER32(http_meth_option));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_http_meth_get, SFLOW_CTR_RATE(dataSource, http_meth_get, ctr_ival_mS), SFLOW_OK_COUNTER32(http_meth_get));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_http_meth_head, SFLOW_CTR_RATE(dataSource, http_meth_head, ctr_ival_mS), SFLOW_OK_COUNTER32(http_meth_head));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_http_meth_post, SFLOW_CTR_RATE(dataSource, http_meth_post, ctr_ival_mS), SFLOW_OK_COUNTER32(http_meth_post));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_http_meth_put, SFLOW_CTR_RATE(dataSource, http_meth_put, ctr_ival_mS), SFLOW_OK_COUNTER32(http_meth_put));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_http_meth_delete, SFLOW_CTR_RATE(dataSource, http_meth_delete, ctr_ival_mS), SFLOW_OK_COUNTER32(http_meth_delete));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_http_meth_trace, SFLOW_CTR_RATE(dataSource, http_meth_trace, ctr_ival_mS), SFLOW_OK_COUNTER32(http_meth_trace));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_http_meth_connect, SFLOW_CTR_RATE(dataSource, http_meth_connect, ctr_ival_mS), SFLOW_OK_COUNTER32(http_meth_connect));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_http_meth_other, SFLOW_CTR_RATE(dataSource, http_meth_other, ctr_ival_mS), SFLOW_OK_COUNTER32(http_meth_other));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_http_status_1XX, SFLOW_CTR_RATE(dataSource, http_status_1XX, ctr_ival_mS), SFLOW_OK_COUNTER32(http_status_1XX));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_http_status_2XX, SFLOW_CTR_RATE(dataSource, http_status_2XX, ctr_ival_mS), SFLOW_OK_COUNTER32(http_status_2XX));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_http_status_3XX, SFLOW_CTR_RATE(dataSource, http_status_3XX, ctr_ival_mS), SFLOW_OK_COUNTER32(http_status_3XX));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_http_status_4XX, SFLOW_CTR_RATE(dataSource, http_status_4XX, ctr_ival_mS), SFLOW_OK_COUNTER32(http_status_4XX));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_http_status_5XX, SFLOW_CTR_RATE(dataSource, http_status_5XX, ctr_ival_mS), SFLOW_OK_COUNTER32(http_status_5XX));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_http_status_other, SFLOW_CTR_RATE(dataSource, http_status_other, ctr_ival_mS), SFLOW_OK_COUNTER32(http_status_other));
  }

  SFLOW_CTR_LATCH(dataSource, http_meth_option);
  SFLOW_CTR_LATCH(dataSource, http_meth_get);
  SFLOW_CTR_LATCH(dataSource, http_meth_head);
  SFLOW_CTR_LATCH(dataSource, http_meth_post);
  SFLOW_CTR_LATCH(dataSource, http_meth_put);
  SFLOW_CTR_LATCH(dataSource, http_meth_delete);
  SFLOW_CTR_LATCH(dataSource, http_meth_trace);
  SFLOW_CTR_LATCH(dataSource, http_meth_connect);
  SFLOW_CTR_LATCH(dataSource, http_meth_other);
  SFLOW_CTR_LATCH(dataSource, http_status_1XX);
  SFLOW_CTR_LATCH(dataSource, http_status_2XX);
  SFLOW_CTR_LATCH(dataSource, http_status_3XX);
  SFLOW_CTR_LATCH(dataSource, http_status_4XX);
  SFLOW_CTR_LATCH(dataSource, http_status_5XX);
  SFLOW_CTR_LATCH(dataSource, http_status_other);
}


static void
process_struct_HTTP_WORKERS(SFlowXDR *x, SFlowDataSource *dataSource, Ganglia_host *hostdata, char *metric_prefix, apr_time_t ctr_ival_mS)
{
  uint32_t http_workers_active, http_workers_idle, http_workers_max, http_workers_req_delayed, http_workers_req_dropped;

  debug_msg("process_struct_HTTP_WORKERS");

  http_workers_active = SFLOWXDR_next(x);
  http_workers_idle = SFLOWXDR_next(x);
  http_workers_max = SFLOWXDR_next(x);
  http_workers_req_delayed = SFLOWXDR_next(x);
  http_workers_req_dropped = SFLOWXDR_next(x);
    
  if(x->counterDeltas) {
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_http_workers_req_delayed, SFLOW_CTR_RATE(dataSource, http_workers_req_delayed, ctr_ival_mS), SFLOW_OK_COUNTER32(http_workers_req_delayed));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_http_workers_req_dropped, SFLOW_CTR_RATE(dataSource, http_workers_req_dropped, ctr_ival_mS), SFLOW_OK_COUNTER32(http_workers_req_dropped));
  }

  SFLOW_CTR_LATCH(dataSource, http_workers_req_delayed);
  SFLOW_CTR_LATCH(dataSource, http_workers_req_dropped);

  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_http_workers_active, http_workers_active, SFLOW_OK_GAUGE32(http_workers_active));
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_http_workers_idle, http_workers_idle, SFLOW_OK_GAUGE32(http_workers_idle));
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_http_workers_max, http_workers_max, SFLOW_OK_GAUGE32(http_workers_max));
}

static void
process_struct_JVM(SFlowXDR *x, SFlowDataSource *dataSource, Ganglia_host *hostdata, char *metric_prefix, apr_time_t ctr_ival_mS)
{
  uint64_t jvm_hmem_initial;
  uint64_t jvm_hmem_used;
  uint64_t jvm_hmem_committed;
  uint64_t jvm_hmem_max;
  uint64_t jvm_nhmem_initial;
  uint64_t jvm_nhmem_used;
  uint64_t jvm_nhmem_committed;
  uint64_t jvm_nhmem_max;
  uint32_t jvm_gc_count;
  uint32_t jvm_gc_ms;
  uint32_t jvm_cls_loaded;
  uint32_t jvm_cls_total;
  uint32_t jvm_cls_unloaded;
  uint32_t jvm_comp_ms;
  uint32_t jvm_thread_live;
  uint32_t jvm_thread_daemon;
  uint32_t jvm_thread_started;
  uint32_t jvm_fds_open;
  uint32_t jvm_fds_max;

  debug_msg("process_struct_JVM");
  SFLOWXDR_next_int64(x, &jvm_hmem_initial);
  SFLOWXDR_next_int64(x, &jvm_hmem_used);
  SFLOWXDR_next_int64(x, &jvm_hmem_committed);
  SFLOWXDR_next_int64(x, &jvm_hmem_max);
  SFLOWXDR_next_int64(x, &jvm_nhmem_initial);
  SFLOWXDR_next_int64(x, &jvm_nhmem_used);
  SFLOWXDR_next_int64(x, &jvm_nhmem_committed);
  SFLOWXDR_next_int64(x, &jvm_nhmem_max);
  jvm_gc_count = SFLOWXDR_next(x);
  jvm_gc_ms = SFLOWXDR_next(x);
  jvm_cls_loaded = SFLOWXDR_next(x);
  jvm_cls_total = SFLOWXDR_next(x);
  jvm_cls_unloaded = SFLOWXDR_next(x);
  jvm_comp_ms = SFLOWXDR_next(x);
  jvm_thread_live = SFLOWXDR_next(x);
  jvm_thread_daemon = SFLOWXDR_next(x);
  jvm_thread_started = SFLOWXDR_next(x);
  jvm_fds_open = SFLOWXDR_next(x);
  jvm_fds_max = SFLOWXDR_next(x);
    
  if(x->counterDeltas) {
    debug_msg("process_struct_JVM - accumulate counters");
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_jvm_thread_started, SFLOW_CTR_RATE(dataSource, jvm_thread_started, ctr_ival_mS), SFLOW_OK_COUNTER32(jvm_thread_started));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_jvm_gc_count, SFLOW_CTR_RATE(dataSource, jvm_gc_count, ctr_ival_mS), SFLOW_OK_COUNTER32(jvm_gc_count));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_jvm_gc_cpu, SFLOW_CTR_RATE(dataSource, jvm_gc_ms, ctr_ival_mS), SFLOW_OK_COUNTER32(jvm_gc_ms));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_jvm_comp_cpu, SFLOW_CTR_RATE(dataSource, jvm_comp_ms, ctr_ival_mS), SFLOW_OK_COUNTER32(jvm_comp_ms));
  }


  SFLOW_CTR_LATCH(dataSource, jvm_thread_started);
  SFLOW_CTR_LATCH(dataSource, jvm_gc_count);
  SFLOW_CTR_LATCH(dataSource, jvm_gc_ms);
  SFLOW_CTR_LATCH(dataSource, jvm_comp_ms);

  /* and the GAUGES too */
  /* there does not seem to be a way to submit a 64-bit integer value, so cast these ones to double */

  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_jvm_hmem_initial, (double)jvm_hmem_initial, SFLOW_OK_GAUGE64(jvm_hmem_initial));
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_jvm_hmem_used, (double)jvm_hmem_used, SFLOW_OK_GAUGE64(jvm_hmem_used));
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_jvm_hmem_committed, (double)jvm_hmem_committed, SFLOW_OK_GAUGE64(jvm_hmem_committed));
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_jvm_hmem_max, (double)jvm_hmem_max, SFLOW_OK_GAUGE64(jvm_hmem_max));
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_jvm_nhmem_initial, (double)jvm_nhmem_initial, SFLOW_OK_GAUGE64(jvm_nhmem_initial));
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_jvm_nhmem_used, (double)jvm_nhmem_used, SFLOW_OK_GAUGE64(jvm_nhmem_used));
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_jvm_nhmem_committed, (double)jvm_nhmem_committed, SFLOW_OK_GAUGE64(jvm_nhmem_committed));
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_jvm_nhmem_max, (double)jvm_nhmem_max, SFLOW_OK_GAUGE64(jvm_nhmem_max));
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_jvm_cls_loaded, (double)jvm_cls_loaded, SFLOW_OK_GAUGE32(jvm_cls_loaded));
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_jvm_cls_total, (double)jvm_cls_total, SFLOW_OK_GAUGE32(jvm_cls_total));
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_jvm_cls_unloaded, (double)jvm_cls_unloaded, SFLOW_OK_GAUGE32(jvm_cls_unloaded));
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_jvm_thread_live, (double)jvm_thread_live, SFLOW_OK_GAUGE32(jvm_thread_live));
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_jvm_thread_daemon, (double)jvm_thread_daemon, SFLOW_OK_GAUGE32(jvm_thread_daemon));
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_jvm_fds_open, (double)jvm_fds_open, SFLOW_OK_GAUGE32(jvm_fds_open));
  submit_sflow_double(hostdata, metric_prefix, SFLOW_M_jvm_fds_max, (double)jvm_fds_max, SFLOW_OK_GAUGE32(jvm_fds_max));
}

static void
process_struct_NVML_GPU(SFlowXDR *x, SFlowDataSource *dataSource, Ganglia_host *hostdata, char *metric_prefix, apr_time_t ctr_ival_mS)
{
  uint32_t nvml_gpu_count, nvml_gpu_processes, nvml_gpu_time, nvml_gpu_rw_time, nvml_gpu_ecc_errors, nvml_gpu_energy, nvml_gpu_temperature, nvml_gpu_fan_speed;
  uint64_t nvml_gpu_mem_total, nvml_gpu_mem_free;

  debug_msg("process_struct_NVML_GPU");

  nvml_gpu_count = SFLOWXDR_next(x);
  nvml_gpu_processes = SFLOWXDR_next(x);
  nvml_gpu_time = SFLOWXDR_next(x);
  nvml_gpu_rw_time = SFLOWXDR_next(x);
  SFLOWXDR_next_int64(x, &nvml_gpu_mem_total);
  SFLOWXDR_next_int64(x, &nvml_gpu_mem_free);
  nvml_gpu_ecc_errors = SFLOWXDR_next(x);
  nvml_gpu_energy = SFLOWXDR_next(x);
  nvml_gpu_temperature = SFLOWXDR_next(x);
  nvml_gpu_fan_speed = SFLOWXDR_next(x);
    
  if(x->counterDeltas) {
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_nvml_gpu_time, SFLOW_CTR_MS_PC(dataSource, nvml_gpu_time, ctr_ival_mS), SFLOW_OK_COUNTER32(nvml_gpu_time));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_nvml_gpu_rw_time, SFLOW_CTR_MS_PC(dataSource, nvml_gpu_rw_time, ctr_ival_mS), SFLOW_OK_COUNTER32(nvml_gpu_rw_time));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_nvml_gpu_ecc_errors, SFLOW_CTR_RATE(dataSource, nvml_gpu_ecc_errors, ctr_ival_mS), SFLOW_OK_COUNTER32(nvml_gpu_ecc_errors));
    submit_sflow_float(hostdata, metric_prefix, SFLOW_M_nvml_gpu_energy, SFLOW_CTR_RATE(dataSource, nvml_gpu_energy, ctr_ival_mS) / (float)1000.0, SFLOW_OK_COUNTER32(nvml_gpu_energy));
  }

  SFLOW_CTR_LATCH(dataSource, nvml_gpu_time);
  SFLOW_CTR_LATCH(dataSource, nvml_gpu_rw_time);
  SFLOW_CTR_LATCH(dataSource, nvml_gpu_ecc_errors);
  SFLOW_CTR_LATCH(dataSource, nvml_gpu_energy);

  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_nvml_gpu_count, nvml_gpu_count, SFLOW_OK_GAUGE32(nvml_gpu_count));
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_nvml_gpu_processes, nvml_gpu_processes, SFLOW_OK_GAUGE32(nvml_gpu_processes));
  submit_sflow_float(hostdata, metric_prefix, SFLOW_M_nvml_gpu_mem_total, SFLOW_MEM_KB(nvml_gpu_mem_total), SFLOW_OK_GAUGE64(nvml_gpu_mem_total));
  submit_sflow_float(hostdata, metric_prefix, SFLOW_M_nvml_gpu_mem_free, SFLOW_MEM_KB(nvml_gpu_mem_free), SFLOW_OK_GAUGE64(nvml_gpu_mem_free));
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_nvml_gpu_temperature, nvml_gpu_temperature, SFLOW_OK_GAUGE32(nvml_gpu_temperature));
  submit_sflow_uint32(hostdata, metric_prefix, SFLOW_M_nvml_gpu_fan_speed, nvml_gpu_fan_speed, SFLOW_OK_GAUGE32(nvml_gpu_fan_speed));
}


static bool_t
processCounterSample(SFlowXDR *x, char **errorMsg)
{
  uint32_t lostSamples;
  char hostname[SFLOW_MAX_HOSTNAME_LEN + 1];
  char spoofstr[SFLOW_MAX_SPOOFHOST_LEN + 1];
  char osrelease[SFLOW_MAX_OSRELEASE_LEN + 1];
  char uuidstr[SFLOW_MAX_UUIDSTR_LEN + 1];
  char sflowdsi[SFLOW_MAX_DSI_LEN + 1];
  char mprefix[SFLOW_MAX_METRIC_PREFIX_LEN + 1];
  uint32_t mprefix_len;
  char *hid_mprefix = NULL;
  uint32_t machine_type, os_name;
  Ganglia_value_msg vmsg = { 0 };
  apr_time_t ctr_ival_mS;
  Ganglia_host *hostdata;
  SFlowAgent *agent;
  SFlowSubAgent *subAgent;
  SFlowDataSource *dataSource;

  /* system details */
  hostname[0] = '\0';
  spoofstr[0] = '\0';
  osrelease[0] = '\0';
  uuidstr[0] = '\0';
  sflowdsi[0] = '\0';
  machine_type = 0;
  os_name = 0;

  
  if(x->offset.HID) {
    uint32_t hostname_len;
    uint32_t osrelease_len;
    /* We got a host-id structure.  Use this to set the "spoof" field.
     * The other fields like osrelease can be picked out here and we'll
     * submit them later once we have the hostdata context. */
    SFLOWXDR_setc(x,x->offset.HID);
    hostname_len = SFLOWXDR_next(x);
    if(hostname_len > 0 && hostname_len <= SFLOW_MAX_HOSTNAME_LEN) {
      memcpy(hostname, SFLOWXDR_str(x), hostname_len);
      hostname[hostname_len] = '\0';
      SFLOWXDR_skip_b(x,hostname_len);
    }
    
    printUUID(SFLOWXDR_str(x), uuidstr, 37);
    SFLOWXDR_skip(x,4);
    
    machine_type = SFLOWXDR_next(x);
    os_name = SFLOWXDR_next(x);
    osrelease_len = SFLOWXDR_next(x);
    if(osrelease_len > 0 && osrelease_len <= SFLOW_MAX_OSRELEASE_LEN) {
      memcpy(osrelease, SFLOWXDR_str(x), osrelease_len);
      osrelease[osrelease_len] = '\0';
    }
  }

  /* look up the Ganglia host */
  hostdata = (Ganglia_host *)apr_hash_get(hosts, x->agentipstr, APR_HASH_KEY_STRING);
  if(!hostdata) {
    /* not in the hash table yet */
    if(hostname[0] && x->offset.foundPH) {
      snprintf(spoofstr,SFLOW_MAX_SPOOFHOST_LEN, "%s:%s", x->agentipstr, hostname);
      vmsg.Ganglia_value_msg_u.gstr.metric_id.host = spoofstr;
      vmsg.Ganglia_value_msg_u.gstr.metric_id.spoof = TRUE;
      hostdata = Ganglia_host_get(x->agentipstr, x->remotesa, &(vmsg.Ganglia_value_msg_u.gstr.metric_id)); 
      if(!hostdata) {
	err_msg("sFlow: Ganglia_host_get(%s) failed\n", x->agentipstr);
	return FALSE;
      }
    }
    else {
      /* We don't have the physical hostname yet,  so skip this sample. We are going
       * to wait and only create the host when we can tell it the hostname at the same
       * time. Otherwise a reverse-DNS name would be looked up and used,  and there is
       * no guarantee that it would be the same as the sFlow-supplied hostname.
       * Although gmond does not mind if the hostname changes,  gmetad does, because it
       * creates entries key'd by hostname. If we ever change our minds about the
       * hostname gmetad leaves "zombie" host entries that still show up in the UI.
       */
      return TRUE;
    }
  }
  else {
    /* found it - update the time */
    hostdata->last_heard_from = apr_time_now();
  }

  /* hang our counter and sequence number state onto the hostdata context */
  agent = hostdata->sflow;
  if(agent == NULL) {
    debug_msg("first time - allocate SFlowAgent");
    agent = hostdata->sflow = apr_pcalloc(hostdata->pool, sizeof(SFlowAgent));
    /* indicate when we started this */
    hostdata->gmond_started = apr_time_as_msec(x->now) / 1000;
  }
  /* find or create the subAgent */
  for(subAgent = hostdata->sflow->subAgents; subAgent; subAgent = subAgent->nxt) {
    if(subAgent->subAgentId == x->subAgentId) break;
  }
  if(subAgent == NULL) {
    debug_msg("create subAgent %s:%u", x->agentipstr, x->subAgentId);
    subAgent = apr_pcalloc(hostdata->pool, sizeof(SFlowSubAgent));
    subAgent->subAgentId = x->subAgentId;
    subAgent->nxt = agent->subAgents;
    agent->subAgents = subAgent;
  }
  /* find or create the dataSource */
  /* This might need to be a hash table or tree if we want to support
   * a large number of datasources per sub-agent.
   */
  for(dataSource = subAgent->dataSources; dataSource; dataSource = dataSource->nxt) {
    if(dataSource->dsClass == x->dsClass && dataSource->dsIndex == x->dsIndex) break;
  }
  if(dataSource == NULL) {
    debug_msg("create datasource %s:%u-%u:%u", x->agentipstr, x->subAgentId, x->dsClass, x->dsIndex);
    dataSource = apr_pcalloc(hostdata->pool, sizeof(SFlowDataSource));
    dataSource->dsClass = x->dsClass;
    dataSource->dsIndex = x->dsIndex;
    dataSource->last_sample_time = x->now;
    dataSource->nxt = subAgent->dataSources;
    subAgent->dataSources = dataSource;
    x->counterDeltas = FALSE;
    /* turn the dsIndex into a string and cache it here so we can use it
     * as a metric prefix if required.  (I think there may be an apr call to
     * do this in one go?)
     */
    snprintf(mprefix, SFLOW_MAX_METRIC_PREFIX_LEN, "%u", (unsigned int)dataSource->dsIndex);
    mprefix_len = strlen(mprefix);
    dataSource->metric_prefix = apr_pcalloc(hostdata->pool, mprefix_len);
    memcpy(dataSource->metric_prefix, mprefix, mprefix_len);
    dataSource->metric_prefix[mprefix_len] = '\0';
  }
  
  /* check sequence numbers */
  /* we can't use delta(datagramSeqNumber) here because we might have skipped over
   * a number of sFlow datagrams that we are not interested in (e.g. with packet
   * samples).  It's enough to use the sequence numbers associated with the datasources
   * we are interested in, because we should see all of those.
   */
  lostSamples = x->csSeqNo - dataSource->csSeqNo - 1;
  
  if(x->datagramSeqNo == 1
     || x->csSeqNo == 1
     || lostSamples > SFLOW_MAX_LOST_SAMPLES) {
    /* Agent or Sampler was restarted, or we lost too many in a row,
     * so just latch the counters this time without submitting deltas.*/
    debug_msg("sequence number error - %s:%u-%u:%u lostSamples=%u",
	      x->agentipstr, x->subAgentId, x->dsClass, x->dsIndex, lostSamples);
    x->counterDeltas = FALSE;
  }
  subAgent->datagramSeqNo = x->datagramSeqNo;
  dataSource->csSeqNo = x->csSeqNo;

  if(x->offset.HID) {
    /* make sure all strings are persistent, so that we don't have to
     * rely on submit_sflow_string() or Ganglia_value_save() making
     * a copy in the appropriate pool.  For now that only applies to
     * osrelease, uuidstr and possibly hostname */
    /* note that with the addition of metric_prefix we now *are* relying
     * on Ganglia_value_save() to copy, so this step of caching the
     * osrelease and uuid in the datasource might be redundant!
     */
    keepString(&(dataSource->osrelease), osrelease);
    keepString(&(dataSource->uuidstr), uuidstr);
    keepString(&(dataSource->hostname), hostname);
  }
  
  /* we'll need the time (in mS) since the last sample (use mS rather than
   * seconds to limit the effect of rounding/truncation in integer arithmetic) */
  ctr_ival_mS = apr_time_as_msec(x->now - dataSource->last_sample_time);
  dataSource->last_sample_time = x->now;
  
  /* always send a heartbeat */
  submit_sflow_uint32(hostdata, NULL, SFLOW_M_heartbeat, (apr_time_as_msec(x->now) - x->uptime_mS) / 1000, TRUE);

  /* choose the metric prefix in case we decide to use it below */
  if(x->offset.HID) {
    if(x->offset.foundPH) {
      /* phyiscal host - no metric prefix, and hostname is already in the hostdata */
      hid_mprefix = NULL;
    }
    else {
      /* it's a VM - use the prefix */
      hid_mprefix = dataSource->metric_prefix;
      if(dataSource->hostname && dataSource->hostname[0] != '\0') {
	/* got hostname - so we can use this name as the prefix */
	hid_mprefix = dataSource->hostname;
      }
    }
  }
  
  /* from here on it gets easier because we know by the structure id whether it pertains to the physical
   * host or whether we should consider using the metric-prefix. Start with the phyiscal host metrics...
   */

  if(x->offset.foundPH) {
    if(x->offset.HID) {
      /* sumbit the system fields that we already extracted above */
      if(dataSource->osrelease && dataSource->osrelease[0] != '\0') {
	submit_sflow_string(hostdata, hid_mprefix, SFLOW_M_os_release, dataSource->osrelease, TRUE);
      }
      if(dataSource->uuidstr && dataSource->uuidstr[0] != '\0') {
	submit_sflow_string(hostdata, hid_mprefix, SFLOW_M_uuid, dataSource->uuidstr, TRUE);
      }
      if(machine_type <= SFLOW_MAX_MACHINE_TYPE) {
	submit_sflow_string(hostdata, hid_mprefix, SFLOW_M_machine_type, SFLOWMachineTypes[machine_type], TRUE);
      }
      if(os_name <= SFLOW_MAX_OS_NAME) {
	submit_sflow_string(hostdata, hid_mprefix, SFLOW_M_os_name, SFLOWOSNames[os_name], TRUE);
      }
    }
    if((SFLOWXDR_setc(x, x->offset.CPU))) process_struct_CPU(x, dataSource, hostdata, NULL, ctr_ival_mS);
    if((SFLOWXDR_setc(x, x->offset.MEM))) process_struct_MEM(x, dataSource, hostdata, NULL, ctr_ival_mS);
    if((SFLOWXDR_setc(x, x->offset.DSK))) process_struct_DSK(x, dataSource, hostdata, NULL, ctr_ival_mS);
    if((SFLOWXDR_setc(x, x->offset.NIO))) process_struct_NIO(x, dataSource, hostdata, NULL, ctr_ival_mS);
    if((SFLOWXDR_setc(x, x->offset.VNODE))) process_struct_VNODE(x, dataSource, hostdata, NULL, ctr_ival_mS);
    if((SFLOWXDR_setc(x, x->offset.NVML_GPU))) process_struct_NVML_GPU(x, dataSource, hostdata, NULL, ctr_ival_mS);
  }

  /* now we go virtual... */
  /* however be careful how we treat the virtual counter blocks that may come from a JVM.  Although a JVM
   * is technically a virtual machine too,  we want to report JVM-specific metrics for these, so they are
   * processed separately below. */
  if(sflowCFG.submit_all_virtual
     && !x->offset.JVM) {
    if((SFLOWXDR_setc(x, x->offset.VCPU))) process_struct_VCPU(x, dataSource, hostdata, hid_mprefix, ctr_ival_mS);
    if((SFLOWXDR_setc(x, x->offset.VMEM))) process_struct_VMEM(x, dataSource, hostdata, hid_mprefix, ctr_ival_mS);
    if((SFLOWXDR_setc(x, x->offset.VDSK))) process_struct_VDSK(x, dataSource, hostdata, hid_mprefix, ctr_ival_mS);
    if((SFLOWXDR_setc(x, x->offset.VNIO))) process_struct_VNIO(x, dataSource, hostdata, hid_mprefix, ctr_ival_mS);
  }

  if(sflowCFG.submit_memcache) {
#ifdef SFLOW_MEMCACHE_2200
    if((SFLOWXDR_setc(x, x->offset.MEMCACHE_2200))) {
      process_struct_MEMCACHE_2200(x, dataSource, hostdata, sflowCFG.multiple_memcache ? dataSource->metric_prefix : NULL, ctr_ival_mS);
    }
#endif
    if((SFLOWXDR_setc(x, x->offset.MEMCACHE))) {
      process_struct_MEMCACHE(x, dataSource, hostdata, sflowCFG.multiple_memcache ? dataSource->metric_prefix : NULL, ctr_ival_mS);
    }
  }

  if(sflowCFG.submit_http) {
    if((SFLOWXDR_setc(x, x->offset.HTTP))) {
      process_struct_HTTP(x, dataSource, hostdata, sflowCFG.multiple_http ? dataSource->metric_prefix : NULL, ctr_ival_mS);
      /* detect workers structure appearing with HTTP counters and treat specially as HTTP_WORKERS.  For example,  this way
       * the workers structure that mod_sflow sends will appear here and the metrics will be http metrics with the same prefix
       */
      if((SFLOWXDR_setc(x, x->offset.WORKERS))) {
	process_struct_HTTP_WORKERS(x, dataSource, hostdata, sflowCFG.multiple_http ? dataSource->metric_prefix : NULL, ctr_ival_mS);
      }
    }
  }

  if(sflowCFG.submit_jvm) {
    if((SFLOWXDR_setc(x, x->offset.JVM))) {
      /* The JVM datasource may have sent a HostID structure. In which case the "osrelease" field will be the Java version */
      if(dataSource->osrelease && dataSource->osrelease[0] != '\0') {
	submit_sflow_string(hostdata, sflowCFG.multiple_jvm ? hid_mprefix : NULL, SFLOW_M_jvm_release, dataSource->osrelease, TRUE);
      }
      /* we can use the hid_mprefix here too, since the java agent typically fills in a virtual hostname */
      process_struct_JVM(x, dataSource, hostdata, sflowCFG.multiple_jvm ? hid_mprefix : NULL, ctr_ival_mS);
      if((SFLOWXDR_setc(x, x->offset.VCPU))) process_struct_JVM_VCPU(x, dataSource, hostdata, sflowCFG.multiple_jvm ? hid_mprefix : NULL, ctr_ival_mS);
      if((SFLOWXDR_setc(x, x->offset.VMEM))) process_struct_JVM_VMEM(x, dataSource, hostdata, sflowCFG.multiple_jvm ? hid_mprefix : NULL, ctr_ival_mS);
    }
  }

  return TRUE;
}

bool_t
process_sflow_datagram(apr_sockaddr_t *remotesa, char *buf, apr_size_t len, apr_time_t now, char **errorMsg)
{
  uint32_t samples, s;
  SFlowXDR xdrDatagram = { 0 };
  SFlowXDR *x = &xdrDatagram;
  x->counterDeltas = TRUE;
  x->now = now;
  x->remotesa = remotesa;

  if(len < SFLOW_MIN_LEN) {
    *errorMsg="sFlow datagram too short";
    return FALSE;
  }

  /* initialize the XDR parsing */
  SFLOWXDR_init(x,buf,len);
  
  /* check the version number */
  if(SFLOWXDR_next(x) != SFLOW_VERSION_5) {
    *errorMsg="sFlow version must be 5";
    return FALSE;
  }
  /* read the header */
  x->agentAddr.type = SFLOWXDR_next(x);
  switch(x->agentAddr.type) {
  case SFLOW_ADDRTYPE_IP4:
    x->agentAddr.a.ip4 = SFLOWXDR_next_n(x);
    break;
  case SFLOW_ADDRTYPE_IP6:
    x->agentAddr.a.ip6[0] = SFLOWXDR_next_n(x);
    x->agentAddr.a.ip6[1] = SFLOWXDR_next_n(x);
    x->agentAddr.a.ip6[2] = SFLOWXDR_next_n(x);
    x->agentAddr.a.ip6[3] = SFLOWXDR_next_n(x);
    break;
  default:
    *errorMsg="bad agent address type";
    return FALSE;
    break;
  }

  /* turn the agent ip into a string */
  if(x->agentAddr.type == SFLOW_ADDRTYPE_IP6) {
#if defined(HAVE_INET_NTOP) && defined(AF_INET6)
    my_inet_ntop(AF_INET6, &x->agentAddr.a, x->agentipstr, SFLOW_MAX_HOSTNAME_LEN);
#else
    /* Probably on Cygwin, or some other platform that does not support IPv6.
     just print it out myself - all we need is a unique string so don't worry
     about compact formatting. */
    snprintf(x->agentipstr, SFLOW_MAX_HOSTNAME_LEN, "%04x:%04x:%04x:%04x",
	     x->agentAddr.a.ip6[0],
	     x->agentAddr.a.ip6[1],
	     x->agentAddr.a.ip6[2],
	     x->agentAddr.a.ip6[3]);
#endif
  }
  else {
    my_inet_ntop(AF_INET, &x->agentAddr.a, x->agentipstr, SFLOW_MAX_HOSTNAME_LEN);
  }
    
  /* make sure we got something sensible */
  if(x->agentipstr == NULL || x->agentipstr[0] == '\0') {
    *errorMsg =  "sFlow agent address format error";
    return FALSE;
  }

  x->subAgentId = SFLOWXDR_next(x);
  x->datagramSeqNo = SFLOWXDR_next(x);
  x->uptime_mS = SFLOWXDR_next(x);
  /* array of flow/counter samples */
  samples = SFLOWXDR_next(x);
  for(s = 0; s < samples; s++) {
    uint32_t sType = SFLOWXDR_next(x);
    uint32_t sQuads = SFLOWXDR_next(x) >> 2;
    uint32_t sMark = SFLOWXDR_mark(x,sQuads);
    if(!SFLOWXDR_more(x,sQuads)) {
      *errorMsg="parse error: sample length";
      return FALSE;
    }
    /* only care about the counter samples */
    switch(sType) {
    case SFLOW_COUNTERS_SAMPLE_EXPANDED:
    case SFLOW_COUNTERS_SAMPLE:
      {
	uint32_t csElements, e;
	uint32_t ceTag, ceQuads, ceMark;
	x->csSeqNo = SFLOWXDR_next(x);
	if(sType == SFLOW_COUNTERS_SAMPLE_EXPANDED) {
	  x->dsClass = SFLOWXDR_next(x);
	  x->dsIndex = SFLOWXDR_next(x);
	}
	else {
	  uint32_t dsCombined = SFLOWXDR_next(x);
	  x->dsClass = dsCombined >> 24;
	  x->dsIndex = dsCombined & 0x00FFFFFF;
	}
	csElements = SFLOWXDR_next(x);
	for(e = 0; e < csElements; e++) {
	  if(!SFLOWXDR_more(x,2)) {
	    *errorMsg="parse error: counter elements";
	    return FALSE;
	  }
	  ceTag = SFLOWXDR_next(x);
	  ceQuads = SFLOWXDR_next(x) >> 2;
	  ceMark = SFLOWXDR_mark(x,ceQuads);
	  if(!SFLOWXDR_more(x,ceQuads)) {
	    *errorMsg="parse error: counter element length";
	    return FALSE;
	  }
	  /* only care about selected structures.
	   * Just record their offsets here. We'll read the fields out below */
	  switch(ceTag) {
	  case SFLOW_COUNTERBLOCK_HOST_HID: x->offset.HID = SFLOWXDR_mark(x,0); break; /* HID can be Physical or Virtual */

	  case SFLOW_COUNTERBLOCK_HOST_CPU: x->offset.CPU = SFLOWXDR_mark(x,0); x->offset.foundPH++; break;
	  case SFLOW_COUNTERBLOCK_HOST_MEM: x->offset.MEM = SFLOWXDR_mark(x,0); x->offset.foundPH++; break;
	  case SFLOW_COUNTERBLOCK_HOST_DSK: x->offset.DSK = SFLOWXDR_mark(x,0); x->offset.foundPH++; break;
	  case SFLOW_COUNTERBLOCK_HOST_NIO: x->offset.NIO = SFLOWXDR_mark(x,0); x->offset.foundPH++; break;
	  case SFLOW_COUNTERBLOCK_HOST_VNODE: x->offset.VNODE = SFLOWXDR_mark(x,0); x->offset.foundPH++; break; /* hypervisor */

	  case SFLOW_COUNTERBLOCK_HOST_VCPU: x->offset.VCPU = SFLOWXDR_mark(x,0); x->offset.foundVM++; break;
	  case SFLOW_COUNTERBLOCK_HOST_VMEM: x->offset.VMEM = SFLOWXDR_mark(x,0); x->offset.foundVM++; break;
	  case SFLOW_COUNTERBLOCK_HOST_VDSK: x->offset.VDSK = SFLOWXDR_mark(x,0); x->offset.foundVM++; break;
	  case SFLOW_COUNTERBLOCK_HOST_VNIO: x->offset.VNIO = SFLOWXDR_mark(x,0); x->offset.foundVM++; break;
#ifdef SFLOW_MEMCACHE_2200
	  case SFLOW_COUNTERBLOCK_MEMCACHE_2200: x->offset.MEMCACHE_2200 = SFLOWXDR_mark(x,0); break;
#endif
	  case SFLOW_COUNTERBLOCK_MEMCACHE: x->offset.MEMCACHE = SFLOWXDR_mark(x,0); break;
	  case SFLOW_COUNTERBLOCK_HTTP: x->offset.HTTP = SFLOWXDR_mark(x,0); break;
	  case SFLOW_COUNTERBLOCK_JVM: x->offset.JVM = SFLOWXDR_mark(x,0); break;
	  case SFLOW_COUNTERBLOCK_NVML_GPU: x->offset.NVML_GPU = SFLOWXDR_mark(x, 0); break;
	  case SFLOW_COUNTERBLOCK_WORKERS: x->offset.WORKERS = SFLOWXDR_mark(x, 0); break;
	  }
	  SFLOWXDR_skip(x,ceQuads);
	  
	  if(!SFLOWXDR_markOK(x,ceMark)) {
	    *errorMsg="parse error: counter element overrun/underrun";
	    return FALSE;
	  }
	}

	/* note that an HID struct is not enough on its own */
	if(x->offset.foundPH ||
	   (sflowCFG.submit_all_virtual && x->offset.foundVM) ||
#ifdef SFLOW_MEMCACHE_2200
	   x->offset.MEMCACHE_2200 ||
#endif
	   x->offset.MEMCACHE ||
	   x->offset.HTTP ||
	   x->offset.JVM) {
	  uint32_t csEnd = SFLOWXDR_mark(x,0);
	  if(!processCounterSample(x, errorMsg)) {
	    /* something went wrong - early termination */
	    debug_msg("process_sflow_datagram - early termination");
	    return FALSE;
	  }
	  /* make sure we pick up the decoding where we left off */
	  SFLOWXDR_setc(x, csEnd);
	}
	/* clear the offsets for the next counter sample */
	memset(&x->offset, 0, sizeof(x->offset));

      }
      break;
    default:
      /* skip other sample types */
      SFLOWXDR_skip(x,sQuads);
    }
    if(!SFLOWXDR_markOK(x,sMark)) {
      *errorMsg="parse error: sample overrun/underrun";
      return FALSE;
    }
  }
  return TRUE;
}
