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
      int debug_level;
      int should_setuid;
      char *setuid_username;
      char *rrd_rootdir;
      int scalable_mode;
      int all_trusted;
      int num_RRAs;
      char *RRAs[MAX_RRAS];
} gmetad_config_t;

int get_gmetad_config(char *conffile);

#endif
