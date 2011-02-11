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
process_sflow_float(Ganglia_host *hostdata, EnumSFLOWGMetric tag, float val)
{
  Ganglia_metadata_msg fmsg = { 0 };
  Ganglia_value_msg vmsg = { 0 };
  fmsg.id = vmsg.id = gmetric_float;
  fmsg.Ganglia_metadata_msg_u.gfull.metric.type = "float";
  vmsg.Ganglia_value_msg_u.gf.metric_id.name = SFLOWGMetricTable[tag].mname;
  vmsg.Ganglia_value_msg_u.gf.f = val;
  vmsg.Ganglia_value_msg_u.gf.fmt = SFLOWGMetricTable[tag].format;
  process_sflow_gmetric(hostdata, tag, &fmsg, &vmsg);
}

static void
process_sflow_double(Ganglia_host *hostdata, EnumSFLOWGMetric tag, double val)
{
  Ganglia_metadata_msg fmsg = { 0 };
  Ganglia_value_msg vmsg = { 0 };
  fmsg.id = vmsg.id = gmetric_double;
  fmsg.Ganglia_metadata_msg_u.gfull.metric.type = "double";
  vmsg.Ganglia_value_msg_u.gd.metric_id.name = SFLOWGMetricTable[tag].mname;
  vmsg.Ganglia_value_msg_u.gd.d = val;
  vmsg.Ganglia_value_msg_u.gd.fmt = SFLOWGMetricTable[tag].format;
  process_sflow_gmetric(hostdata, tag, &fmsg, &vmsg);
}

static void
process_sflow_uint16(Ganglia_host *hostdata, EnumSFLOWGMetric tag, uint16_t val)
{
  Ganglia_metadata_msg fmsg = { 0 };
  Ganglia_value_msg vmsg = { 0 };
  fmsg.id = vmsg.id = gmetric_ushort;
  fmsg.Ganglia_metadata_msg_u.gfull.metric.type = "uint16";
  vmsg.Ganglia_value_msg_u.gu_short.metric_id.name = SFLOWGMetricTable[tag].mname;
  vmsg.Ganglia_value_msg_u.gu_short.us = val;
  vmsg.Ganglia_value_msg_u.gu_short.fmt = SFLOWGMetricTable[tag].format;
  process_sflow_gmetric(hostdata, tag, &fmsg, &vmsg);
}

static void
process_sflow_uint32(Ganglia_host *hostdata, EnumSFLOWGMetric tag, uint32_t val)
{
  Ganglia_metadata_msg fmsg = { 0 };
  Ganglia_value_msg vmsg = { 0 };
  fmsg.id = vmsg.id = gmetric_uint;
  fmsg.Ganglia_metadata_msg_u.gfull.metric.type = "uint32";
  vmsg.Ganglia_value_msg_u.gu_int.metric_id.name = SFLOWGMetricTable[tag].mname;
  vmsg.Ganglia_value_msg_u.gu_int.ui = val;
  vmsg.Ganglia_value_msg_u.gu_int.fmt = SFLOWGMetricTable[tag].format;
  process_sflow_gmetric(hostdata, tag, &fmsg, &vmsg);
}

static void
process_sflow_string(Ganglia_host *hostdata, EnumSFLOWGMetric tag, const char *val)
{
  Ganglia_metadata_msg fmsg = { 0 };
  Ganglia_value_msg vmsg = { 0 };
  fmsg.id = vmsg.id = gmetric_uint;
  fmsg.Ganglia_metadata_msg_u.gfull.metric.type = "string";
  vmsg.Ganglia_value_msg_u.gstr.metric_id.name = SFLOWGMetricTable[tag].mname;
  vmsg.Ganglia_value_msg_u.gstr.str = (char *)val;
  vmsg.Ganglia_value_msg_u.gstr.fmt = SFLOWGMetricTable[tag].format;
  process_sflow_gmetric(hostdata, tag, &fmsg, &vmsg);
}
  
