enum gangliaValueTypes {
  GANGLIA_VALUE_UNKNOWN,
  GANGLIA_VALUE_STRING, 
  GANGLIA_VALUE_UNSIGNED_SHORT,
  GANGLIA_VALUE_SIGNED_SHORT,
  GANGLIA_VALUE_UNSIGNED_INT,
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
  GANGLIA_METRIC_ALERT,
  GANGLIA_METRIC_UNKNOWN_OLD_FORMAT
};

struct gangliaMessageHeader {
  gangliaHostinfo *host;
  gangliaMsginfo  *msg;
};

struct gangliaMessageBody {
  unsigned int source_instance;
  gangliaDataState state;
  unsigned int age;
  unsigned int step;
  string units<>;
  gangliaMetric metrics<>;
};

struct gangliaFormat_1 {
  gangliaMessageHeader *hdr;
  gangliaMessageBody   *bdy;
};

/* 2.5.x compatibility..... */
struct gmetricMessage {
  opaque type<>;
  opaque name<>;
  opaque values<>;
  opaque units<>;
  unsigned int slope;
  unsigned int tmax;
  unsigned int dmax;
};

/* This is based on the old key_metrics.h file given #ifdef LINUX.
   All other operating system metric keys have been placed at the end
   of the list.  This breaks backward compatibility for those platforms
   unfortunately.
*/
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
   metric_bytes_out,
   metric_bytes_in,
   metric_pkts_in,
   metric_pkts_out,
   metric_disk_total,
   metric_disk_free,
   metric_part_max_used,
   GANGLIA_NUM_OLD_METRICS, /* this should always directly follow the last 25 metric */

   GANGLIA_FORMAT_1 = 2874789, /* give a little space for old metrics in case they are modified */

   /* insert new formats here. */

   GANGLIA_NUM_FORMATS     /* this should always be the last entry */
};

union gangliaMessage switch (gangliaFormats format) {
  case GANGLIA_FORMAT_1:
    gangliaFormat_1 *format_1; 

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
  case metric_bytes_in:      /* xdr_float */
  case metric_bytes_out:     /* xdr_float */
  case metric_pkts_in:       /* xdr_float */
  case metric_pkts_out:      /* xdr_float */
  case metric_part_max_used: /* xdr_float */
    float f;

  case metric_disk_total:    /* xdr_double */
  case metric_disk_free:     /* xdr_double */
    double d;

  default:
    void;
};

