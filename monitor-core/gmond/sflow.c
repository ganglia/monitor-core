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

#include "sflow.h"

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
  for(int i = 0; i < len; i++) {
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
process_sflow_gmetric(Ganglia_host *hostdata, EnumSFLOWGMetric tag, Ganglia_metadata_msg *fmsg, Ganglia_value_msg *vmsg)
{
  /* add the rest of the metadata */
  struct Ganglia_extra_data extra_array[3];
  char *group, *desc, *title;
  Ganglia_metadatadef *gfull = &fmsg->Ganglia_metadata_msg_u.gfull;
  char *mname = SFLOWGMetricTable[tag].mname;
  gfull->metric_id.name = mname;
  gfull->metric.name = SFLOWGMetricTable[tag].mname;
  gfull->metric.units = SFLOWGMetricTable[tag].units;
  gfull->metric.slope = SFLOWGMetricTable[tag].slope;
  gfull->metric.tmax = 60; /* "(secs) poll if it changes faster than this" */
  gfull->metric.dmax = 0; /* "(secs) how long before stale?" */

  /* extra metadata */
  group = SFLOWGMetricTable[tag].group;
  desc = SFLOWGMetricTable[tag].desc;
  title = SFLOWGMetricTable[tag].title;
  if(group == NULL) group = "sflow";
  if(title == NULL) title = mname;
  if(desc == NULL) desc = title;
  extra_array[0].name = "GROUP";
  extra_array[0].data = group;
  extra_array[1].name = "DESC";
  extra_array[1].data = desc;
  extra_array[2].name = "TITLE";
  extra_array[2].data = title;
  gfull->metric.metadata.metadata_len = 3;
  gfull->metric.metadata.metadata_val = extra_array;
  
  /* submit */
  Ganglia_metadata_save(hostdata, fmsg );
  Ganglia_value_save(hostdata, vmsg);
  Ganglia_update_vidals(hostdata, vmsg);
  Ganglia_metadata_check(hostdata, vmsg);
}

static void
process_sflow_float(Ganglia_host *hostdata, EnumSFLOWGMetric tag, float val, bool_t ok)
{
    Ganglia_metadata_msg fmsg = { 0 };
    Ganglia_value_msg vmsg = { 0 };
  if(ok || sflowCFG.submit_null_float) {
    fmsg.id = vmsg.id = gmetric_float;
    fmsg.Ganglia_metadata_msg_u.gfull.metric.type = "float";
    vmsg.Ganglia_value_msg_u.gf.metric_id.name = SFLOWGMetricTable[tag].mname;
    vmsg.Ganglia_value_msg_u.gf.f = (ok ? val : sflowCFG.null_float);
    vmsg.Ganglia_value_msg_u.gf.fmt = SFLOWGMetricTable[tag].format;
    process_sflow_gmetric(hostdata, tag, &fmsg, &vmsg);
  }
}

static void
process_sflow_double(Ganglia_host *hostdata, EnumSFLOWGMetric tag, double val, bool_t ok)
{
  Ganglia_metadata_msg fmsg = { 0 };
  Ganglia_value_msg vmsg = { 0 };
  if(ok || sflowCFG.submit_null_float) {
    fmsg.id = vmsg.id = gmetric_double;
    fmsg.Ganglia_metadata_msg_u.gfull.metric.type = "double";
    vmsg.Ganglia_value_msg_u.gd.metric_id.name = SFLOWGMetricTable[tag].mname;
    vmsg.Ganglia_value_msg_u.gd.d = (ok ? val : (double)sflowCFG.null_float);
    vmsg.Ganglia_value_msg_u.gd.fmt = SFLOWGMetricTable[tag].format;
    process_sflow_gmetric(hostdata, tag, &fmsg, &vmsg);
  }
}

static void
process_sflow_uint16(Ganglia_host *hostdata, EnumSFLOWGMetric tag, uint16_t val, bool_t ok)
{
  Ganglia_metadata_msg fmsg = { 0 };
  Ganglia_value_msg vmsg = { 0 };
  if(ok || sflowCFG.submit_null_int) {
    fmsg.id = vmsg.id = gmetric_ushort;
    fmsg.Ganglia_metadata_msg_u.gfull.metric.type = "uint16";
    vmsg.Ganglia_value_msg_u.gu_short.metric_id.name = SFLOWGMetricTable[tag].mname;
    vmsg.Ganglia_value_msg_u.gu_short.us = (ok ? val : (uint16_t)sflowCFG.null_int);
    vmsg.Ganglia_value_msg_u.gu_short.fmt = SFLOWGMetricTable[tag].format;
    process_sflow_gmetric(hostdata, tag, &fmsg, &vmsg);
  }
}

static void
process_sflow_uint32(Ganglia_host *hostdata, EnumSFLOWGMetric tag, uint32_t val, bool_t ok)
{
  Ganglia_metadata_msg fmsg = { 0 };
  Ganglia_value_msg vmsg = { 0 };
  if(ok || sflowCFG.submit_null_int) {
    fmsg.id = vmsg.id = gmetric_uint;
    fmsg.Ganglia_metadata_msg_u.gfull.metric.type = "uint32";
    vmsg.Ganglia_value_msg_u.gu_int.metric_id.name = SFLOWGMetricTable[tag].mname;
    vmsg.Ganglia_value_msg_u.gu_int.ui = (ok ? val : sflowCFG.null_int);
    vmsg.Ganglia_value_msg_u.gu_int.fmt = SFLOWGMetricTable[tag].format;
    process_sflow_gmetric(hostdata, tag, &fmsg, &vmsg);
  }
}

static void
process_sflow_string(Ganglia_host *hostdata, EnumSFLOWGMetric tag, const char *val, bool_t ok)
{
  Ganglia_metadata_msg fmsg = { 0 };
  Ganglia_value_msg vmsg = { 0 };
  if(ok || sflowCFG.submit_null_str) {
    fmsg.id = vmsg.id = gmetric_uint;
    fmsg.Ganglia_metadata_msg_u.gfull.metric.type = "string";
    vmsg.Ganglia_value_msg_u.gstr.metric_id.name = SFLOWGMetricTable[tag].mname;
    vmsg.Ganglia_value_msg_u.gstr.str = (ok ? (char *)val : sflowCFG.null_str);
    vmsg.Ganglia_value_msg_u.gstr.fmt = SFLOWGMetricTable[tag].format;
    process_sflow_gmetric(hostdata, tag, &fmsg, &vmsg);
  }
}

#define SFLOW_MEM_KB(bytes) (float)(bytes / 1024)

  /* convenience macros for handling counters */
#define SFLOW_CTR_LATCH(hdata,field) hdata->sflow->field = field
#define SFLOW_CTR_DELTA(hdata,field) (field - hdata->sflow->field)
#define SFLOW_CTR_RATE(hdata, field, mS) ((float)(SFLOW_CTR_DELTA(hdata, field)) * (float)1000.0 / (float)mS)
#define SFLOW_CTR_MS_PC(hdata, field, mS) ((float)(SFLOW_CTR_DELTA(hdata, field)) * (float)100.0 / (float)mS)

  /* metrics may be marked as "unsupported" by the sender,  so check for those reserved values */
#define SFLOW_OK_FLOAT(field) (field != (float)-1)
#define SFLOW_OK_GAUGE32(field) (field != (uint32_t)-1)
#define SFLOW_OK_GAUGE64(field) (field != (uint64_t)-1)
#define SFLOW_OK_COUNTER32(field) (field != (uint32_t)-1)
#define SFLOW_OK_COUNTER64(field) (field != (uint64_t)-1)

static void
process_struct_CPU(SFlowXDR *x, Ganglia_host *hostdata,  apr_time_t ctr_ival_mS)
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
  
  process_sflow_float(hostdata, SFLOW_M_load_one, load_one, SFLOW_OK_FLOAT(load_one));
  process_sflow_float(hostdata, SFLOW_M_load_five, load_five, SFLOW_OK_FLOAT(load_five));
  process_sflow_float(hostdata, SFLOW_M_load_fifteen, load_fifteen, SFLOW_OK_FLOAT(load_fifteen));
  process_sflow_uint32(hostdata, SFLOW_M_proc_run, proc_run, SFLOW_OK_GAUGE32(proc_run));
  process_sflow_uint32(hostdata, SFLOW_M_proc_total, proc_total, SFLOW_OK_GAUGE32(proc_total));
  process_sflow_uint16(hostdata, SFLOW_M_cpu_num, cpu_num, SFLOW_OK_GAUGE32(cpu_num));
  process_sflow_uint32(hostdata, SFLOW_M_cpu_speed, cpu_speed, SFLOW_OK_GAUGE32(cpu_speed));
  process_sflow_uint32(hostdata, SFLOW_M_cpu_boottime, (apr_time_as_msec(x->now) / 1000) - cpu_uptime, SFLOW_OK_GAUGE32(cpu_uptime));
  
  if(x->counterDeltas) {
    uint32_t delta_cpu_user =   SFLOW_CTR_DELTA(hostdata, cpu_user);
    uint32_t delta_cpu_nice =   SFLOW_CTR_DELTA(hostdata, cpu_nice);
    uint32_t delta_cpu_system = SFLOW_CTR_DELTA(hostdata, cpu_system);
    uint32_t delta_cpu_idle =   SFLOW_CTR_DELTA(hostdata, cpu_idle);
    uint32_t delta_cpu_wio =    SFLOW_CTR_DELTA(hostdata, cpu_wio);
    uint32_t delta_cpu_intr =   SFLOW_CTR_DELTA(hostdata, cpu_intr);
    uint32_t delta_cpu_sintr =  SFLOW_CTR_DELTA(hostdata, cpu_sintr);
      
    uint32_t cpu_total = \
      delta_cpu_user +
      delta_cpu_nice +
      delta_cpu_system +
      delta_cpu_idle +
      delta_cpu_wio +
      delta_cpu_intr +
      delta_cpu_sintr;

#define SFLOW_CTR_CPU_PC(field) (cpu_total ? ((float)field * 100.0 / (float)cpu_total) : 0)
    process_sflow_float(hostdata, SFLOW_M_cpu_user, SFLOW_CTR_CPU_PC(delta_cpu_user), SFLOW_OK_COUNTER32(cpu_user));
    process_sflow_float(hostdata, SFLOW_M_cpu_nice, SFLOW_CTR_CPU_PC(delta_cpu_nice), SFLOW_OK_COUNTER32(cpu_nice));
    process_sflow_float(hostdata, SFLOW_M_cpu_system, SFLOW_CTR_CPU_PC(delta_cpu_system), SFLOW_OK_COUNTER32(cpu_system));
    process_sflow_float(hostdata, SFLOW_M_cpu_idle, SFLOW_CTR_CPU_PC(delta_cpu_idle), SFLOW_OK_COUNTER32(cpu_idle));
    process_sflow_float(hostdata, SFLOW_M_cpu_wio, SFLOW_CTR_CPU_PC(delta_cpu_wio), SFLOW_OK_COUNTER32(cpu_wio));
    process_sflow_float(hostdata, SFLOW_M_cpu_intr, SFLOW_CTR_CPU_PC(delta_cpu_intr), SFLOW_OK_COUNTER32(cpu_intr));
    process_sflow_float(hostdata, SFLOW_M_cpu_sintr, SFLOW_CTR_CPU_PC(delta_cpu_sintr), SFLOW_OK_COUNTER32(cpu_sintr));

    if(sflowCFG.submit_all_physical) {
      process_sflow_float(hostdata, SFLOW_M_interrupts, SFLOW_CTR_RATE(hostdata, interrupts, ctr_ival_mS), SFLOW_OK_COUNTER32(interrupts));
      process_sflow_float(hostdata, SFLOW_M_contexts, SFLOW_CTR_RATE(hostdata, contexts, ctr_ival_mS), SFLOW_OK_COUNTER32(contexts));
    }
  }

    
  SFLOW_CTR_LATCH(hostdata, cpu_user);
  SFLOW_CTR_LATCH(hostdata, cpu_nice);
  SFLOW_CTR_LATCH(hostdata, cpu_system);
  SFLOW_CTR_LATCH(hostdata, cpu_idle);
  SFLOW_CTR_LATCH(hostdata, cpu_wio);
  SFLOW_CTR_LATCH(hostdata, cpu_intr);
  SFLOW_CTR_LATCH(hostdata, cpu_sintr);
  SFLOW_CTR_LATCH(hostdata, interrupts);
  SFLOW_CTR_LATCH(hostdata, contexts);
}

