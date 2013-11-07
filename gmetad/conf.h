#ifndef CONFIG_H
#define CONFIG_H 1
#include <stdlib.h>
#include <string.h>
#include "llist.h"

#define MAX_RRAS 32
typedef struct
   {
      char *gridname;
      int xml_port;
      int interactive_port;
      int server_threads;
      int umask;
      llist_entry *trusted_hosts;
      int unsummarized_sflow_vm_metrics;
      llist_entry *unsummarized_metrics;
      int debug_level;
      int should_setuid;
      char *setuid_username;
      char *rrd_rootdir;
      char *carbon_server;
      int carbon_port;
      char *carbon_protocol;
      int carbon_timeout;
      char *memcached_parameters;
      char *graphite_prefix;
      char *graphite_path;
      int scalable_mode;
      int write_rrds;
      int all_trusted;
      int num_RRAs;
      char *RRAs[MAX_RRAS];
      char *riemann_server;
      int riemann_port;
      char *riemann_protocol;
      char *riemann_attributes;
      int case_sensitive_hostnames;
      int shortest_step;
} gmetad_config_t;

int get_gmetad_config(char *conffile);

#endif