#ifdef RPC_HDR
%
%
%struct gangliaOldMetric /* this is for limited 2.5.x compatibility ... */
%{
%   int key;                /* the unique key for this metric.
%                              matches position in metric array and enum. */
%   const char name[16];    /* the name of the metric */
%   int step;               /* how often is the metric collected */
%   gangliaValueTypes type; /* the type of data to expect */
%   char units[32];         /* units the value are in */
%};
%typedef struct gangliaOldMetric gangliaOldMetric;
%
% gangliaOldMetric *gangliaOldMetric_get( int key );
%
#endif
#ifdef RPC_XDR
%
% gangliaOldMetric *
% gangliaOldMetric_get( int key )
% {
% static gangliaOldMetric gangliaOldMetricArray[GANGLIA_NUM_OLD_METRICS] = {
%   {   metric_user_defined, "gmetric",          0,    GANGLIA_VALUE_UNKNOWN,  ""  },
%   {   metric_cpu_num,      "cpu_num",         -1,    GANGLIA_VALUE_UNSIGNED_SHORT,   "CPUs"       },
%   {   metric_cpu_speed,    "cpu_speed",       -1,    GANGLIA_VALUE_UNSIGNED_INT,     "MHz"        },
%   {   metric_mem_total,    "mem_total",       -1,    GANGLIA_VALUE_UNSIGNED_INT,     "KB"         },
%   {   metric_swap_total,   "swap_total",      -1,    GANGLIA_VALUE_UNSIGNED_INT,     "KB"         },
%   {   metric_boottime,     "boottime",        -1,    GANGLIA_VALUE_UNSIGNED_INT,     "s"          },
%   {   metric_sys_clock,    "sys_clock",       -1,    GANGLIA_VALUE_UNSIGNED_INT,     "s"          },
%   {   metric_machine_type, "machine_type",    -1,    GANGLIA_VALUE_STRING,           ""           },
%   {   metric_os_name,      "os_name",         -1,    GANGLIA_VALUE_STRING,           ""           },
%   {   metric_os_release,   "os_release",      -1,    GANGLIA_VALUE_STRING,           ""           },
%   {   metric_cpu_user,     "cpu_user",        20,    GANGLIA_VALUE_FLOAT,            "%"          },
%   {   metric_cpu_nice,     "cpu_nice",        20,    GANGLIA_VALUE_FLOAT,            "%"          },
%   {   metric_cpu_system,   "cpu_system",      20,    GANGLIA_VALUE_FLOAT,            "%"          },
%   {   metric_cpu_idle,     "cpu_idle",        20,    GANGLIA_VALUE_FLOAT,            "%"          },
%   {   metric_cpu_aidle,    "cpu_aidle",      950,    GANGLIA_VALUE_FLOAT,            "%"          },
%   {   metric_load_one,     "load_one",        20,    GANGLIA_VALUE_FLOAT,            ""           },
%   {   metric_load_five,    "load_five",       40,    GANGLIA_VALUE_FLOAT,            ""           },
%   {   metric_load_fifteen, "load_fifteen",    80,    GANGLIA_VALUE_FLOAT,            ""           },
%   {   metric_proc_run,     "proc_run",        80,    GANGLIA_VALUE_UNSIGNED_INT,     ""           },
%   {   metric_proc_total,   "proc_total",      80,    GANGLIA_VALUE_UNSIGNED_INT,     ""           },
%   {   metric_mem_free,     "mem_free",        40,    GANGLIA_VALUE_UNSIGNED_INT,     "KB"         },
%   {   metric_mem_shared,   "mem_shared",      40,    GANGLIA_VALUE_UNSIGNED_INT,     "KB"         },
%   {   metric_mem_buffers,  "mem_buffers",     40,    GANGLIA_VALUE_UNSIGNED_INT,     "KB"         },
%   {   metric_mem_cached,   "mem_cached",      40,    GANGLIA_VALUE_UNSIGNED_INT,     "KB"         },
%   {   metric_swap_free,    "swap_free",       40,    GANGLIA_VALUE_UNSIGNED_INT,     "KB"         },
%   {   metric_gexec,        "gexec",           -1,    GANGLIA_VALUE_STRING,           ""           },
%   {   metric_heartbeat,    "heartbeat",       -1,    GANGLIA_VALUE_UNSIGNED_INT,     ""           },
%   {   metric_mtu,          "mtu",           1200,    GANGLIA_VALUE_UNSIGNED_INT,     ""           },
%   {   metric_location,     "location",        -1,    GANGLIA_VALUE_STRING,           "(x,y,z)"    },
%   {   metric_bytes_out,    "bytes_out",       40,    GANGLIA_VALUE_FLOAT,            "bytes/sec"  },
%   {   metric_bytes_in,     "bytes_in",        40,    GANGLIA_VALUE_FLOAT,            "bytes/sec"  },
%   {   metric_pkts_in,      "pkts_in",         40,    GANGLIA_VALUE_FLOAT,            "packets/sec"},
%   {   metric_pkts_out,     "pkts_out",        40,    GANGLIA_VALUE_FLOAT,            "packets/sec"},
%   {   metric_disk_total,   "disk_total",    3600,    GANGLIA_VALUE_DOUBLE,           "GB"         },
%   {   metric_disk_free,    "disk_free",       40,    GANGLIA_VALUE_DOUBLE,           "GB"         },
%   {   metric_part_max_used,"part_max_used",   40,    GANGLIA_VALUE_FLOAT,            "%"          }
% };
%
% if(key < 0 || key >= GANGLIA_NUM_OLD_METRICS)
%   {
%     return NULL;
%   }
% return &gangliaOldMetricArray[key];
% }
#endif



           





