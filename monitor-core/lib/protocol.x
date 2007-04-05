#define __GANGLIA_MTU     1500
#define __UDP_HEADER_SIZE 28
#define __MAX_DESC_LEN 128
%#define UDP_HEADER_SIZE __UDP_HEADER_SIZE
%#define MAX_DESC_LEN __MAX_DESC_LEN


#define GANGLIA_MAX_XDR_MESSAGE_LEN (__GANGLIA_MTU - __UDP_HEADER_SIZE - 8)

enum Ganglia_value_types {
  GANGLIA_VALUE_UNKNOWN,
  GANGLIA_VALUE_STRING, 
  GANGLIA_VALUE_UNSIGNED_SHORT,
  GANGLIA_VALUE_SHORT,
  GANGLIA_VALUE_UNSIGNED_INT,
  GANGLIA_VALUE_INT,
  GANGLIA_VALUE_FLOAT,
  GANGLIA_VALUE_DOUBLE
};

struct Ganglia_gmetric_message {
  string type<>;
  string name<>;
  string value<>;
  string units<>;
  unsigned int slope;
  unsigned int tmax;
  unsigned int dmax;
};

/* Yemi */
struct Ganglia_spoof_header {
  string spoofName<>;
  string spoofIP<>;
};
struct Ganglia_spoof_message {
  struct Ganglia_spoof_header spheader;
  struct Ganglia_gmetric_message gmetric;
};

/*
** When adding a new core metric you need to make three changes
** in this file:
**
** - Add a new entry to "Ganglia_message_formats", just before
**   GANGLIA_NUM_25_METRICS
** - Add the new enum entry to the "Ganglia_message" union/switch.
**   Failing to do so will lead to garbage in the reporting of
**   the new metric
** - Add a reporting string to "ganglia_25_metric_array". Put the
**   new string at the end.
**
** In addition do not forget a matching "Ganglia_metric_cb_define"
** in gmond/gmond.c
**
*/
enum Ganglia_message_formats {
   metric_user_defined = 0, /* gmetric message */
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
   metric_cpu_wio,
   metric_bread_sec,
   metric_bwrite_sec,
   metric_lread_sec,
   metric_lwrite_sec,
   metric_rcache,
   metric_wcache,
   metric_phread_sec,
   metric_phwrite_sec,
   metric_cpu_intr,
   metric_cpu_sintr,
   metric_mem_arm,
   metric_mem_rm,
   metric_mem_avm,
   metric_mem_vm,
   GANGLIA_NUM_25_METRICS, /* this should always directly follow the last 25 metric_* */
/* Yemi */
   spoof_metric = 4096,
   spoof_heartbeat = 4097,
   modular_metric = 4098
};

union Ganglia_message switch (Ganglia_message_formats id) {
  case metric_user_defined:
  case modular_metric:
    Ganglia_gmetric_message gmetric;
/* Yemi */
  case spoof_metric:
    Ganglia_spoof_message spmetric;
  case spoof_heartbeat:
    Ganglia_spoof_header spheader;
 
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
  case metric_bytes_in:      /* xdr_float */
  case metric_bytes_out:     /* xdr_float */
  case metric_pkts_in:       /* xdr_float */
  case metric_pkts_out:      /* xdr_float */
  case metric_part_max_used: /* xdr_float */
  case metric_cpu_wio:       /* xdr_float */
  case metric_bread_sec:     /* xdr_float */
  case metric_bwrite_sec:    /* xdr_float */
  case metric_lread_sec:     /* xdr_float */
  case metric_lwrite_sec:    /* xdr_float */
  case metric_rcache:        /* xdr_float */
  case metric_wcache:        /* xdr_float */
  case metric_phread_sec:    /* xdr_float */
  case metric_phwrite_sec:   /* xdr_float */
  case metric_cpu_intr:      /* xdr_float */
  case metric_cpu_sintr:     /* xdr_float */
    float f;

  case metric_disk_total:    /* xdr_double */
  case metric_disk_free:     /* xdr_double */
    double d;

  default:
    void;
};

struct Ganglia_25metric
{
   int key;
   string name<16>;
   int tmax;
   Ganglia_value_types type;
   string units<32>;
   string slope<32>;
   string fmt<32>;
   int msg_size;
   string desc<__MAX_DESC_LEN>;
};

