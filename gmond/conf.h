#ifndef GMOND_CONFIG_H
#define GMOND_CONFIG_H 1
#include <stdlib.h>
#include <string.h>
#include "llist.h"

/* autoconf me later */
#define DEFAULT_GMOND_CONFIG_FILE "/etc/gmond.conf"

typedef struct
   {
      char *name;
      char *owner;
      char *latlong;
      char *url;
      char *location;
      char *mcast_channel;
      char *send_bind;
      int send_bind_given;
      unsigned short mcast_port;
      long int mcast_if_given;
      char *mcast_if;
      long int mcast_ttl;
      long int mcast_threads;
      unsigned short xml_port;
      long int xml_threads;
      llist_entry *trusted_hosts;
      long int num_nodes;
      long int num_custom_metrics;
      long int mute;
      long int deaf;
      long int debug_level;
      long int no_setuid;
      char *setuid;
      long int no_gexec;
      long int all_trusted;
      long int host_dmax;
} gmond_config_t;

int get_gmond_config( char *conffile);

#endif
