#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "ganglia.h"
#include "confuse.h"
#include "debug_msg.h"
#include <apr_pools.h>
#include <apr_strings.h>
#include <apr_tables.h>
#include <apr_net.h>

#include "protocol.h"  /* generated from ./lib/protocol.x */

/***** IMPORTANT ************
Any changes that you make to this file need to be reconciled in ./conf.pod
in order for the documentation to be in order with the code 
****************************/

void build_default_gmond_configuration(apr_pool_t *context);

static cfg_opt_t cluster_opts[] = {
  CFG_STR("name", NULL, CFGF_NONE ),
  CFG_STR("owner", NULL, CFGF_NONE ),
  CFG_STR("latlong", NULL, CFGF_NONE ),
  CFG_STR("url", NULL, CFGF_NONE ),
  CFG_END()
};

static cfg_opt_t host_opts[] = {
  CFG_STR("location", "unspecified", CFGF_NONE ),
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
  CFG_INT("cleanup_threshold", 300, CFGF_NONE),
  CFG_BOOL("gexec", 0, CFGF_NONE),
  CFG_END()
};

static cfg_opt_t access_opts[] = {
  CFG_STR("action", NULL, CFGF_NONE),
  CFG_STR("ip", NULL, CFGF_NONE),
  CFG_STR("mask", NULL, CFGF_NONE),
  CFG_END()
};

static cfg_opt_t acl_opts[] = {
  CFG_STR("default","allow", CFGF_NONE),
  CFG_SEC("access", access_opts, CFGF_MULTI ),
  CFG_END()
}; 

static cfg_opt_t udp_send_channel_opts[] = {
  CFG_STR("mcast_join", NULL, CFGF_NONE),
  CFG_STR("mcast_if", NULL, CFGF_NONE),
  CFG_STR("host", NULL, CFGF_NONE ),
  CFG_INT("port", -1, CFGF_NONE ),
  CFG_END()
};

static cfg_opt_t udp_recv_channel_opts[] = {
  CFG_STR("mcast_join", NULL, CFGF_NONE ),
  CFG_STR("bind", NULL, CFGF_NONE ),
  CFG_INT("port", -1, CFGF_NONE ),
  CFG_STR("mcast_if", NULL, CFGF_NONE),
  CFG_SEC("acl", acl_opts, CFGF_NONE), 
  CFG_END()
};

static cfg_opt_t tcp_accept_channel_opts[] = {
  CFG_STR("bind", NULL, CFGF_NONE ),
  CFG_INT("port", -1, CFGF_NONE ),
  CFG_STR("interface", NULL, CFGF_NONE),
  CFG_SEC("acl", acl_opts, CFGF_NONE),
  CFG_INT("timeout", 0, CFGF_NONE),
  CFG_END()
};

static cfg_opt_t metric_opts[] = {
  CFG_STR("name", NULL, CFGF_NONE ),
  CFG_FLOAT("value_threshold", -1, CFGF_NONE),
  CFG_END()
};

static cfg_opt_t collection_group_opts[] = {
  CFG_SEC("metric", metric_opts, CFGF_MULTI),
  CFG_BOOL("collect_once", 0, CFGF_NONE),  
  CFG_INT("collect_every", 60, CFGF_NONE),    
  CFG_INT("time_threshold", 3600, CFGF_NONE),    /* tmax */
  CFG_END()
};

static cfg_opt_t gmond_opts[] = {
  CFG_SEC("cluster",   cluster_opts, CFGF_NONE),
  CFG_SEC("host",      host_opts, CFGF_NONE),
  CFG_SEC("globals",     globals_opts, CFGF_NONE), 
  CFG_SEC("udp_send_channel", udp_send_channel_opts, CFGF_MULTI),
  CFG_SEC("udp_recv_channel", udp_recv_channel_opts, CFGF_MULTI),
  CFG_SEC("tcp_accept_channel", tcp_accept_channel_opts, CFGF_MULTI),
  CFG_SEC("collection_group",  collection_group_opts, CFGF_MULTI),
  CFG_END()
}; 

char *default_gmond_configuration = NULL;

#define BASE_GMOND_CONFIGURATION "\
/* This configuration is as close to 2.5.x default behavior as possible \n\
   The values closely match ./gmond/metric.h definitions in 2.5.x */ \n\
globals {                    \n\
  setuid = yes              \n\
  user = nobody              \n\
  cleanup_threshold = 300 /*secs */ \n\
} \n\
\n\
/* If a cluster attribute is specified, then all gmond hosts are wrapped inside \n\
 * of a <CLUSTER> tag.  If you do not specify a cluster tag, then all <HOSTS> will \n\
 * NOT be wrapped inside of a <CLUSTER> tag. */ \n\