static void
process_struct_MEM(SFlowXDR *x, Ganglia_host *hostdata,  apr_time_t ctr_ival_mS)
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
  
  process_sflow_float(hostdata, SFLOW_M_mem_total, SFLOW_MEM_KB(mem_total), SFLOW_OK_GAUGE64(mem_total));
  process_sflow_float(hostdata, SFLOW_M_mem_free, SFLOW_MEM_KB(mem_free), SFLOW_OK_GAUGE64(mem_free));
  process_sflow_float(hostdata, SFLOW_M_mem_shared, SFLOW_MEM_KB(mem_shared), SFLOW_OK_GAUGE64(mem_shared));
  process_sflow_float(hostdata, SFLOW_M_mem_buffers, SFLOW_MEM_KB(mem_buffers), SFLOW_OK_GAUGE64(mem_buffers));
  process_sflow_float(hostdata, SFLOW_M_mem_cached, SFLOW_MEM_KB(mem_cached), SFLOW_OK_GAUGE64(mem_cached));
  process_sflow_float(hostdata, SFLOW_M_swap_total, SFLOW_MEM_KB(swap_total), SFLOW_OK_GAUGE64(swap_total));
  process_sflow_float(hostdata, SFLOW_M_swap_free, SFLOW_MEM_KB(swap_free), SFLOW_OK_GAUGE64(swap_free));
  
  if(x->counterDeltas) {
    if(sflowCFG.submit_all_physical) {
      process_sflow_float(hostdata, SFLOW_M_page_in, SFLOW_CTR_RATE(hostdata, page_in, ctr_ival_mS), SFLOW_OK_COUNTER32(page_in));
      process_sflow_float(hostdata, SFLOW_M_page_out, SFLOW_CTR_RATE(hostdata, page_out, ctr_ival_mS), SFLOW_OK_COUNTER32(page_out));
      process_sflow_float(hostdata, SFLOW_M_swap_in, SFLOW_CTR_RATE(hostdata, swap_in, ctr_ival_mS), SFLOW_OK_COUNTER32(swap_in));
      process_sflow_float(hostdata, SFLOW_M_swap_out, SFLOW_CTR_RATE(hostdata, swap_out, ctr_ival_mS), SFLOW_OK_COUNTER32(swap_out));
    }
  }
  SFLOW_CTR_LATCH(hostdata, page_in);
  SFLOW_CTR_LATCH(hostdata, page_out);
  SFLOW_CTR_LATCH(hostdata, swap_in);
  SFLOW_CTR_LATCH(hostdata, swap_out);
}

