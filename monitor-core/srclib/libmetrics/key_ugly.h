#ifndef KEY_UGLY_H
#define KEY_UGLY_H 1

/*
 * All behavior of gmond is tweak'd by this array..
 *
 * Each metric is checked during some random interval between "check_min" and
 * "check_max" (assuming the threshold value is not negative).  If the metric is
 * "threshold" greater than the previous metric value then the key_metric
 * (listed on the left) is multicast.  If there has been no multicast
 * during some random interval from "mcast_min" to "mcast_max" then a multicast
 * is sent regardless of the metric value.
 *
 */
#define INIT 0, 0, {0}, {0}
#define KEY(NAME) { #NAME, NAME ##_func, INIT

metric_t metric[num_key_metrics] = {
/*             threshold    check_max  mcast_max       units        */
/* key               check_min   mcast_min     type             fmt */
/*------------------------------------------------------------------*/

/* user_defined does nothing */
{"user_defined",metric_init,
                  INIT,  -1,  -1,   -1,   -1,   -1, g_int32, "",     ""},

/* CONFIGURATION METRICS */
KEY(cpu_num),      -1,  -1,   -1,  900, 1200, g_uint16, "","%hu"},
KEY(cpu_speed),    -1,  -1,   -1,  900, 1200, g_uint32, "MHz", "%hu"},
KEY(mem_total),    -1,  -1,   -1,  900, 1200, g_uint32, "KB", "%u" },
KEY(swap_total),   -1,  -1,   -1,  900, 1200, g_uint32, "KB", "%u" },
KEY(boottime),     -1,  -1,   -1,  900, 1200, g_timestamp, "s",    "%u" },
KEY(sys_clock),    -1,  -1,   -1,  900, 1200, g_timestamp, "s",    "%u" },
KEY(machine_type), -1,  -1,   -1,  900, 1200, g_string, "",    "%s" },
KEY(os_name),      -1,  -1,   -1,  900, 1200, g_string, "",    "%s" },
KEY(os_release),   -1,  -1,   -1,  900, 1200, g_string, "",    "%s" },

/* STATE METRICS */
KEY(cpu_user),      1,  15,   20,   60,   90, g_float, "%",   "%.1f"},
KEY(cpu_nice),      1,  15,   20,   60,   90, g_float, "%",   "%.1f"},
KEY(cpu_system),    1,  15,   20,   60,   90, g_float, "%",   "%.1f"},
KEY(cpu_idle),      5,  15,   20,   60,   90, g_float, "%",   "%.1f"},
KEY(cpu_wio),       1,  15,   20,   60,   90, g_float, "%",   "%.1f"},
KEY(cpu_aidle),     5, 850,  950, 3400, 3800, g_float, "%",   "%.1f"},
KEY(load_one),      1,  15,   20,   50,   70, g_float, "",    "%.2f"},
KEY(load_five),     1,  30,   40,  275,  325, g_float, "",    "%.2f"},
KEY(load_fifteen),  1,  60,   80,  850,  950, g_float, "",    "%.2f"},
KEY(proc_run),      1,  45,   80,  850,  950, g_uint32, "","%u"},
KEY(proc_total),    1,  45,   80,  850,  950, g_uint32, "","%u"},
KEY(mem_free),   1024,  30,   40,  120,  180, g_uint32, "KB", "%u" },
KEY(mem_shared), 1024,  30,   40,  120,  180, g_uint32, "KB", "%u" },
KEY(mem_buffers),1024,  30,   40,  120,  180, g_uint32, "KB", "%u" },
KEY(mem_cached), 1024,  30,   40,  120,  180, g_uint32, "KB", "%u" },
KEY(swap_free),  1024,  30,   40,  120,  180, g_uint32, "KB", "%u" },

/* gmond internals */
KEY(gexec),        -1,  -1,   -1,  180,  300, g_string, "",    "%s"},
KEY(heartbeat),    -1,  -1,   -1,   10,   20, g_uint32, "",    "%u"},
KEY(mtu), -1, -1,  -1, 900, 1200, g_uint32, "B", "%u" },
KEY(location), -1, -1,  -1, 900, 1200, g_string, "(x,y,z)", "%s" }

#ifdef SOLARIS
,
/*
 * (sacerdoti) Experiments have shown gmon resting bandwidth is around
 * 2KB/s for a 128-node cluster. We set the value thresh of these
 * metrics to double that.
 *
 */
KEY(bytes_out),  4096, 30,   40,  200,  300, g_float, "bytes/sec", "%.2f" },
KEY(bytes_in),   4096, 30,   40,  200,  300, g_float, "bytes/sec", "%.2f" },
KEY(pkts_in), 256, 30,   40,  200,  300, g_float, "packets/sec", "%.2f" },
KEY(pkts_out),   256, 30,   40,  200,  300, g_float, "packets/sec", "%.2f" },

/* buffer reads and writes, adjusted per second */

KEY(bread_sec),   1,  15,   20,  60,  90, g_float, "", "%.2f" },
KEY(bwrite_sec), 1,  15,   20,  60,  90, g_float, "", "%.2f" },

