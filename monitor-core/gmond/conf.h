#ifndef CONF_H
#define CONF_H 1

/***** IMPORTANT ************
Any changes that you make to this file need to be reconciled in ./conf.pod
in order for the documentation to be in order with the code 
****************************/

#include "confuse.h"

#define DEFAULT_GMOND_CONFIGURATION "\
globals {                    \n\
  setuid = no                \n\
  user = nobody              \n\
} \n\
udp_send_channel { \n\
  mcast_join = 239.2.11.71 \n\
  port = 8649 \n\
} \n\
udp_recv_channel { \n\
  mcast_join = 239.2.11.71 \n\
  port = 8649 \n\
  bind = 239.2.11.71 \n\
} \n\
tcp_accept_channel { \n\
  port = 8649 \n\
} \n\
collection_group { \n\
  collect_once = yes \n\
  time_threshold = 20 \n\
  metric { \n\
    name = \"heartbeat\" \n\
  } \n\
} \n\
collection_group { \n\
  collect_every = 60 \n\
  metric { \n\
    name = \"cpu_user\"  \n\
  } \n\
  metric { \n\
    name = \"cpu_system\"   \n\
  } \n\
  metric { \n\
    name = \"cpu_idle\"  \n\
  } \n\
  metric { \n\
    name = \"cpu_nice\"  \n\
  } \n\
} \n\
"

static cfg_opt_t cluster_opts[] = {
  CFG_STR("name", NULL, CFGF_NONE ),
  CFG_STR("owner", NULL, CFGF_NONE ),
  CFG_STR("latlong", NULL, CFGF_NONE ),
  CFG_STR("url", NULL, CFGF_NONE ),
  CFG_END()
};

static cfg_opt_t globals_opts[] = {
  CFG_BOOL("daemonize", 1, CFGF_NONE),
  CFG_BOOL("setuid", 1, CFGF_NONE),
  CFG_STR("user", "nobody", CFGF_NONE),
  /* later i guess we should add "group" as well */
  CFG_INT("debug_level", 0, CFGF_NONE),
  CFG_INT("max_udp_msg_len", 1472, CFGF_NONE),
  CFG_BOOL("mute", 0, CFGF_NONE),
  CFG_BOOL("deaf", 0, CFGF_NONE),
  CFG_INT("host_dmax", 0, CFGF_NONE),
  CFG_END()
};

static cfg_opt_t udp_send_channel_opts[] = {
  CFG_STR("mcast_join", NULL, CFGF_NONE),
  CFG_STR("mcast_if", NULL, CFGF_NONE),
  CFG_STR("ip", NULL, CFGF_NONE ),
  CFG_INT("port", -1, CFGF_NONE ),
  CFG_END()
};

static cfg_opt_t udp_recv_channel_opts[] = {
  CFG_STR("mcast_join", NULL, CFGF_NONE ),
  CFG_STR("bind", NULL, CFGF_NONE ),
  CFG_INT("port", -1, CFGF_NONE ),
  CFG_STR("mcast_if", NULL, CFGF_NONE),
  CFG_STR("allow_ip", NULL, CFGF_NONE),
  CFG_STR("allow_mask", NULL, CFGF_NONE),
  CFG_END()
};

static cfg_opt_t tcp_accept_channel_opts[] = {
  CFG_STR("bind", NULL, CFGF_NONE ),
  CFG_INT("port", -1, CFGF_NONE ),
  CFG_STR("interface", NULL, CFGF_NONE),
  CFG_STR("allow_ip", NULL, CFGF_NONE),
  CFG_STR("allow_mask", NULL, CFGF_NONE),
  CFG_END()
};

static cfg_opt_t metric_opts[] = {
  CFG_STR("name", NULL, CFGF_NONE ),
  CFG_FLOAT("value_threshold", -1, CFGF_NONE),
  CFG_END()
};

static cfg_opt_t collection_group_opts[] = {
  CFG_STR("name", NULL, CFGF_NONE),
  CFG_SEC("metric", metric_opts, CFGF_MULTI),
  CFG_BOOL("collect_once", 0, CFGF_NONE),  
  CFG_INT("collect_every", 60, CFGF_NONE),    
  CFG_INT("time_threshold", 3600, CFGF_NONE),    /* tmax */
  CFG_INT("lifetime", 0, CFGF_NONE),             /* dmax */
  CFG_END()
};

static cfg_opt_t gmond_opts[] = {
  CFG_SEC("cluster",   cluster_opts, CFGF_NONE),
  CFG_SEC("globals",     globals_opts, CFGF_NONE), 
  CFG_SEC("udp_send_channel", udp_send_channel_opts, CFGF_MULTI),
  CFG_SEC("udp_recv_channel", udp_recv_channel_opts, CFGF_MULTI),
  CFG_SEC("tcp_accept_channel", tcp_accept_channel_opts, CFGF_MULTI),
  CFG_SEC("collection_group",  collection_group_opts, CFGF_MULTI),
  CFG_END()
}; 

#endif /* CONF_H */