static void
process_struct_DSK(SFlowXDR *x, Ganglia_host *hostdata,  apr_time_t ctr_ival_mS)
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
  process_sflow_double(hostdata, SFLOW_M_disk_total, (double)disk_total / 1073741824.0, SFLOW_OK_GAUGE64(disk_total));
  process_sflow_double(hostdata, SFLOW_M_disk_free, (double)disk_free / 1073741824.0, SFLOW_OK_GAUGE64(disk_free));
  process_sflow_float(hostdata, SFLOW_M_part_max_used, (float)part_max_used / 100.0, SFLOW_OK_GAUGE32(part_max_used));

  if(x->counterDeltas) {
    if(sflowCFG.submit_all_physical) {
      process_sflow_float(hostdata, SFLOW_M_reads, SFLOW_CTR_RATE(hostdata, reads, ctr_ival_mS), SFLOW_OK_COUNTER32(reads));
      process_sflow_float(hostdata, SFLOW_M_bytes_read, SFLOW_CTR_RATE(hostdata, bytes_read, ctr_ival_mS), SFLOW_OK_COUNTER64(bytes_read));
      process_sflow_float(hostdata, SFLOW_M_read_time, SFLOW_CTR_MS_PC(hostdata, read_time, ctr_ival_mS), SFLOW_OK_COUNTER32(read_time));
      process_sflow_float(hostdata, SFLOW_M_writes, SFLOW_CTR_RATE(hostdata, writes, ctr_ival_mS), SFLOW_OK_COUNTER32(writes));
      process_sflow_float(hostdata, SFLOW_M_bytes_written, SFLOW_CTR_RATE(hostdata, bytes_written, ctr_ival_mS), SFLOW_OK_COUNTER64(bytes_written));
      process_sflow_float(hostdata, SFLOW_M_write_time, SFLOW_CTR_MS_PC(hostdata, write_time, ctr_ival_mS), SFLOW_OK_COUNTER32(write_time));
    }
  }

  SFLOW_CTR_LATCH(hostdata, reads);
  SFLOW_CTR_LATCH(hostdata, bytes_read);
  SFLOW_CTR_LATCH(hostdata, read_time);
  SFLOW_CTR_LATCH(hostdata, writes);
  SFLOW_CTR_LATCH(hostdata, bytes_written);
  SFLOW_CTR_LATCH(hostdata, write_time);
}

