#ifndef CONFIG_H
#define CONFIG_H 1
#include <stdlib.h>
#include <string.h>

#include "lib/llist.h"

typedef struct
   {
      char *gridname;
      int xml_port;
      int xml_compression_level;
      int interactive_port;
      int server_threads;
      llist_entry *trusted_hosts;
      int debug_level;
      int should_setuid;
      char *setuid_username;
      char *rrd_rootdir;
      int scalable_mode;
      int all_trusted;
      int num_rras;
      char **rras;
} gmetad_config_t;

int get_gmetad_config(char *conffile);

#endif
