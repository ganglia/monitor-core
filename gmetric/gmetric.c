#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <confuse.h>   /* header for libconfuse */

#include <apr.h>
#include <apr_strings.h>
#include <apr_pools.h>

#include "ganglia.h"
#include "cmdline.h"

Ganglia_pool global_context;
Ganglia_metric gmetric;
Ganglia_gmond_config gmond_config;
Ganglia_udp_send_channels send_channels;

/* The commandline options */
struct gengetopt_args_info args_info;

int
main( int argc, char *argv[] )
{
  int rval;

  /* process the commandline options */
  if (cmdline_parser (argc, argv, &args_info) != 0)
        exit(1);

  /* create the global context */
  global_context = Ganglia_pool_create(NULL);
  if(!global_context)
    {
      fprintf(stderr,"Unable to create global context. Exiting.\n");
      exit(1);
    }
  
  /* parse the configuration file */
  gmond_config = Ganglia_gmond_config_create( args_info.conf_arg, !args_info.conf_given);

  /* deal with spoof overrides */
  cfg_t *globals = (cfg_t*) cfg_getsec( (cfg_t *)gmond_config, "globals" );
  char *override_hostname = cfg_getstr( globals, "override_hostname" );
  char *override_ip = cfg_getstr( globals, "override_ip" );

  /* build the udp send channels */
  send_channels = Ganglia_udp_send_channels_create(global_context, gmond_config);
  if(!send_channels)
    {
      fprintf(stderr,"Unable to create ganglia send channels. Exiting.\n");
      exit(1);
    }

  /* create the message */
  gmetric = Ganglia_metric_create(global_context);
  if(!gmetric)
    {
      fprintf(stderr,"Unable to allocate gmetric structure. Exiting.\n");
      exit(1);
    }
  apr_pool_t *gm_pool = (apr_pool_t*)gmetric->pool;

  if(args_info.spoof_given && args_info.heartbeat_given){
    rval = Ganglia_metric_set(gmetric, "heartbeat", "0", "uint32", "", 0, 0, 0);
  }else{
    if( ! (args_info.name_given && args_info.value_given && args_info.type_given))
      {
        fprintf(stderr,"Incorrect options supplied, exiting.\n");
        exit(1);
      }
    rval = Ganglia_metric_set( gmetric, args_info.name_arg, args_info.value_arg,
             args_info.type_arg, args_info.units_arg, cstr_to_slope(args_info.slope_arg),
             args_info.tmax_arg, args_info.dmax_arg);
  }

  /* TODO: make this less ugly later */
  switch(rval)
    {
    case 1:
      fprintf(stderr,"gmetric parameters invalid. exiting.\n");
      exit(1); 
    case 2:
      fprintf(stderr,"one of your parameters has an invalid character '\"'. exiting.\n");
      exit(1);
    case 3:
      fprintf(stderr,"the type parameter \"%s\" is not a valid type. exiting.\n", args_info.type_arg);
      exit(1);
    case 4:
      fprintf(stderr,"the value parameter \"%s\" does not represent a number. exiting.\n", args_info.value_arg);
      exit(1);
    }

  /* TODO: Try to validate the spoof arg format.  A better validation could 
   *  be done here. This is just checking for a colon. */
  if(args_info.spoof_given && !strchr(args_info.spoof_arg,':'))
    {
      fprintf(stderr,"Incorrect format for spoof argument. exiting.\n");
      exit(1);
    }

  if(args_info.spoof_given)
      Ganglia_metadata_add(gmetric, SPOOF_HOST, args_info.spoof_arg);
  if(!args_info.spoof_given && override_hostname != NULL)
    {
      char *spoof_string = apr_pstrcat(gm_pool, override_ip != NULL ? override_ip : override_hostname, ":", override_hostname, NULL);
      Ganglia_metadata_add(gmetric, SPOOF_HOST, spoof_string);
    }
  if(args_info.heartbeat_given)
      Ganglia_metadata_add(gmetric, SPOOF_HEARTBEAT, "yes");
  if(args_info.cluster_given)
      Ganglia_metadata_add(gmetric, "CLUSTER", args_info.cluster_arg);
  if(args_info.group_given)
    {
      char *last;
      for (char *group = apr_strtok(args_info.group_arg, ", ", &last); group != NULL; group = apr_strtok(NULL, ", ", &last)) {
        Ganglia_metadata_add(gmetric, "GROUP", group);
      }
    }
  if(args_info.desc_given)
      Ganglia_metadata_add(gmetric, "DESC", args_info.desc_arg);
  if(args_info.title_given)
      Ganglia_metadata_add(gmetric, "TITLE", args_info.title_arg);

  /* send the message */
  rval = Ganglia_metric_send(gmetric, send_channels);
  if(rval)
    {
      fprintf(stderr,"There was an error sending to %d of the send channels.\n", rval);
    }

  /* cleanup */
  Ganglia_metric_destroy(gmetric); /* not really necessary but for symmetry */
  Ganglia_pool_destroy(global_context);

  return 0;
}