static void
process_struct_NIO(SFlowXDR *x, Ganglia_host *hostdata,  apr_time_t ctr_ival_mS)
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
    process_sflow_float(hostdata, SFLOW_M_bytes_in, SFLOW_CTR_RATE(hostdata, bytes_in, ctr_ival_mS), SFLOW_OK_COUNTER64(bytes_in));
    process_sflow_float(hostdata, SFLOW_M_pkts_in, SFLOW_CTR_RATE(hostdata, pkts_in, ctr_ival_mS), SFLOW_OK_COUNTER32(pkts_in));
    process_sflow_float(hostdata, SFLOW_M_bytes_out, SFLOW_CTR_RATE(hostdata, bytes_out, ctr_ival_mS), SFLOW_OK_COUNTER64(bytes_out));
    process_sflow_float(hostdata, SFLOW_M_pkts_out, SFLOW_CTR_RATE(hostdata, pkts_out, ctr_ival_mS), SFLOW_OK_COUNTER32(pkts_out));
    if(sflowCFG.submit_all_physical) {
      process_sflow_float(hostdata, SFLOW_M_errs_in, SFLOW_CTR_RATE(hostdata, errs_in, ctr_ival_mS), SFLOW_OK_COUNTER32(errs_in));
      process_sflow_float(hostdata, SFLOW_M_drops_in, SFLOW_CTR_RATE(hostdata, drops_in, ctr_ival_mS), SFLOW_OK_COUNTER32(drops_in));
      process_sflow_float(hostdata, SFLOW_M_errs_out, SFLOW_CTR_RATE(hostdata, errs_out, ctr_ival_mS), SFLOW_OK_COUNTER32(errs_out));
      process_sflow_float(hostdata, SFLOW_M_drops_out, SFLOW_CTR_RATE(hostdata, drops_out, ctr_ival_mS), SFLOW_OK_COUNTER32(drops_out));
    }
  }
    
  SFLOW_CTR_LATCH(hostdata, bytes_in);
  SFLOW_CTR_LATCH(hostdata, pkts_in);
  SFLOW_CTR_LATCH(hostdata, errs_in);
  SFLOW_CTR_LATCH(hostdata, drops_in);
  SFLOW_CTR_LATCH(hostdata, bytes_out);
  SFLOW_CTR_LATCH(hostdata, pkts_out);
  SFLOW_CTR_LATCH(hostdata, errs_out);
  SFLOW_CTR_LATCH(hostdata, drops_out);
}

