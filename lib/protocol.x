enum gangliaValueTypes {
  GANGLIA_VALUE_STRING, 
  GANGLIA_VALUE_INT,
  GANGLIA_VALUE_FLOAT,
  GANGLIA_VALUE_DOUBLE
};

typedef string varstring<>;

union gangliaValue switch(gangliaValueTypes type) {
  case GANGLIA_VALUE_STRING:
     string str<>;
  case GANGLIA_VALUE_INT:
     int i;
  case GANGLIA_VALUE_FLOAT:
     float f;
  case GANGLIA_VALUE_DOUBLE:
     double d;
  default:
     void;
};

struct gangliaMetric {
  string name<>;
  gangliaValue value;
};

/* Necessary for source to act as proxy */
struct gangliaHostinfo {
  string hostname<>;
  string ip<>;
};

/* Necessary for reliable UDP (later) */
struct gangliaMsginfo {
  unsigned int seq; 
  unsigned int timestamp;
};

enum gangliaDataState {
  GANGLIA_METRIC_CONSTANT,               /* slope == "zero" */
  GANGLIA_METRIC_TIME_THRESHOLD,         /* slope != "zero" here down */
  GANGLIA_METRIC_VALUE_THRESHOLD,
  GANGLIA_METRIC_WARNING,
  GANGLIA_METRIC_ALERT
};

struct gangliaNetinfo {
  gangliaHostinfo *host;
  gangliaMsginfo  *msg;
};

struct gangliaMessageHeader {
  unsigned int index;
};

struct gangliaMessageBody {
  unsigned int source_instance;
  gangliaNetinfo *net;
  gangliaDataState state;
  unsigned int age;
  unsigned int step;
  string units<>;
  gangliaMetric metrics<>;
};

struct gangliaMessage {
  gangliaMessageHeader hdr;
  gangliaMessageBody   bdy;
};

/* 2.5.x compatibility..... */
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

struct gmetricMessage {
  opaque type<MAXTYPELEN>;
  opaque name<MAXMCASTMSG>;
  opaque values<FRAMESIZE>;
  opaque units<MAXUNITSLEN>;
  unsigned int slope;
  unsigned int tmax;
  unsigned int dmax;
};

enum gangliaMetricIndex {
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
   max_num_25_metric_keys
};

union ganglia25Message switch (gangliaMetricIndex metric) {
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
    string str<>;

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
