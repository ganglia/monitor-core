#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include "conf.h"
#include <apr_pools.h>
#include <apr_strings.h>

char *default_gmond_configuration = NULL;

#define BASE_GMOND_CONFIGURATION "\
/* This configuration is as close to 2.5.x default behavior as possible \n\
   The values closely match ./gmond/metric.h definitions in 2.5.x */ \n\
globals {                    \n\
  setuid = no                \n\
  user = nobody              \n\
} \n\
\n\
/* Feel free to specify as many udp_send_channels as you like.  Gmond \n\
   used to only support having a single channel */ \n\
udp_send_channel { \n\
  mcast_join = 239.2.11.71 \n\
  port = 8649 \n\
} \n\
\n\
/* You can specify as many udp_recv_channels as you like as well. */ \n\
udp_recv_channel { \n\
  mcast_join = 239.2.11.71 \n\
  port = 8649 \n\
  bind = 239.2.11.71 \n\
} \n\
\n\
/* You can specify as many tcp_accept_channels as you like to share \n\
   an xml description of the state of the cluster */ \n\
tcp_accept_channel { \n\
  port = 8649 \n\
} \n\
\n\
\n\
/* The old internal 2.5.x metric array has been replaced by the following \n\
   collection_group directives.  What follows is the default behavior for \n\
   collecting and sending metrics that is as close to 2.5.x behavior as \n\
   possible. */\n\
\n\
/* This collection group will cause a heartbeat (or beacon) to be sent every \n\
   20 seconds.  In the heartbeat is the GMOND_STARTED data which expresses \n\
   the age of the running gmond. */ \n\
