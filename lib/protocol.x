
const MAXSTRINGLEN = 1400;
/* For gmetric messages */
const MAXTYPELEN =  16;
/*
 * Max multicast message: 1500 bytes (Ethernet max frame size)
 * minus 20 bytes for IPv4 header, minus 8 bytes for UDP header.
 */
const MAXMCASTMSG = 1472;
const FRAMESIZE =   1400;
const MAXUNITSLEN = 16;

/* All format prefixed with metric are for 2.5 information */
enum gangliaFormats {
   metric_user_defined,
   metric_cpu_num,
   metric_cpu_speed,
   metric_mem_total,
   metric_swap_total,
   metric_boottime,
   metric_sys_clock,
   metric_machine_type,
   metric_os_name,
   metric_os_release,
   metric_cpu_user,
   metric_cpu_nice,
   metric_cpu_system,
   metric_cpu_idle,
   metric_cpu_aidle,
   metric_load_one,
   metric_load_five,
   metric_load_fifteen,
   metric_proc_run,
   metric_proc_total,
   metric_mem_free,
   metric_mem_shared,
   metric_mem_buffers,
   metric_mem_cached,
   metric_swap_free,
   metric_gexec,
   metric_heartbeat,
   metric_mtu,
   metric_location,
   metric_cpu_wio,
   metric_bread_sec,
   metric_bwrite_sec,
   metric_lread_sec,
   metric_lwrite_sec,
   metric_phread_sec,
   metric_phwrite_sec,
   metric_rcache,
   metric_wcache,
   metric_bytes_in,
   metric_bytes_out,
   metric_pkts_in,
   metric_pkts_out,
   metric_disk_total,
   metric_disk_free,
   metric_part_max_used,
   metric_cpu_intr,
   metric_cpu_ssys,
   metric_cpu_wait,
   metric_mem_arm,
   metric_mem_rm,
   metric_mem_avm,
   metric_mem_vm,
   GANGLIA_METRIC_GROUP,
/* Insert new message formats here */
   
   MAX_NUM_GANGLIA_FORMATS  /* Make sure this is always the last in the enum */
};

struct gmetricMessage {
  opaque type<MAXTYPELEN>;
  opaque name<MAXMCASTMSG>;
  opaque values<FRAMESIZE>;
  opaque units<MAXUNITSLEN>;
  unsigned int slope;
  unsigned int tmax;
  unsigned int dmax;
};

/* Ganglia Message Format Version 1 */
enum gangliaValueTypes_1 {
  GANGLIA_VALUE_STRING_1,
  GANGLIA_VALUE_INT_1,
  GANGLIA_VALUE_FLOAT_1,
  GANGLIA_VALUE_DOUBLE_1
};

typedef string varstring<>;

union gangliaValue_1 switch(gangliaValueTypes_1 type) {
  case GANGLIA_VALUE_STRING_1:
     string str<>;
  case GANGLIA_VALUE_INT_1:
     int i;
  case GANGLIA_VALUE_FLOAT_1:
     float f;
  case GANGLIA_VALUE_DOUBLE_1:
     double d;
  default:
     void;
};

struct gangliaNamedValue_1 {
  string name<>;
  gangliaValue_1 value;
};

enum gangliaSlopeTypes_1 {
  GANGLIA_SLOPE_ZERO_1,
  GANGLIA_SLOPE_POSITIVE_1,
  GANGLIA_SLOPE_NEGATIVE_1,
  GANGLIA_SLOPE_BOTH_1,
  GANGLIA_SLOPE_UNSPECIFIED_1
};

enum gangliaMetricQualities_1 {
  GANGLIA_QUALITY_NORMAL_1,
  GANGLIA_QUALITY_WARNING_1,
  GANGLIA_QUALITY_ALERT_1
};

struct gangliaMetricGroup {
  string units<>;
  int collected;
  unsigned int tn;
  unsigned int tmax;
  unsigned int dmax;
  gangliaSlopeTypes_1 slope;
  gangliaMetricQualities_1 quality;
  gangliaNamedValue_1 data<>;
};
/* End Ganglia Message Format Version 1 */

union gangliaMessage switch (gangliaFormats format) {
  case GANGLIA_METRIC_GROUP:
    gangliaMetricGroup *group;
 
  /* 2.5.x sources... */
  case metric_user_defined:
    gmetricMessage gmetric;

  case metric_cpu_num:       /* xdr_u_short */
    unsigned short u_short;

  case metric_cpu_speed:     /* xdr_u_int */
  case metric_mem_total:     /* xdr_u_int */
  case metric_swap_total:    /* xdr_u_int */
  case metric_boottime:      /* xdr_u_int */
  case metric_sys_clock:     /* xdr_u_int */
  case metric_proc_run:      /* xdr_u_int */
  case metric_proc_total:    /* xdr_u_int */
  case metric_mem_free:      /* xdr_u_int */
  case metric_mem_shared:    /* xdr_u_int */
  case metric_mem_buffers:   /* xdr_u_int */
  case metric_mem_cached:    /* xdr_u_int */
  case metric_swap_free:     /* xdr_u_int */
  case metric_heartbeat:     /* xdr_u_int */
  case metric_mtu:           /* xdr_u_int */
  case metric_mem_arm:       /* xdr_u_int */
  case metric_mem_rm:        /* xdr_u_int */
  case metric_mem_avm:       /* xdr_u_int */
  case metric_mem_vm:        /* xdr_u_int */
    unsigned int u_int;  

  case metric_machine_type:  /* xdr_string */
  case metric_os_name:       /* xdr_string */
  case metric_os_release:    /* xdr_string */
  case metric_gexec:         /* xdr_string */
  case metric_location:      /* xdr_string */
    string str<MAXSTRINGLEN>;

  case metric_cpu_user:      /* xdr_float */
  case metric_cpu_nice:      /* xdr_float */
  case metric_cpu_system:    /* xdr_float */
  case metric_cpu_idle:      /* xdr_float */
  case metric_cpu_aidle:     /* xdr_float */
  case metric_load_one:      /* xdr_float */
  case metric_load_five:     /* xdr_float */
  case metric_load_fifteen:  /* xdr_float */
  case metric_cpu_wio:       /* xdr_float */
  case metric_bread_sec:     /* xdr_float */
  case metric_bwrite_sec:    /* xdr_float */
  case metric_lread_sec:     /* xdr_float */
  case metric_lwrite_sec:    /* xdr_float */
  case metric_phread_sec:    /* xdr_float */
  case metric_phwrite_sec:   /* xdr_float */
  case metric_rcache:        /* xdr_float */ 
  case metric_wcache:        /* xdr_float */
  case metric_bytes_in:      /* xdr_float */
  case metric_bytes_out:     /* xdr_float */
  case metric_pkts_in:       /* xdr_float */
  case metric_pkts_out:      /* xdr_float */
  case metric_part_max_used: /* xdr_float */
  case metric_cpu_intr:      /* xdr_float */
  case metric_cpu_ssys:      /* xdr_float */
  case metric_cpu_wait:      /* xdr_float */
    float f;

  case metric_disk_total:    /* xdr_double */
  case metric_disk_free:     /* xdr_double */
    double d;

  default:
    void;

};