static void
process_struct_VNODE(SFlowXDR *x, Ganglia_host *hostdata,  apr_time_t ctr_ival_mS)
{
  uint32_t mhz, cpus, domains;
  uint64_t memory, memory_free;
  mhz =  SFLOWXDR_next(x);
  cpus = SFLOWXDR_next(x);
  SFLOWXDR_next_int64(x,&memory);
  SFLOWXDR_next_int64(x,&memory_free);
  domains = SFLOWXDR_next(x);

  if(sflowCFG.submit_all_physical) {
    process_sflow_uint32(hostdata, SFLOW_M_vnode_cpu_speed, mhz, SFLOW_OK_GAUGE32(mhz));
    process_sflow_uint32(hostdata, SFLOW_M_vnode_cpu_num, cpus, SFLOW_OK_GAUGE32(cpus));
    process_sflow_float(hostdata, SFLOW_M_vnode_mem_total, SFLOW_MEM_KB(memory), SFLOW_OK_GAUGE64(memory));
    process_sflow_float(hostdata, SFLOW_M_vnode_mem_free, SFLOW_MEM_KB(memory_free), SFLOW_OK_GAUGE64(memory_free));
    process_sflow_uint32(hostdata, SFLOW_M_vnode_domains, domains, SFLOW_OK_GAUGE32(domains));
  }
}

static void
process_struct_PARENT(SFlowXDR *x, Ganglia_host *hostdata,  apr_time_t ctr_ival_mS)
{
  uint32_t parentClass, parentIndex;
  parentClass = SFLOWXDR_next(x);
  parentIndex = SFLOWXDR_next(x);
  /* record the parent sFlow data-source identifier, so that the UI can associate
   * VMs with their hypervisors, etc. */
  char sflowdsi[SFLOW_MAX_DSI_LEN + 1];
  snprintf(sflowdsi, SFLOW_MAX_DSI_LEN, "%s>%u:%u", x->agentipstr, parentClass, parentIndex);
  keepString(&hostdata->sflow->parent_dsi, sflowdsi);
  process_sflow_string(hostdata, SFLOW_M_parent_dsi, hostdata->sflow->parent_dsi, TRUE);
}

static void
process_struct_VCPU(SFlowXDR *x, Ganglia_host *hostdata,  apr_time_t ctr_ival_mS)
{
  uint32_t vstate, vcpu_mS, vcpus;
  vstate =  SFLOWXDR_next(x);
  vcpu_mS = SFLOWXDR_next(x);
  vcpus = SFLOWXDR_next(x);

  if(vstate <= SFLOW_MAX_DOMAINSTATE_NAME) {
    process_sflow_string(hostdata, SFLOW_M_vcpu_state, SFLOWVirDomainStateNames[vstate], TRUE);
  }
  process_sflow_uint32(hostdata, SFLOW_M_vcpu_num, vcpus, SFLOW_OK_GAUGE32(vcpus));
  
  if(x->counterDeltas) {
    process_sflow_float(hostdata, SFLOW_M_vcpu_util, SFLOW_CTR_MS_PC(hostdata, vcpu_mS, ctr_ival_mS), SFLOW_OK_COUNTER32(vcpu_mS));
  }
  SFLOW_CTR_LATCH(hostdata, vcpu_mS);
}

