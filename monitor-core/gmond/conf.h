#ifndef CONF_H
#define CONF_H 1

#include "confuse.h"

#define DEFAULT_CONFIGURATION "\
behavior {                    \n\
  setuid = no                 \n\
  user = nobody               \n\
} \n\
udp_send_channel { \n\
  ip   = 127.0.0.1 \n\
  port = 8649 \n\
} \n\
udp_recv_channel { \n\
  port = 8649 \n\
} \n\
collection_group { \n\
  name = \"cpu_stat\" \n\
  metric { \n\
    name = \"cpu_user\"  \n\
    absolute_minimum = 0 \n\
  } \n\
  metric { \n\
    name = \"cpu_sys\"   \n\
  } \n\
  metric { \n\
    name = \"cpu_idle\"  \n\
  } \n\
  metric { \n\
    name = \"cpu_nice\"  \n\
  } \n\
} \n\
"

static cfg_opt_t location_opts[] = {
  CFG_INT("rack", -1, CFGF_NONE ),
  CFG_INT("rank", -1, CFGF_NONE ),
  CFG_INT("plane", -1, CFGF_NONE ),
  CFG_END()
};

static cfg_opt_t id_opts[] = {
  CFG_STR("cluster", NULL, CFGF_NONE ),
  CFG_STR("owner", NULL, CFGF_NONE ),
  CFG_STR("latitude", NULL, CFGF_NONE ),
  CFG_STR("longitude", NULL, CFGF_NONE ),
  CFG_STR("url", NULL, CFGF_NONE ),
  CFG_END()
};

static cfg_opt_t behavior_opts[] = {
  CFG_BOOL("daemonize", 1, CFGF_NONE),
  CFG_BOOL("setuid", 1, CFGF_NONE),
  CFG_STR("user", "nobody", CFGF_NONE),
  /* later i guess we should add "group" as well */
  CFG_INT("debug_level", 0, CFGF_NONE),
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
  CFG_STR("protocol", "xdr", CFGF_NONE),
  CFG_END()
};

static cfg_opt_t udp_recv_channel_opts[] = {
  CFG_STR("mcast_join", NULL, CFGF_NONE ),
  CFG_STR("bind", NULL, CFGF_NONE ),
  CFG_INT("port", -1, CFGF_NONE ),
  CFG_STR("mcast_if", NULL, CFGF_NONE),
  CFG_STR("protocol", "xdr", CFGF_NONE),
  CFG_END()
};

static cfg_opt_t tcp_accept_channel_opts[] = {
  CFG_STR("bind", NULL, CFGF_NONE ),
  CFG_INT("port", -1, CFGF_NONE ),
  CFG_STR("interface", NULL, CFGF_NONE),
  CFG_STR("protocol", "xml", CFGF_NONE),
  CFG_END()
};

int metric_validate_func(cfg_t *cfg, cfg_opt_t *opt);


static cfg_opt_t metric_opts[] = {
  CFG_BOOL( "absolute_minimum_given", 0, CFGF_NONE ),
  CFG_FLOAT("absolute_minimum", 0, CFGF_NONE ),
  CFG_BOOL( "absolute_minimum_alert_given", 0, CFGF_NONE ),
  CFG_FLOAT("absolute_minimum_alert", 0, CFGF_NONE ),
  CFG_BOOL( "absolute_minimum_warning_given", 0, CFGF_NONE ),
  CFG_FLOAT("absolute_minimum_warning", 0, CFGF_NONE ),
  CFG_BOOL( "absolute_maximum_warning_given", 0, CFGF_NONE ),
  CFG_FLOAT("absolute_maximum_warning", 0, CFGF_NONE ),
  CFG_BOOL( "absolute_maximum_alert_given", 0, CFGF_NONE ),
  CFG_FLOAT("absolute_maximum_alert", 0, CFGF_NONE ),
  CFG_BOOL( "absolute_maximum_given", 0, CFGF_NONE ),
  CFG_FLOAT("absolute_maximum", 0, CFGF_NONE ),
  CFG_BOOL( "relative_change_normal_given", 0, CFGF_NONE),
  CFG_FLOAT("relative_change_normal", 0, CFGF_NONE),
  CFG_BOOL( "relative_change_warning_given", 0, CFGF_NONE),
  CFG_FLOAT("relative_change_warning", 0, CFGF_NONE),
  CFG_BOOL( "relative_change_alert_given", 0, CFGF_NONE),
  CFG_FLOAT("relative_change_alert", 0, CFGF_NONE),
  CFG_STR("units", NULL, CFGF_NONE ),
  CFG_STR("name", NULL, CFGF_NONE ),
/*
  CFG_PTR_CB("value", NULL, CFGF_NONE ),
*/
  CFG_INT("current_state", 0, CFGF_NONE), /* high_alert, high_warning, normal, low_warning, low_alert */
  CFG_END()
};

static cfg_opt_t static_collection_group_opts[] = {
  CFG_STR("name", NULL, CFGF_NONE),
  CFG_INT("collection_interval", 60, CFGF_NONE),
  CFG_END()
};

/* Group with metrics that change... */
static cfg_opt_t collection_group_opts[] = {
  CFG_STR("name", NULL, CFGF_NONE),
  CFG_SEC("metric", metric_opts, CFGF_MULTI),
  CFG_INT("collection_interval", 60, CFGF_NONE),    
  CFG_INT("announce_interval", 3600, CFGF_NONE),    /* tmax */
  CFG_INT("lifetime", 0, CFGF_NONE),                /* dmax */
  CFG_END()
};


static cfg_opt_t gmond_opts[] = {
  CFG_SEC("location",  location_opts, CFGF_NONE),
  CFG_SEC("identification",  id_opts, CFGF_NONE),
  CFG_SEC("behavior",     behavior_opts, CFGF_NONE), 
  CFG_SEC("udp_send_channel", udp_send_channel_opts, CFGF_MULTI),
  CFG_SEC("udp_recv_channel", udp_recv_channel_opts, CFGF_MULTI),
  CFG_SEC("tcp_accept_channel", tcp_accept_channel_opts, CFGF_MULTI),
  CFG_SEC("static_collection_group", static_collection_group_opts, CFGF_MULTI),
  CFG_SEC("collection_group",  collection_group_opts, CFGF_MULTI),
  CFG_END()
}; 

#endif /* CONF_H */