cluster { \n\
  name = \"unspecified\" \n\
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
/* This collection group will collect the CPU status info every 20 secs. \n\
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
  metric { \n\
    name = \"cpu_wio\" \n\
    value_threshold = \"1.0\" \n\
  } \n\
  metric { \n\
    name = \"cpu_intr\" \n\
    value_threshold = \"1.0\" \n\
  } \n\
  metric { \n\
    name = \"cpu_sintr\" \n\
    value_threshold = \"1.0\" \n\
  } \n\
} \n\
\n\
collection_group { \n\
  collect_every = 20 \n\
  time_threshold = 90 \n\
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
#define HPUX_SPECIFIC_CONFIGURATION "\n\
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
#if HPUX
  default_gmond_configuration = apr_pstrcat(context, default_gmond_configuration, HPUX_SPECIFIC_CONFIGURATION, NULL);
#endif
}


#if 0
static void
cleanup_configuration_file(void)
{
  cfg_free( config_file );
}
#endif

int libgmond_apr_lib_initialized = 0;

Ganglia_pool
Ganglia_pool_create( Ganglia_pool parent )
{
  apr_status_t status;
  Ganglia_pool pool = NULL;

  if(!libgmond_apr_lib_initialized)
    {
      status = apr_initialize();
      if(status != APR_SUCCESS)
	{
	  return NULL;
	}
      libgmond_apr_lib_initialized = 1;
      atexit(apr_terminate);
    }

  status = apr_pool_create( &pool, parent );
  if(status != APR_SUCCESS)
    {
      return NULL;
    }
  return pool;
}

void
Ganglia_pool_destroy( Ganglia_pool pool )
{
  apr_pool_destroy(pool);
}

Ganglia_gmond_config
Ganglia_gmond_config_create(char *path, int fallback_to_default)
{
  Ganglia_gmond_config config = NULL;
  /* Make sure we process ~ in the filename if the shell doesn't */
  char *tilde_expanded = cfg_tilde_expand( path );
  config = cfg_init( gmond_opts, CFGF_NOCASE );

  switch( cfg_parse( config, tilde_expanded ) )
    {
    case CFG_FILE_ERROR:
      /* Unable to open file so we'll go with the configuration defaults */
      fprintf(stderr,"Configuration file '%s' not found.\n", tilde_expanded);
      if(!fallback_to_default)
	{
	  /* Don't fallback to the default configuration.. just exit. */
	  exit(1);
	}
      /* .. otherwise use our default configuration */
      if(cfg_parse_buf(config, default_gmond_configuration) == CFG_PARSE_ERROR)
	{
	  fprintf(stderr,"Your default configuration buffer failed to parse. Exiting.\n");
          exit(1);
	}
      break;
    case CFG_PARSE_ERROR:
      fprintf(stderr,"Parse error for '%s'\n", tilde_expanded );
      exit(1);
    case CFG_SUCCESS:
      break;
    default:
      /* I have no clue whats goin' on here... */
      exit(1);
    }

  if(tilde_expanded)
    free(tilde_expanded);

#if 0
  atexit(cleanup_configuration_file);
#endif
  return config;
}

Ganglia_udp_send_channels
Ganglia_udp_send_channels_create( Ganglia_pool context, Ganglia_gmond_config config )
{
  Ganglia_udp_send_channels send_channels = NULL;
  int i, num_udp_send_channels = cfg_size( config, "udp_send_channel");

  /* Return null if there are no send channels specified */
  if(num_udp_send_channels <= 0)
    return send_channels;

  /* Create my UDP send array */
  send_channels = apr_array_make( context, num_udp_send_channels, 
				   sizeof(apr_socket_t *));

  for(i = 0; i< num_udp_send_channels; i++)
    {
      cfg_t *udp_send_channel;
      char *mcast_join, *mcast_if, *host;
      int port;
      apr_socket_t *socket = NULL;
      apr_pool_t *pool = NULL;

      udp_send_channel = cfg_getnsec( config, "udp_send_channel", i);
      host           = cfg_getstr( udp_send_channel, "host" );
      mcast_join     = cfg_getstr( udp_send_channel, "mcast_join" );
      mcast_if       = cfg_getstr( udp_send_channel, "mcast_if" );
      port           = cfg_getint( udp_send_channel, "port");

      debug_msg("udp_send_channel mcast_join=%s mcast_if=%s host=%s port=%d\n",
		  mcast_join? mcast_join:"NULL", 
		  mcast_if? mcast_if:"NULL",
		  host,
		  port);

      /* Create a subpool */
      apr_pool_create(&pool, context);

      /* Join the specified multicast channel */
      if( mcast_join )
	{
	  /* We'll be listening on a multicast channel */
	  socket = create_mcast_client(pool, mcast_join, port, 0);
	  if(!socket)
	    {
	      fprintf(stderr,"Unable to join multicast channel %s:%d. Exiting\n",
		      mcast_join, port);
	      exit(1);
	    }
	}
      else
	{
          /* Create a UDP socket */
          socket = create_udp_client( pool, host, port );
          if(!socket)
            {
              fprintf(stderr,"Unable to create UDP client for %s:%d. Exiting.\n",
		      host? host: "NULL", port);
	      exit(1);
	    }
	}

      /* Add the socket to the array */
      *(apr_socket_t **)apr_array_push(send_channels) = socket;
    }

  return send_channels;
}