#ifdef RPC_HDR
% #define GANGLIA_MAX_MESSAGE_LEN GANGLIA_MAX_XDR_MESSAGE_LEN
% Ganglia_25metric *Ganglia_25metric_bykey( int key );
% Ganglia_25metric *Ganglia_25metric_byname( char *name );
#endif

#ifdef RPC_XDR
% Ganglia_25metric ganglia_25_metric_array[GANGLIA_NUM_25_METRICS] = {
%  {metric_user_defined, "gmetric",       0,  GANGLIA_VALUE_UNKNOWN,        "",           "",      "",    0,                 "User Defined"},
%  {metric_cpu_num,      "cpu_num",    1200,  GANGLIA_VALUE_UNSIGNED_SHORT, "CPUs",       "zero",  "%hu", UDP_HEADER_SIZE+8, "Total number of CPUs"},
%  {metric_cpu_speed,    "cpu_speed",  1200,  GANGLIA_VALUE_UNSIGNED_INT,   "MHz",        "zero",  "%hu", UDP_HEADER_SIZE+8, "CPU Speed in terms of MHz"},
%  {metric_mem_total,    "mem_total",  1200,  GANGLIA_VALUE_UNSIGNED_INT,   "KB",         "zero",  "%u",  UDP_HEADER_SIZE+8, "Total amount of memory displayed in KBs"},
%  {metric_swap_total,   "swap_total", 1200,  GANGLIA_VALUE_UNSIGNED_INT,   "KB",         "zero",  "%u",  UDP_HEADER_SIZE+8, "Total amount of swap space displayed in KBs"},
%  {metric_boottime,     "boottime",   1200,  GANGLIA_VALUE_UNSIGNED_INT,   "s",          "zero",  "%u",  UDP_HEADER_SIZE+8, "The last time that the system was started"},
%  {metric_sys_clock,    "sys_clock",  1200,  GANGLIA_VALUE_UNSIGNED_INT,   "s",          "zero",  "%u",  UDP_HEADER_SIZE+8, "Time as reported by the system clock"},
%  {metric_machine_type, "machine_type",1200, GANGLIA_VALUE_STRING,         "",           "zero",  "%s",  UDP_HEADER_SIZE+32, "System architecture"},
%  {metric_os_name,      "os_name",    1200,  GANGLIA_VALUE_STRING,         "",           "zero",  "%s",  UDP_HEADER_SIZE+32, "Operating system name"},
%  {metric_os_release,   "os_release", 1200,  GANGLIA_VALUE_STRING,         "",           "zero",  "%s",  UDP_HEADER_SIZE+32, "Operating system release date"},
%  {metric_cpu_user,     "cpu_user",     90,  GANGLIA_VALUE_FLOAT,          "%",          "both",  "%.1f",UDP_HEADER_SIZE+8, "Percentage of CPU utilization that occurred while executing at the user level"},
%  {metric_cpu_nice,     "cpu_nice",     90,  GANGLIA_VALUE_FLOAT,          "%",          "both",  "%.1f",UDP_HEADER_SIZE+8, "Percentage of CPU utilization that occurred while executing at the user level with nice priority"},
%  {metric_cpu_system,   "cpu_system",   90,  GANGLIA_VALUE_FLOAT,          "%",          "both",  "%.1f",UDP_HEADER_SIZE+8, "Percentage of CPU utilization that occurred while executing at the system level"},
%  {metric_cpu_idle,     "cpu_idle",     90,  GANGLIA_VALUE_FLOAT,          "%",          "both",  "%.1f",UDP_HEADER_SIZE+8, "Percentage of time that the CPU or CPUs were idle and the system did not have an outstanding disk I/O request"},
%  {metric_cpu_aidle,    "cpu_aidle",  3800,  GANGLIA_VALUE_FLOAT,          "%",          "both",  "%.1f",UDP_HEADER_SIZE+8, "Percent of time since boot idle CPU"},
%  {metric_load_one,     "load_one",     70,  GANGLIA_VALUE_FLOAT,          " ",          "both",  "%.2f",UDP_HEADER_SIZE+8, "One minute load average"},
%  {metric_load_five,    "load_five",   325,  GANGLIA_VALUE_FLOAT,          " ",          "both",  "%.2f",UDP_HEADER_SIZE+8, "Five minute load average"},
%  {metric_load_fifteen, "load_fifteen",950,  GANGLIA_VALUE_FLOAT,          " ",          "both",  "%.2f",UDP_HEADER_SIZE+8, "Fifteen minute load average"},
%  {metric_proc_run,     "proc_run",    950,  GANGLIA_VALUE_UNSIGNED_INT,   " ",          "both",  "%u",  UDP_HEADER_SIZE+8, "Total number of running processes"},
%  {metric_proc_total,   "proc_total",  950,  GANGLIA_VALUE_UNSIGNED_INT,   " ",          "both",  "%u",  UDP_HEADER_SIZE+8, "Total number of processes"},
%  {metric_mem_free,     "mem_free",    180,  GANGLIA_VALUE_UNSIGNED_INT,   "KB",         "both",  "%u",  UDP_HEADER_SIZE+8, "Amount of available memory"},
%  {metric_mem_shared,   "mem_shared",  180,  GANGLIA_VALUE_UNSIGNED_INT,   "KB",         "both",  "%u",  UDP_HEADER_SIZE+8, "Amount of shared memory"},
%  {metric_mem_buffers,  "mem_buffers", 180,  GANGLIA_VALUE_UNSIGNED_INT,   "KB",         "both",  "%u",  UDP_HEADER_SIZE+8, "Amount of buffered memory"},
%  {metric_mem_cached,   "mem_cached",  180,  GANGLIA_VALUE_UNSIGNED_INT,   "KB",         "both",  "%u",  UDP_HEADER_SIZE+8, "Amount of cached memory"},
%  {metric_swap_free,    "swap_free",   180,  GANGLIA_VALUE_UNSIGNED_INT,   "KB",         "both",  "%u",  UDP_HEADER_SIZE+8, "Amount of available swap memory"},
%  {metric_gexec,        "gexec",       300,  GANGLIA_VALUE_STRING,         "",           "zero",  "%s",  UDP_HEADER_SIZE+32, "gexec available"},
%  {metric_heartbeat,    "heartbeat",    20,  GANGLIA_VALUE_UNSIGNED_INT,   "",           "",      "%u",  UDP_HEADER_SIZE+8, "Last heartbeat"},
%  {metric_mtu,          "mtu",        1200,  GANGLIA_VALUE_UNSIGNED_INT,   "",           "both",  "%u",  UDP_HEADER_SIZE+8, "Network maximum transmission unit"},
%  {metric_location,     "location",   1200,  GANGLIA_VALUE_STRING,         "(x,y,z)",    "",      "%s",  UDP_HEADER_SIZE+12, "Location of the machine"},
%  {metric_bytes_out,    "bytes_out",   300,  GANGLIA_VALUE_FLOAT,          "bytes/sec",  "both",  "%.2f",UDP_HEADER_SIZE+8, "Number of bytes out per second"},
%  {metric_bytes_in,     "bytes_in",    300,  GANGLIA_VALUE_FLOAT,          "bytes/sec",  "both",  "%.2f",UDP_HEADER_SIZE+8, "Number of bytes in per second"},
%  {metric_pkts_in,      "pkts_in",     300,  GANGLIA_VALUE_FLOAT,          "packets/sec","both",  "%.2f",UDP_HEADER_SIZE+8, "Packets in per second"},
%  {metric_pkts_out,     "pkts_out",    300,  GANGLIA_VALUE_FLOAT,          "packets/sec","both",  "%.2f",UDP_HEADER_SIZE+8, "Packets out per second"},
%  {metric_disk_total,   "disk_total", 1200,  GANGLIA_VALUE_DOUBLE,         "GB",         "both",  "%.3f",UDP_HEADER_SIZE+16, "Total available disk space"},
%  {metric_disk_free,    "disk_free",   180,  GANGLIA_VALUE_DOUBLE,         "GB",         "both",  "%.3f",UDP_HEADER_SIZE+16, "Total free disk space"},
%  {metric_part_max_used,"part_max_used",180, GANGLIA_VALUE_FLOAT,          "%",          "both",  "%.1f",UDP_HEADER_SIZE+8, "Maximum percent used for all partitions"},
%  {metric_cpu_wio,      "cpu_wio",       90, GANGLIA_VALUE_FLOAT,          "%",          "both",  "%.1f",UDP_HEADER_SIZE+8, "Percentage of time that the CPU or CPUs were idle during which the system had an outstanding disk I/O request"},
%  {metric_bread_sec,    "bread_sec",     90, GANGLIA_VALUE_FLOAT,          "1/sec",      "both",  "%.2f",UDP_HEADER_SIZE+8, "bread_sec)"},
%  {metric_bwrite_sec,   "bwrite_sec",    90, GANGLIA_VALUE_FLOAT,          "1/sec",      "both",  "%.2f",UDP_HEADER_SIZE+8, "bwrite_sec)"},
%  {metric_lread_sec,    "lread_sec",     90, GANGLIA_VALUE_FLOAT,          "1/sec",      "both",  "%.2f",UDP_HEADER_SIZE+8, "lread_sec)"},
%  {metric_lwrite_sec,   "lwrite_sec",    90, GANGLIA_VALUE_FLOAT,          "1/sec",      "both",  "%.2f",UDP_HEADER_SIZE+8, "lwrite_sec"},
%  {metric_rcache,       "rcache",        90, GANGLIA_VALUE_FLOAT,          "%",          "both",  "%.1f",UDP_HEADER_SIZE+8, "rcache"},
%  {metric_wcache,       "wcache",        90, GANGLIA_VALUE_FLOAT,          "%",          "both",  "%.1f",UDP_HEADER_SIZE+8, "wcache"},
%  {metric_phread_sec,   "phread_sec",    90, GANGLIA_VALUE_FLOAT,          "1/sec",      "both",  "%.2f",UDP_HEADER_SIZE+8, "phread_sec"},
%  {metric_phwrite_sec,  "phwrite_sec",   90, GANGLIA_VALUE_FLOAT,          "1/sec",      "both",  "%.2f",UDP_HEADER_SIZE+8, "phwrite_sec"},
%  {metric_cpu_intr,     "cpu_intr",      90, GANGLIA_VALUE_FLOAT,          "%",          "both",  "%.1f",UDP_HEADER_SIZE+8, "cpu_intr"},
%  {metric_cpu_sintr,    "cpu_sintr",     90, GANGLIA_VALUE_FLOAT,          "%",          "both",  "%.1f",UDP_HEADER_SIZE+8, "cpu_sintr"},
%  {metric_mem_arm,      "mem_arm",      180, GANGLIA_VALUE_UNSIGNED_INT,   "KB",         "both",  "%u",  UDP_HEADER_SIZE+8, "mem_arm"},
%  {metric_mem_rm,       "mem_rm",       180, GANGLIA_VALUE_UNSIGNED_INT,   "KB",         "both",  "%u",  UDP_HEADER_SIZE+8, "mem_rm"},
%  {metric_mem_avm,      "mem_avm",      180, GANGLIA_VALUE_UNSIGNED_INT,   "KB",         "both",  "%u",  UDP_HEADER_SIZE+8, "mem_avm"},
%  {metric_mem_vm,       "mem_vm",       180, GANGLIA_VALUE_UNSIGNED_INT,   "KB",         "both",  "%u",  UDP_HEADER_SIZE+8, "mem_vm"}
% };
%
% Ganglia_25metric *
% Ganglia_25metric_bykey( int key )
% {
%   if(key < 0 || key >= GANGLIA_NUM_25_METRICS)
%     {
%       return NULL;
%     }
%   return &ganglia_25_metric_array[key];
% }
% Ganglia_25metric *
% Ganglia_25metric_byname( char *name )
% {
%   int i;
%   if(!name)
%     {
%       return NULL;
%     }
%   for(i = 0; i< GANGLIA_NUM_25_METRICS; i++)
%     {
%       if(! strcasecmp(name, ganglia_25_metric_array[i].name))
%         {
%           return &ganglia_25_metric_array[i];
%         }
%     }
%   return NULL;
% }
#endif
