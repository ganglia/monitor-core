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
  CFG_STR("mcast_ip", NULL, CFGF_NONE),
  CFG_STR("mcast_if", NULL, CFGF_NONE),
  CFG_STR("ip", NULL, CFGF_NONE ),
  CFG_INT("port", -1, CFGF_NONE ),
  CFG_STR("protocol", "xdr", CFGF_NONE),
  CFG_END()
};

static cfg_opt_t udp_recv_channel_opts[] = {
  CFG_STR("mcast_ip", NULL, CFGF_NONE ),
  CFG_STR("bind", NULL, CFGF_NONE ),
  CFG_INT("port", -1, CFGF_NONE ),
  CFG_STR("mcast_if", NULL, CFGF_NONE),
  CFG_STR("protocol", "xdr", CFGF_NONE),
  CFG_END()
};

static cfg_opt_t xml_out_channel_opts[] = {
  CFG_STR("bind", NULL, CFGF_NONE ),
  CFG_INT("port", -1, CFGF_NONE ),
  CFG_STR("interface", NULL, CFGF_NONE),
  CFG_END()
};


static cfg_opt_t gmond_opts[] = {
  CFG_SEC("location",  location_opts, CFGF_NONE),
  CFG_SEC("identification",  id_opts, CFGF_NONE),
  CFG_SEC("behavior",     behavior_opts, CFGF_NONE), 
  CFG_SEC("udp_send_channel", udp_send_channel_opts, CFGF_MULTI),
  CFG_SEC("udp_recv_channel", udp_recv_channel_opts, CFGF_MULTI),
  CFG_SEC("xml_out_channel", xml_out_channel_opts, CFGF_MULTI),
  CFG_END()
}; 

#endif /* CONF_H */