/* This function will send a datagram to every udp_send_channel specified */
int
Ganglia_udp_send_message(Ganglia_udp_send_channels channels, char *buf, int len )
{
  apr_status_t status;
  int i;
  int num_errors = 0;
  apr_size_t size;

  if(!buf || len<=0)
    return 1;

  for(i=0; i< channels->nelts; i++)
    {
      apr_socket_t *socket = ((apr_socket_t **)(channels->elts))[i];
      size   = len;
      status = apr_socket_send( socket, buf, &size );
      if(status != APR_SUCCESS)
	{
	  num_errors++;
	}
    }
  return num_errors;
}

Ganglia_gmetric
Ganglia_gmetric_create( Ganglia_pool parent_pool )
{
  Ganglia_pool pool = Ganglia_pool_create(parent_pool);
  Ganglia_gmetric gmetric;
  if(!pool)
    {
      return NULL;
    }
  gmetric = apr_pcalloc( pool, sizeof(struct Ganglia_gmetric));
  if(!gmetric)
    {
      Ganglia_pool_destroy(pool);
      return NULL;
    }

  gmetric->pool = pool;
  gmetric->msg  = apr_pcalloc( pool, sizeof(struct Ganglia_gmetric_message));
  if(!gmetric->msg)
    {
      Ganglia_pool_destroy(pool);
      return NULL;
    }

  return gmetric;
}

int
Ganglia_gmetric_send( Ganglia_gmetric gmetric, Ganglia_udp_send_channels send_channels )
{
  int len;
  XDR x;
  char gmetricmsg[1500];
  Ganglia_message msg;

  msg.id = 0;
  memcpy( &(msg.Ganglia_message_u.gmetric), gmetric->msg, sizeof(Ganglia_gmetric_message));

  /* Send the message */
  xdrmem_create(&x, gmetricmsg, 1500, XDR_ENCODE);
  xdr_Ganglia_message(&x, &msg);
  len = xdr_getpos(&x); 
  return Ganglia_udp_send_message( send_channels, gmetricmsg, len);
}

void
Ganglia_gmetric_destroy( Ganglia_gmetric gmetric )
{
  if(!gmetric)
    return;
  Ganglia_pool_destroy(gmetric->pool);
}


/*
 * struct Ganglia_gmetric_message {
 *   char *type;
 *   char *name;
 *   char *value;
 *   char *units;
 *   u_int slope;
 *   u_int tmax;
 *   u_int dmax;
 * };
 */
int
Ganglia_gmetric_set( Ganglia_gmetric gmetric, char *name, char *value, char *type, char *units, unsigned int slope, unsigned int tmax, unsigned int dmax)
{
  /* Make sure all the params look ok */
  if(!gmetric||!name||!value||!type||!units||slope<0||slope>4)
    return 1;

  /* Make sure none of the string params have a '"' in them (breaks the xml) */
  if(strchr(name, '"')||strchr(value,'"')||strchr(type,'"')||strchr(units,'"'))
    {
      return 2;
    }

  /* Make sure the type is one that is supported (string|int8|uint8|int16|uint16|int32|uint32|float|double)*/
  if(!(!strcmp(type,"string")||!strcmp(type,"int8")||!strcmp(type,"uint8")||
     !strcmp(type,"int16")||!strcmp(type,"uint16")||!strcmp(type,"int32")||
     !strcmp(type,"uint32")||!strcmp(type,"float")||!strcmp(type,"double")))
    {
      return 3;
    }

  /* All the data is there and validated... copy it into the structure */
  gmetric->msg->name = apr_pstrdup( gmetric->pool, name);
  gmetric->msg->value = apr_pstrdup( gmetric->pool, value);
  gmetric->msg->type  = apr_pstrdup( gmetric->pool, type);
  gmetric->msg->units = apr_pstrdup( gmetric->pool, units);
  gmetric->msg->slope = slope;
  gmetric->msg->tmax = tmax;
  gmetric->msg->dmax = dmax;

  return 0;
}