static void
process_struct_VMEM(SFlowXDR *x, Ganglia_host *hostdata,  apr_time_t ctr_ival_mS)
{
  uint64_t memory, memory_max;
  SFLOWXDR_next_int64(x,&memory);
  SFLOWXDR_next_int64(x,&memory_max);

  process_sflow_float(hostdata, SFLOW_M_vmem_total, SFLOW_MEM_KB(memory_max), SFLOW_OK_GAUGE64(memory_max));
  if(memory_max > 0) {
    process_sflow_float(hostdata, SFLOW_M_vmem_util, (float)memory * (float)100.0 / (float)memory_max, SFLOW_OK_GAUGE64(memory));
  }
}

static void
process_struct_VDSK(SFlowXDR *x, Ganglia_host *hostdata,  apr_time_t ctr_ival_mS)
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
  process_sflow_double(hostdata, SFLOW_M_vdisk_capacity, (double)disk_capacity / 1073741824.0, SFLOW_OK_GAUGE64(disk_capacity));
  process_sflow_double(hostdata, SFLOW_M_vdisk_total, (double)disk_allocation / 1073741824.0, SFLOW_OK_GAUGE64(disk_allocation));
  process_sflow_double(hostdata, SFLOW_M_vdisk_free, (double)disk_free / 1073741824.0, SFLOW_OK_GAUGE64(disk_free));

  if(x->counterDeltas) {
    process_sflow_float(hostdata, SFLOW_M_vdisk_reads, SFLOW_CTR_RATE(hostdata, vreads, ctr_ival_mS), SFLOW_OK_COUNTER32(vreads));
    process_sflow_float(hostdata, SFLOW_M_vdisk_bytes_read, SFLOW_CTR_RATE(hostdata, vbytes_read, ctr_ival_mS), SFLOW_OK_COUNTER64(vbytes_read));
    process_sflow_float(hostdata, SFLOW_M_vdisk_writes, SFLOW_CTR_RATE(hostdata, vwrites, ctr_ival_mS), SFLOW_OK_COUNTER32(vwrites));
    process_sflow_float(hostdata, SFLOW_M_vdisk_bytes_written, SFLOW_CTR_RATE(hostdata, vbytes_written, ctr_ival_mS), SFLOW_OK_COUNTER64(vbytes_written));
    process_sflow_float(hostdata, SFLOW_M_vdisk_errs, SFLOW_CTR_RATE(hostdata, vdskerrs, ctr_ival_mS), SFLOW_OK_COUNTER32(vdskerrs));
  }

  SFLOW_CTR_LATCH(hostdata, vreads);
  SFLOW_CTR_LATCH(hostdata, vbytes_read);
  SFLOW_CTR_LATCH(hostdata, vwrites);
  SFLOW_CTR_LATCH(hostdata, vbytes_written);
  SFLOW_CTR_LATCH(hostdata, vdskerrs);
}

static void
process_struct_VNIO(SFlowXDR *x, Ganglia_host *hostdata,  apr_time_t ctr_ival_mS)
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
    process_sflow_float(hostdata, SFLOW_M_vbytes_in, SFLOW_CTR_RATE(hostdata, vbytes_in, ctr_ival_mS), SFLOW_OK_COUNTER64(vbytes_in));
    process_sflow_float(hostdata, SFLOW_M_vpkts_in, SFLOW_CTR_RATE(hostdata, vpkts_in, ctr_ival_mS), SFLOW_OK_COUNTER32(vpkts_in));
    process_sflow_float(hostdata, SFLOW_M_vbytes_out, SFLOW_CTR_RATE(hostdata, vbytes_out, ctr_ival_mS), SFLOW_OK_COUNTER64(vbytes_out));
    process_sflow_float(hostdata, SFLOW_M_vpkts_out, SFLOW_CTR_RATE(hostdata, vpkts_out, ctr_ival_mS), SFLOW_OK_COUNTER32(vpkts_out));
    process_sflow_float(hostdata, SFLOW_M_verrs_in, SFLOW_CTR_RATE(hostdata, verrs_in, ctr_ival_mS), SFLOW_OK_COUNTER32(verrs_in));
    process_sflow_float(hostdata, SFLOW_M_vdrops_in, SFLOW_CTR_RATE(hostdata, vdrops_in, ctr_ival_mS), SFLOW_OK_COUNTER32(vdrops_in));
    process_sflow_float(hostdata, SFLOW_M_verrs_out, SFLOW_CTR_RATE(hostdata, verrs_out, ctr_ival_mS), SFLOW_OK_COUNTER32(verrs_out));
    process_sflow_float(hostdata, SFLOW_M_vdrops_out, SFLOW_CTR_RATE(hostdata, vdrops_out, ctr_ival_mS), SFLOW_OK_COUNTER32(vdrops_out));
  }
    
  SFLOW_CTR_LATCH(hostdata, vbytes_in);
  SFLOW_CTR_LATCH(hostdata, vpkts_in);
  SFLOW_CTR_LATCH(hostdata, verrs_in);
  SFLOW_CTR_LATCH(hostdata, vdrops_in);
  SFLOW_CTR_LATCH(hostdata, vbytes_out);
  SFLOW_CTR_LATCH(hostdata, vpkts_out);
  SFLOW_CTR_LATCH(hostdata, verrs_out);
  SFLOW_CTR_LATCH(hostdata, vdrops_out);
}