/* linear reads/writes, adjusted per second */

KEY(lread_sec),1,  15,   20,  60,  90, g_float, "", "%.2f" },
KEY(lwrite_sec), 1,  15,   20,  60,  90, g_float, "", "%.2f" },

/* cache hit ratios, rcache = 1 - (bread/lread), wcache = 1 - (bwrite/lwrite) */

KEY(rcache), 1,  15,   20,  60,  90, g_float, "%", "%.1f" },
KEY(wcache), 1,  15,   20,  60,  90, g_float, "%", "%.1f" },

/* physical reads/writes per second. */

KEY(phread_sec), 1,  15,   20,  60,  90, g_float, "", "%.2f" },
KEY(phwrite_sec), 1,  15,   20,  60,  90, g_float, "", "%.2f" }

#endif

#ifdef LINUX
,
/* (sacerdoti) Experiments have shown gmon resting bandwidth is around 2KB/s for
 * a 128-node cluster. We set the value thresh of these metrics to double that.
 */
KEY(bytes_out),  4096, 30,   40,  200,  300, g_float, "bytes/sec", "%.2f" },
KEY(bytes_in),   4096, 30,   40,  200,  300, g_float, "bytes/sec", "%.2f" },
KEY(pkts_in), 256, 30,   40,  200,  300, g_float, "packets/sec", "%.2f" },
KEY(pkts_out),   256, 30,   40,  200,  300, g_float, "packets/sec", "%.2f" },

/*
 * The amount of disk space could change - hot-swap, mounts, etc. check: 30-60min. */
KEY(disk_total), 1, 1800, 3600, 900, 1200, g_double, "GB", "%.3f" },
KEY(disk_free), 1, 30, 40, 120, 180, g_double, "GB", "%.3f" },
KEY(part_max_used), 1, 30, 40, 120, 180, g_float, "%", "%.1f" },
KEY(cpu_intr),      1,  15,   20,   60,   90, g_float, "%",   "%.1f"},
KEY(cpu_sintr),     1,  15,   20,   60,   90, g_float, "%",   "%.1f"}

#endif

#ifdef CYGWIN
,
/*
 * (sacerdoti) Experiments have shown gmon resting bandwidth is around
 * 2KB/s for a 128-node cluster. We set the value thresh of these
 * metrics to double that.
 *
 */
KEY(bytes_out),  4096, 30,   40,  200,  300, g_float, "bytes/sec", "%.2f" },
KEY(bytes_in),   4096, 30,   40,  200,  300, g_float, "bytes/sec", "%.2f" },
KEY(pkts_in), 256, 30,   40,  200,  300, g_float, "packets/sec", "%.2f" },
KEY(pkts_out),   256, 30,   40,  200,  300, g_float, "packets/sec", "%.2f" },
KEY(cpu_intr),      1,  15,   20,   60,   90, g_float, "%",   "%.1f"},
KEY(cpu_sintr),     1,  15,   20,   60,   90, g_float, "%",   "%.1f"}

#endif

#ifdef HPUX
,
KEY(cpu_intr),      1,  15,   20,   60,   90, g_float, "%",   "%.1f"},
KEY(cpu_sintr),      1,  15,   20,   60,   90, g_float, "%",   "%.1f"},
KEY(mem_arm),   1024,  30,   40,  120,  180, g_uint32, "KB", "%u" },
KEY(mem_rm),   1024,  30,   40,  120,  180, g_uint32, "KB", "%u" },
KEY(mem_avm),   1024,  30,   40,  120,  180, g_uint32, "KB", "%u" },
KEY(mem_vm),   1024,  30,   40,  120,  180, g_uint32, "KB", "%u" }

#endif

#ifdef FREEBSD

,
/*
 * (sacerdoti) Experiments have shown gmon resting bandwidth is around
 * 2KB/s for a 128-node cluster. We set the value thresh of these
 * metrics to double that.
 *
 */
KEY(bytes_out),  4096, 30,   40,  200,  300, g_float, "bytes/sec", "%.2f" },
KEY(bytes_in),   4096, 30,   40,  200,  300, g_float, "bytes/sec", "%.2f" },
KEY(pkts_in), 256, 30,   40,  200,  300, g_float, "packets/sec", "%.2f" },
KEY(pkts_out),   256, 30,   40,  200,  300, g_float, "packets/sec", "%.2f" },

/*
 * The amount of disk space could change - hot-swap, mounts, etc.
 * check: 30-60min.
 */
KEY(disk_total), 1, 1800, 3600, 900, 1200, g_double, "GB", "%.3f" },
KEY(disk_free), 1, 30, 40, 120, 180, g_double, "GB", "%.3f" },
KEY(part_max_used), 1, 30, 40, 120, 180, g_float, "%", "%.1f" }

#endif

#ifdef IRIX
,
KEY(cpu_intr),      1,  15,   20,   60,   90, g_float, "%",   "%.1f"},

#endif

#ifdef AIX
#endif

#ifdef DARWIN
#endif

#ifdef OSF
#endif
};

#endif /* KEY_UGLY_H */