bool_t
process_sflow_datagram(apr_sockaddr_t *remotesa, char *buf, apr_size_t len, apr_time_t now, char **errorMsg)
{
  SFlowXDR x = { 0 };
  uint32_t offset_HID=0;
  uint32_t offset_CPU=0;
  uint32_t offset_MEM=0;
  uint32_t offset_DSK=0;
  uint32_t offset_NIO=0;
  bool_t counterDeltas = TRUE;
  uint32_t samples, s;
  char agentipstr[SFLOW_MAX_IPSTR_LEN];
  char hostname[SFLOW_MAX_HOSTNAME_LEN + 1];
  char spoofstr[SFLOW_MAX_SPOOFHOST_LEN + 1];
  char osrelease[SFLOW_MAX_OSRELEASE_LEN + 1];
  uint32_t machine_type, os_name;
  Ganglia_value_msg vmsg = { 0 };
  Ganglia_host *hostdata;
  uint32_t lostDatagrams, lostSamples;
  apr_time_t ctr_ival_mS;

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
  x.agentAddr.type = SFLOWXDR_next(x);
  switch(x.agentAddr.type) {
  case SFLOW_ADDRTYPE_IP4:
    x.agentAddr.a.ip4 = SFLOWXDR_next_n(x);
    break;
  case SFLOW_ADDRTYPE_IP6:
    x.agentAddr.a.ip6[0] = SFLOWXDR_next_n(x);
    x.agentAddr.a.ip6[1] = SFLOWXDR_next_n(x);
    x.agentAddr.a.ip6[2] = SFLOWXDR_next_n(x);
    x.agentAddr.a.ip6[3] = SFLOWXDR_next_n(x);
    break;
  default:
    *errorMsg="bad agent address type";
    return FALSE;
    break;
  }
  x.agentSubId = SFLOWXDR_next(x);
  x.datagramSeqNo = SFLOWXDR_next(x);
  x.uptime_mS = SFLOWXDR_next(x);
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
	x.csSeqNo = SFLOWXDR_next(x);
	if(sType == SFLOW_COUNTERS_SAMPLE_EXPANDED) {
	  x.dsClass = SFLOWXDR_next(x);
	  x.dsIndex = SFLOWXDR_next(x);
	}
	else {
	  uint32_t dsCombined = SFLOWXDR_next(x);
	  x.dsClass = dsCombined >> 24;
	  x.dsIndex = dsCombined & 0x00FFFFFF;
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
	  if(x.dsClass == SFLOW_DSCLASS_PHYSICAL_ENTITY) {
	    /* only care about certain structures from certain data-sources.
	     * Just record their offsets here. We'll read the fields out below */
	    switch(ceTag) {
	    case SFLOW_COUNTERBLOCK_HOST_HID: offset_HID = SFLOWXDR_mark(x,0); break;
	    case SFLOW_COUNTERBLOCK_HOST_CPU: offset_CPU = SFLOWXDR_mark(x,0); break;
	    case SFLOW_COUNTERBLOCK_HOST_MEM: offset_MEM = SFLOWXDR_mark(x,0); break;
	    case SFLOW_COUNTERBLOCK_HOST_DSK: offset_DSK = SFLOWXDR_mark(x,0); break;
	    case SFLOW_COUNTERBLOCK_HOST_NIO: offset_NIO = SFLOWXDR_mark(x,0); break;
	    }
	    }
	  SFLOWXDR_skip(x,ceQuads);
	  
	  if(!SFLOWXDR_markOK(x,ceMark)) {
	    *errorMsg="parse error: counter element overrun/underrun";
	    return FALSE;
	  }
	}
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

  /* turn the agent ip into a string */
  if(my_inet_ntop(x.agentAddr.type == SFLOW_ADDRTYPE_IP6 ? AF_INET6 : AF_INET,
		  &x.agentAddr.a,
		  agentipstr,
		  SFLOW_MAX_HOSTNAME_LEN) == NULL) {
    *errorMsg =  "sFlow agent address format error";
    return FALSE;
  }

  if(offset_HID == 0 &&
     offset_CPU == 0 &&
     offset_MEM == 0 &&
     offset_DSK == 0 &&
     offset_NIO == 0) {
    /* this sflow datagram does not have any of
     * the structures we are looking for, so just
     * return. */
    return TRUE;
  }

  /* system details */
  hostname[0]='\0';
  spoofstr[0]='\0';
  osrelease[0]='\0';
  machine_type = 0;
  os_name = 0;

  
  if(offset_HID) {
    uint32_t hostname_len;
    uint32_t osrelease_len;
    /* We got a host-id structure.  Use this to set the "spoof" field.
     * The other fields like osrelease can be picked out here and we'll
     * submit them later once we have the hostdata context. */
    SFLOWXDR_setc(x,offset_HID);
    hostname_len = SFLOWXDR_next(x);
    if(hostname_len > 0 && hostname_len <= SFLOW_MAX_HOSTNAME_LEN) {
      memcpy(hostname, SFLOWXDR_str(x), hostname_len);
      hostname[hostname_len] = '\0';
      /* spoof options requires "<ip>:<hostname>"
       * question: we might want to add the dsIndex here too, in case
       * we ever see an sFlow agent reporting on more than one physical
       * entity?  In that case we would want to turn off the datagram
       * sequence number checking below,  and rely only on the datasource
       * sequence numbers */
      snprintf(spoofstr,SFLOW_MAX_SPOOFHOST_LEN, "%s:%s", agentipstr, hostname);
      vmsg.Ganglia_value_msg_u.gstr.metric_id.host = spoofstr;
      vmsg.Ganglia_value_msg_u.gstr.metric_id.spoof = TRUE;
    }
    SFLOWXDR_skip_b(x,hostname_len);
    SFLOWXDR_skip(x,4); /* skip UUID */
    machine_type = SFLOWXDR_next(x);
    os_name = SFLOWXDR_next(x);
    osrelease_len = SFLOWXDR_next(x);
    if(osrelease_len > 0 && osrelease_len <= SFLOW_MAX_OSRELEASE_LEN) {
      memcpy(osrelease, SFLOWXDR_str(x), osrelease_len);
      osrelease[osrelease_len] = '\0';
    }
  }

  /* lookup the ganglia host */
  hostdata = Ganglia_host_get(agentipstr, remotesa, &(vmsg.Ganglia_value_msg_u.gstr.metric_id)); 
  if(!hostdata) {
    *errorMsg = "host_get failed";
    return FALSE;
  }
  
  /* hang our counter and sequence number state onto the hostdata context */
  if(!hostdata->sflow) {
    hostdata->sflow = apr_pcalloc(hostdata->pool, sizeof(SFlowCounterState));
    hostdata->sflow->last_sample_time = now;
    counterDeltas = FALSE;
    hostdata->sflow->dsIndex = x.dsIndex;
    /* indicate when we started this */
    hostdata->gmond_started = apr_time_as_msec(now) / 1000;
  }
  
  /* check sequence numbers */
  lostDatagrams = x.datagramSeqNo - hostdata->sflow->datagramSeqNo - 1;
  lostSamples = x.csSeqNo - hostdata->sflow->csSeqNo - 1;
  if(x.datagramSeqNo == 1
     || x.csSeqNo == 1
     || lostDatagrams > SFLOW_MAX_LOST_SAMPLES
     || lostSamples > SFLOW_MAX_LOST_SAMPLES) {
    /* Agent or Sampler was restarted, or we lost too many in a row,
     * so just latch the counters this time without submitting deltas.*/
    counterDeltas = FALSE;
  }
  hostdata->sflow->datagramSeqNo = x.datagramSeqNo;
  hostdata->sflow->csSeqNo = x.csSeqNo;

  /* make sure all strings are persistent, so that we don't have to
   * rely on process_sflow_string() or Ganglia_value_save() making
   * a copy in the appropriate pool.  For now that only applies to
   * osrelease */
  if(hostdata->sflow->osrelease) {
    if(strcmp(hostdata->sflow->osrelease, osrelease) != 0) {
      /* it changed */
      free(hostdata->sflow->osrelease);
      hostdata->sflow->osrelease = NULL;
    }
  }
  if(hostdata->sflow->osrelease == NULL) {
    hostdata->sflow->osrelease = strdup(osrelease);
  }
  
  /* we'll need the time (in mS) since the last sample (use mS rather than
   * seconds to limit the effect of rounding/truncation in integer arithmetic) */
  ctr_ival_mS = apr_time_as_msec(now - hostdata->sflow->last_sample_time);
  hostdata->sflow->last_sample_time = now;

  /* convenience macros for handling counters */
#define SFLOW_CTR_LATCH(field) hostdata->sflow->field = field
#define SFLOW_CTR_DELTA(field) (field - hostdata->sflow->field)
#define SFLOW_CTR_RATE(field) (SFLOW_CTR_DELTA(field) * 1000 / ctr_ival_mS)

  /* metrics may be marked as "unsupported" by the sender,  so check for those reserved values */
#define SFLOW_OK_FLOAT(field) (field != (float)-1)
#define SFLOW_OK_GAUGE32(field) (field != (uint32_t)-1)
#define SFLOW_OK_GAUGE64(field) (field != (uint64_t)-1)
#define SFLOW_OK_COUNTER32(field) (field != (uint32_t)-1)
#define SFLOW_OK_COUNTER64(field) (field != (uint64_t)-1)

  /* always send a heartbeat */
  process_sflow_uint32(hostdata, SFLOW_M_heartbeat, (apr_time_as_msec(now) - x.uptime_mS) / 1000);
  
  if(offset_HID) {
    /* sumbit the system fields that we already extracted above */
    if(hostdata->sflow->osrelease && hostdata->sflow->osrelease[0] != '\0') {
      process_sflow_string(hostdata, SFLOW_M_os_release, hostdata->sflow->osrelease);
    }
    if(machine_type <= SFLOW_MAX_MACHINE_TYPE) {
      process_sflow_string(hostdata, SFLOW_M_machine_type, SFLOWMachineTypes[machine_type]);
    }
    if(os_name <= SFLOW_MAX_OS_NAME) {
      process_sflow_string(hostdata, SFLOW_M_os_name, SFLOWOSNames[os_name]);
    }
  }
  
  if(offset_CPU) {
    float load_one,load_five,load_fifteen;
    uint32_t proc_run,proc_total,cpu_num,cpu_speed,cpu_uptime,cpu_user,cpu_nice,cpu_system,cpu_idle,cpu_wio,cpu_intr,cpu_sintr;
    SFLOWXDR_setc(x,offset_CPU);
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
    /* skip interrupts, contexts */

    if(SFLOW_OK_FLOAT(load_one))     process_sflow_float(hostdata, SFLOW_M_load_one, load_one);
    if(SFLOW_OK_FLOAT(load_five))    process_sflow_float(hostdata, SFLOW_M_load_five, load_five);
    if(SFLOW_OK_FLOAT(load_fifteen)) process_sflow_float(hostdata, SFLOW_M_load_fifteen, load_fifteen);
    if(SFLOW_OK_GAUGE32(proc_run))   process_sflow_uint32(hostdata, SFLOW_M_proc_run, proc_run);
    if(SFLOW_OK_GAUGE32(proc_total)) process_sflow_uint32(hostdata, SFLOW_M_proc_total, proc_total);
    if(SFLOW_OK_GAUGE32(cpu_num))    process_sflow_uint16(hostdata, SFLOW_M_cpu_num, cpu_num);
    if(SFLOW_OK_GAUGE32(cpu_speed))  process_sflow_uint32(hostdata, SFLOW_M_cpu_speed, cpu_speed);
    if(SFLOW_OK_GAUGE32(cpu_uptime)) process_sflow_uint32(hostdata, SFLOW_M_cpu_boottime, (apr_time_as_msec(now) / 1000) - cpu_uptime);

    if(counterDeltas) {
      uint32_t delta_cpu_user =   SFLOW_CTR_DELTA(cpu_user);
      uint32_t delta_cpu_nice =   SFLOW_CTR_DELTA(cpu_nice);
      uint32_t delta_cpu_system = SFLOW_CTR_DELTA(cpu_system);
      uint32_t delta_cpu_idle =   SFLOW_CTR_DELTA(cpu_idle);
      uint32_t delta_cpu_wio =    SFLOW_CTR_DELTA(cpu_wio);
      uint32_t delta_cpu_intr =   SFLOW_CTR_DELTA(cpu_intr);
      uint32_t delta_cpu_sintr =  SFLOW_CTR_DELTA(cpu_sintr);
      
      uint32_t cpu_total = \
	delta_cpu_user +
	delta_cpu_nice +
	delta_cpu_system +
	delta_cpu_idle +
	delta_cpu_wio +
	delta_cpu_intr +
	delta_cpu_sintr;

#define SFLOW_CTR_CPU_PC(field) (cpu_total ? ((float)field * 100.0 / (float)cpu_total) : 0)
      if(SFLOW_OK_COUNTER32(cpu_user))   process_sflow_float(hostdata, SFLOW_M_cpu_user, SFLOW_CTR_CPU_PC(delta_cpu_user));
      if(SFLOW_OK_COUNTER32(cpu_nice))   process_sflow_float(hostdata, SFLOW_M_cpu_nice, SFLOW_CTR_CPU_PC(delta_cpu_nice));
      if(SFLOW_OK_COUNTER32(cpu_system)) process_sflow_float(hostdata, SFLOW_M_cpu_system, SFLOW_CTR_CPU_PC(delta_cpu_system));
      if(SFLOW_OK_COUNTER32(cpu_idle))   process_sflow_float(hostdata, SFLOW_M_cpu_idle, SFLOW_CTR_CPU_PC(delta_cpu_idle));
      if(SFLOW_OK_COUNTER32(cpu_wio))    process_sflow_float(hostdata, SFLOW_M_cpu_wio, SFLOW_CTR_CPU_PC(delta_cpu_wio));
      if(SFLOW_OK_COUNTER32(cpu_intr))   process_sflow_float(hostdata, SFLOW_M_cpu_intr, SFLOW_CTR_CPU_PC(delta_cpu_intr));
      if(SFLOW_OK_COUNTER32(cpu_sintr))  process_sflow_float(hostdata, SFLOW_M_cpu_sintr, SFLOW_CTR_CPU_PC(delta_cpu_sintr));
    }
    
    SFLOW_CTR_LATCH(cpu_user);
    SFLOW_CTR_LATCH(cpu_nice);
    SFLOW_CTR_LATCH(cpu_system);
    SFLOW_CTR_LATCH(cpu_idle);
    SFLOW_CTR_LATCH(cpu_wio);
    SFLOW_CTR_LATCH(cpu_intr);
    SFLOW_CTR_LATCH(cpu_sintr);
  }
  
  if(offset_MEM) {
    uint64_t mem_total,mem_free,mem_shared,mem_buffers,mem_cached,swap_total,swap_free;
    SFLOWXDR_setc(x,offset_MEM);
    SFLOWXDR_next_int64(x,&mem_total);
    SFLOWXDR_next_int64(x,&mem_free);
    SFLOWXDR_next_int64(x,&mem_shared);
    SFLOWXDR_next_int64(x,&mem_buffers);
    SFLOWXDR_next_int64(x,&mem_cached);
    SFLOWXDR_next_int64(x,&swap_total);
    SFLOWXDR_next_int64(x,&swap_free);
    /* skip page_in, page_out, swap_in, swap_out */
#define SFLOW_MEM_KB(bytes) (float)(bytes / 1000)
    if(SFLOW_OK_GAUGE64(mem_total))   process_sflow_float(hostdata, SFLOW_M_mem_total, SFLOW_MEM_KB(mem_total));
    if(SFLOW_OK_GAUGE64(mem_free))    process_sflow_float(hostdata, SFLOW_M_mem_free, SFLOW_MEM_KB(mem_free));
    if(SFLOW_OK_GAUGE64(mem_shared))  process_sflow_float(hostdata, SFLOW_M_mem_shared, SFLOW_MEM_KB(mem_shared));
    if(SFLOW_OK_GAUGE64(mem_buffers)) process_sflow_float(hostdata, SFLOW_M_mem_buffers, SFLOW_MEM_KB(mem_buffers));
    if(SFLOW_OK_GAUGE64(mem_cached))  process_sflow_float(hostdata, SFLOW_M_mem_cached, SFLOW_MEM_KB(mem_cached));
    if(SFLOW_OK_GAUGE64(swap_total))  process_sflow_float(hostdata, SFLOW_M_swap_total, SFLOW_MEM_KB(swap_total));
    if(SFLOW_OK_GAUGE64(swap_free))   process_sflow_float(hostdata, SFLOW_M_swap_free, SFLOW_MEM_KB(swap_free));
  }
  
  if(offset_DSK) {
    uint64_t disk_total, disk_free;
    uint32_t part_max_used;
    SFLOWXDR_setc(x,offset_DSK);
    SFLOWXDR_next_int64(x,&disk_total);
    SFLOWXDR_next_int64(x,&disk_free);
    part_max_used = SFLOWXDR_next(x);
    /* skip reads, bytes_read, read_time, writes, bytes_written, write_time */
    /* convert bytes to GB (1024*1024*1024=1073741824) */
    if(SFLOW_OK_GAUGE64(disk_total))    process_sflow_double(hostdata, SFLOW_M_disk_total, (double)disk_total / 1073741824.0);
    if(SFLOW_OK_GAUGE64(disk_free))     process_sflow_double(hostdata, SFLOW_M_disk_free, (double)disk_free / 1073741824.0);
    if(SFLOW_OK_GAUGE32(part_max_used)) process_sflow_float(hostdata, SFLOW_M_part_max_used, (float)part_max_used / 100.0);
  }
  
  if(offset_NIO) {
    uint64_t bytes_in, bytes_out;
    uint32_t pkts_in, pkts_out;
    SFLOWXDR_setc(x,offset_NIO);
    SFLOWXDR_next_int64(x,&bytes_in);
    pkts_in =  SFLOWXDR_next(x);
    SFLOWXDR_skip(x,2); /* skip errs_in, drops_in */
    SFLOWXDR_next_int64(x,&bytes_out);
    pkts_out = SFLOWXDR_next(x);
    /* skip errs_out, drops_out */
    
    if(counterDeltas) {
      if(SFLOW_OK_GAUGE64(bytes_in))  process_sflow_float(hostdata, SFLOW_M_bytes_in, SFLOW_CTR_RATE(bytes_in));
      if(SFLOW_OK_GAUGE32(pkts_in))   process_sflow_float(hostdata, SFLOW_M_pkts_in, SFLOW_CTR_RATE(pkts_in));
      if(SFLOW_OK_GAUGE64(bytes_out)) process_sflow_float(hostdata, SFLOW_M_bytes_out, SFLOW_CTR_RATE(bytes_out));
      if(SFLOW_OK_GAUGE32(pkts_out))  process_sflow_float(hostdata, SFLOW_M_pkts_out, SFLOW_CTR_RATE(pkts_out));
    }
    
    SFLOW_CTR_LATCH(bytes_in);
    SFLOW_CTR_LATCH(pkts_in);
    SFLOW_CTR_LATCH(bytes_out);
    SFLOW_CTR_LATCH(pkts_out);
  }
  
  return TRUE;
}