static bool_t
processCounterSample(SFlowXDR *x, char **errorMsg)
{
  uint32_t lostDatagrams, lostSamples;
  char hostname[SFLOW_MAX_HOSTNAME_LEN + 1];
  char spoofstr[SFLOW_MAX_SPOOFHOST_LEN + 1];
  char osrelease[SFLOW_MAX_OSRELEASE_LEN + 1];
  char uuidstr[SFLOW_MAX_UUIDSTR_LEN + 1];
  char sflowdsi[SFLOW_MAX_DSI_LEN + 1];
  uint32_t machine_type, os_name;
  Ganglia_value_msg vmsg = { 0 };
  apr_time_t ctr_ival_mS;
  Ganglia_host *hostdata;

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

  /* spoof options requires "<ip>:<hostname>" but <ip> can be any string
   * so we are going to use the UUID for VMs (which may not even have an IP address).
   */
  snprintf(spoofstr,SFLOW_MAX_SPOOFHOST_LEN, "%s:%s",
	   x->offset.foundVM ? uuidstr : x->agentipstr,
	   hostname);
  vmsg.Ganglia_value_msg_u.gstr.metric_id.host = spoofstr;
  vmsg.Ganglia_value_msg_u.gstr.metric_id.spoof = TRUE;

  /* lookup the ganglia host */
  hostdata = Ganglia_host_get(x->agentipstr, x->remotesa, &(vmsg.Ganglia_value_msg_u.gstr.metric_id)); 
  if(!hostdata) {
    *errorMsg = "host_get failed";
    return FALSE;
  }

  /* hang our counter and sequence number state onto the hostdata context */
  if(!hostdata->sflow) {
    hostdata->sflow = apr_pcalloc(hostdata->pool, sizeof(SFlowCounterState));
    hostdata->sflow->last_sample_time = x->now;
    x->counterDeltas = FALSE;
    hostdata->sflow->dsIndex = x->dsIndex;
    /* indicate when we started this */
    hostdata->gmond_started = apr_time_as_msec(x->now) / 1000;
  }
  
  /* check sequence numbers */
  lostDatagrams = x->datagramSeqNo - hostdata->sflow->datagramSeqNo - 1;
  lostSamples = x->csSeqNo - hostdata->sflow->csSeqNo - 1;
  if(x->datagramSeqNo == 1
     || x->csSeqNo == 1
     || lostDatagrams > SFLOW_MAX_LOST_SAMPLES
     || lostSamples > SFLOW_MAX_LOST_SAMPLES) {
    /* Agent or Sampler was restarted, or we lost too many in a row,
     * so just latch the counters this time without submitting deltas.*/
    x->counterDeltas = FALSE;
  }
  hostdata->sflow->datagramSeqNo = x->datagramSeqNo;
  hostdata->sflow->csSeqNo = x->csSeqNo;

  /* make sure all strings are persistent, so that we don't have to
   * rely on process_sflow_string() or Ganglia_value_save() making
   * a copy in the appropriate pool.  For now that only applies to
   * osrelease and uuidstr */
  keepString(&(hostdata->sflow->osrelease), osrelease);
  keepString(&(hostdata->sflow->uuidstr), uuidstr);
  
  /* we'll need the time (in mS) since the last sample (use mS rather than
   * seconds to limit the effect of rounding/truncation in integer arithmetic) */
  ctr_ival_mS = apr_time_as_msec(x->now - hostdata->sflow->last_sample_time);
  hostdata->sflow->last_sample_time = x->now;

  /* always send a heartbeat */
  process_sflow_uint32(hostdata, SFLOW_M_heartbeat, (apr_time_as_msec(x->now) - x->uptime_mS) / 1000, TRUE);
  
  if(x->offset.HID) {
    /* sumbit the system fields that we already extracted above */
    if(hostdata->sflow->osrelease && hostdata->sflow->osrelease[0] != '\0') {
      process_sflow_string(hostdata, SFLOW_M_os_release, hostdata->sflow->osrelease, TRUE);
    }
    if(hostdata->sflow->uuidstr && hostdata->sflow->uuidstr[0] != '\0') {
      process_sflow_string(hostdata, SFLOW_M_uuid, hostdata->sflow->uuidstr, TRUE);
    }
    if(machine_type <= SFLOW_MAX_MACHINE_TYPE) {
      process_sflow_string(hostdata, SFLOW_M_machine_type, SFLOWMachineTypes[machine_type], TRUE);
    }
    if(os_name <= SFLOW_MAX_OS_NAME) {
      process_sflow_string(hostdata, SFLOW_M_os_name, SFLOWOSNames[os_name], TRUE);
    }
  }

  if((SFLOWXDR_setc(x, x->offset.CPU))) process_struct_CPU(x, hostdata, ctr_ival_mS);
  if((SFLOWXDR_setc(x, x->offset.MEM))) process_struct_MEM(x, hostdata, ctr_ival_mS);
  if((SFLOWXDR_setc(x, x->offset.DSK))) process_struct_DSK(x, hostdata, ctr_ival_mS);
  if((SFLOWXDR_setc(x, x->offset.NIO))) process_struct_NIO(x, hostdata, ctr_ival_mS);
  if((SFLOWXDR_setc(x, x->offset.VNODE))) process_struct_VNODE(x, hostdata, ctr_ival_mS);
  
  /* record our sFlow data-source identifier */
  snprintf(sflowdsi, SFLOW_MAX_DSI_LEN, "%s>%u:%u", x->agentipstr, x->dsClass, x->dsIndex);
  keepString(&hostdata->sflow->dsi, sflowdsi);
  process_sflow_string(hostdata, SFLOW_M_dsi, hostdata->sflow->dsi, TRUE);

  if(sflowCFG.submit_all_virtual) {
    if((SFLOWXDR_setc(x, x->offset.PARENT))) process_struct_PARENT(x, hostdata, ctr_ival_mS);
    if((SFLOWXDR_setc(x, x->offset.VCPU))) process_struct_VCPU(x, hostdata, ctr_ival_mS);
    if((SFLOWXDR_setc(x, x->offset.VMEM))) process_struct_VMEM(x, hostdata, ctr_ival_mS);
    if((SFLOWXDR_setc(x, x->offset.VDSK))) process_struct_VDSK(x, hostdata, ctr_ival_mS);
    if((SFLOWXDR_setc(x, x->offset.VNIO))) process_struct_VNIO(x, hostdata, ctr_ival_mS);
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
  if(my_inet_ntop(x->agentAddr.type == SFLOW_ADDRTYPE_IP6 ? AF_INET6 : AF_INET,
		  &x->agentAddr.a,
		  x->agentipstr,
		  SFLOW_MAX_HOSTNAME_LEN) == NULL) {
    *errorMsg =  "sFlow agent address format error";
    return FALSE;
  }

  x->agentSubId = SFLOWXDR_next(x);
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

	  case SFLOW_COUNTERBLOCK_HOST_PARENT: x->offset.PARENT = SFLOWXDR_mark(x,0); x->offset.foundVM++; break;
	  case SFLOW_COUNTERBLOCK_HOST_VCPU: x->offset.VCPU = SFLOWXDR_mark(x,0); x->offset.foundVM++; break;
	  case SFLOW_COUNTERBLOCK_HOST_VMEM: x->offset.VMEM = SFLOWXDR_mark(x,0); x->offset.foundVM++; break;
	  case SFLOW_COUNTERBLOCK_HOST_VDSK: x->offset.VDSK = SFLOWXDR_mark(x,0); x->offset.foundVM++; break;
	  case SFLOW_COUNTERBLOCK_HOST_VNIO: x->offset.VNIO = SFLOWXDR_mark(x,0); x->offset.foundVM++; break;
	  }
	  SFLOWXDR_skip(x,ceQuads);
	  
	  if(!SFLOWXDR_markOK(x,ceMark)) {
	    *errorMsg="parse error: counter element overrun/underrun";
	    return FALSE;
	  }
	}

	/* note that an HID struct is not enough on its own */
	if(x->offset.foundPH || (sflowCFG.submit_all_virtual && x->offset.foundVM)) {
	  uint32_t csEnd = SFLOWXDR_mark(x,0);
	  if(!processCounterSample(x, errorMsg)) {
	    /* something went wrong - early termination */
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