collection_group { \n\
  collect_once = yes \n\
  time_threshold = 20 \n\
  metric { \n\
    name = \"heartbeat\" \n\
  } \n\
} \n\
\n\
/* This collection group will send general info about this host every 1200 secs. \n\
   This information doesn't change between reboots and is only collected once. */ \n\
collection_group { \n\
  collect_once = yes \n\
  time_threshold = 1200 \n\
  metric { \n\
    name = \"cpu_num\" \n\
  } \n\
  metric { \n\
    name = \"cpu_speed\" \n\
  } \n\
  metric { \n\
    name = \"mem_total\" \n\
  } \n\
  /* Should this be here? Swap can be added/removed between reboots. */ \n\
  metric { \n\
    name = \"swap_total\" \n\
  } \n\
  metric { \n\
    name = \"boottime\" \n\
  } \n\
  metric { \n\
    name = \"machine_type\" \n\
  } \n\
  metric { \n\
    name = \"os_name\" \n\
  } \n\
  metric { \n\
    name = \"os_release\" \n\
  } \n\
  metric { \n\
    name = \"location\" \n\
  } \n\
} \n\
\n\
/* This collection group will send the status of gexecd for this host every 300 secs */\n\
/* Unlike 2.5.x the default behavior is to report gexecd OFF.  */ \n\
collection_group { \n\
  collect_once = yes \n\
  time_threshold = 300 \n\
  metric { \n\
    name = \"gexec\" \n\
  } \n\
} \n\
\n\
/* This collection group will collect the CPU and load status info every 20 secs. \n\
   The time threshold is set to 90 seconds.  In honesty, this time_threshold could be \n\
   set significantly higher to reduce unneccessary network chatter. */ \n\
collection_group { \n\
  collect_every = 20 \n\
  time_threshold = 90 \n\
  /* CPU status */ \n\
  metric { \n\
    name = \"cpu_user\"  \n\
    value_threshold = \"1.0\" \n\
  } \n\
  metric { \n\
    name = \"cpu_system\"   \n\
    value_threshold = \"1.0\" \n\
  } \n\
  metric { \n\
    name = \"cpu_idle\"  \n\
    value_threshold = \"5.0\" \n\
  } \n\
  metric { \n\
    name = \"cpu_nice\"  \n\
    value_threshold = \"1.0\" \n\
  } \n\
  metric { \n\
    name = \"cpu_aidle\" \n\
    value_threshold = \"5.0\" \n\
  } \n\
  /* Load Averages */ \n\
  metric { \n\
    name = \"load_one\" \n\
    value_threshold = \"1.0\" \n\
  } \n\
  metric { \n\
    name = \"load_five\" \n\
    value_threshold = \"1.0\" \n\
  } \n\
  metric { \n\
    name = \"load_fifteen\" \n\
    value_threshold = \"1.0\" \n\
  }\n\
} \n\
\n\
/* This group collects the number of running and total processes */ \n\
collection_group { \n\
  collect_every = 80 \n\
  time_threshold = 950 \n\
  metric { \n\
    name = \"proc_run\" \n\
    value_threshold = \"1.0\" \n\
  } \n\
  metric { \n\
    name = \"proc_total\" \n\
    value_threshold = \"1.0\" \n\
  } \n\
}\n\
\n\
/* This collection group grabs the volatile memory metrics every 40 secs and \n\
   sends them at least every 80 secs.  This time_threshold can be increased \n\
   significantly to reduce unneeded network traffic. */ \n\
collection_group { \n\
  collect_every = 40 \n\
  time_threshold = 180 \n\
  metric { \n\
    name = \"mem_free\" \n\
    value_threshold = \"1024.0\" \n\
  } \n\
  metric { \n\
    name = \"mem_shared\" \n\
    value_threshold = \"1024.0\" \n\
  } \n\
  metric { \n\
    name = \"mem_buffers\" \n\
    value_threshold = \"1024.0\" \n\
  } \n\
  metric { \n\
    name = \"mem_cached\" \n\
    value_threshold = \"1024.0\" \n\
  } \n\
  metric { \n\
    name = \"swap_free\" \n\
    value_threshold = \"1024.0\" \n\
  } \n\
} \n\
\n\
"

#define SOLARIS_SPECIFIC_CONFIGURATION "\
/* solaris specific metrics begin */ \n\
collection_group { \n\
  collect_every = 950 \n\
  time_threshold = 3800 \n\
  metric { \n\
    name = \"cpu_wio\" \n\
    value_threshold = \"5.0\" \n\
  } \n\
} \n\
\n\
collection_group { \n\
  collect_every = 20 \n\
  time_threshold = 90 \n\
  metric { \n\
   name = \"rcache\" \n\
   value_threshold = 1.0 \n\
  } \n\
  metric { \n\
   name = \"wcache\" \n\
   value_threshold = 1.0 \n\
  } \n\
  metric { \n\
    name = \"phread_sec\" \n\
    value_threshold = 1.0 \n\
  } \n\
  metric { \n\
    name = \"phwrite_sec\" \n\
    value_threshold = 1.0 \n\
  }\n\
}\n\
/* end solaris specific metrics */ \n\
\n\
"
#define LINUX_FREEBSD_COMMON_CONFIG "\
collection_group { \n\
  collect_every = 40 \n\
  time_threshold = 300 \n\
  metric { \n\
    name = \"bytes_out\" \n\
    value_threshold = 4096 \n\
  } \n\
  metric { \n\
    name = \"bytes_in\" \n\
    value_threshold = 4096 \n\
  } \n\
  metric { \n\
    name = \"pkts_in\" \n\
    value_threshold = 256 \n\
  } \n\
  metric { \n\
    name = \"pkts_out\" \n\
    value_threshold = 256 \n\
  } \n\
}\n\
\n\
/* Different than 2.5.x default since the old config made no sense */ \n\
collection_group { \n\
  collect_every = 1800 \n\
  time_threshold = 3600 \n\
  metric { \n\
    name = \"disk_total\" \n\
    value_threshold = 1.0 \n\
  } \n\
}\n\
\n\
collection_group { \n\
  collect_every = 40 \n\
  time_threshold = 180 \n\
  metric { \n\
    name = \"disk_free\" \n\
    value_threshold = 1.0 \n\
  } \n\
  metric { \n\
    name = \"part_max_used\" \n\
    value_threshold = 1.0 \n\
  } \n\
}\n\
\n\
"

#define HPUX_SPECIFIC_CONFIGURATION "\n\
collection_group { \n\
  collect_every = 20 \n\
  time_threshold = 90 \n\
  metric { \n\
    name = \"cpu_intr\" \n\
    value_threshold = 1.0 \n\
  } \n\
  metric { \n\
    name = \"cpu_ssys\" \n\
    value_threshold = 1.0 \n\
  } \n\
  metric { \n\
    name = \"cpu_wait\" \n\
    value_threshold = 1.0 \n\
  } \n\
} \n\
\n\
collection_group { \n\
  collect_every = 40 \n\
  time_threshold = 90 \n\
  metric { \n\
    name = \"mem_arm\" \n\
    value_threshold = 1024.0 \n\
  } \n\
  metric { \n\
    name = \"mem_rm\" \n\
    value_threshold = 1024.0 \n\
  } \n\
  metric { \n\
    name = \"mem_avm\" \n\
    value_threshold = 1024.0 \n\
  } \n\
  metric { \n\
    name = \"mem_vm\" \n\
    value_threshold = 1024.0 \n\
  } \n\
}\n\
\n\
"

void
build_default_gmond_configuration(apr_pool_t *context)
{
  default_gmond_configuration = apr_pstrdup(context, BASE_GMOND_CONFIGURATION);
#if SOLARIS
  default_gmond_configuration = apr_pstrcat(context, default_gmond_configuration, SOLARIS_SPECIFIC_CONFIGURATION, NULL);
#endif
#if LINUX || FREEBSD
  default_gmond_configuration = apr_pstrcat(context, default_gmond_configuration, LINUX_FREEBSD_COMMON_CONFIG, NULL);
#endif
#if HPUX
  default_gmond_configuration = apr_pstrcat(context, default_gmond_configuration, HPUX_SPECIFIC_CONFIGURATION, NULL);
#endif
}


