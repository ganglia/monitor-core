#include <stdio.h>
#include <dotconf.h>
#include "ganglia_priv.h"
#include "g25_config.h"

gmond_config_t gmond_config;

static char *
conf_strdup (const char *s)
{
  char *result = (char*)malloc(strlen(s) + 1);
  if (result == (char*)0)
    return (char*)0;
  strcpy(result, s);
  return result;
}

static FUNC_ERRORHANDLER(errorhandler)
{
   printf("gmond config file error: %s\n", msg);
   exit(1);
}

static DOTCONF_CB(cb_name)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   free(c->name);
   c->name = conf_strdup(cmd->data.str);
   return NULL;
}

static DOTCONF_CB(cb_owner)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   free(c->owner);
   c->owner = conf_strdup(cmd->data.str);
   return NULL;
}

static DOTCONF_CB(cb_latlong)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   free(c->latlong);
   c->latlong = conf_strdup(cmd->data.str);
   return NULL;
}

static DOTCONF_CB(cb_location)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   free(c->location);
   c->location = conf_strdup(cmd->data.str);
   return NULL;
}

static DOTCONF_CB(cb_url)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   free(c->url);
   c->url = conf_strdup(cmd->data.str);
   return NULL;
}

static DOTCONF_CB(cb_mcast_channel)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   free(c->mcast_channel);
   c->mcast_channel = conf_strdup(cmd->data.str);
   return NULL;
}

static DOTCONF_CB(cb_mcast_port)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   c->mcast_port = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_mcast_if)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   /* no free.. set to NULL by default */
   c->mcast_if_given = 1;
   c->mcast_if = conf_strdup(cmd->data.str);
   return NULL;
}

static DOTCONF_CB(cb_mcast_ttl)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   c->mcast_ttl  = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_mcast_threads)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   c->mcast_threads = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_xml_port)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   c->xml_port = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_xml_threads)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   c->xml_threads = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_trusted_hosts)
{
   int i;
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;

   c->trusted_hosts = (char **)malloc(sizeof(char *) * cmd->arg_count + 1);
   if(!c->trusted_hosts)
     {
       fprintf(stderr,"Unable to create trusted_hosts array. Exiting.\n");
       exit(1);
     }
   
   for (i = 0; i < cmd->arg_count; i++)
      {
         c->trusted_hosts[i] = strdup(cmd->data.list[i]);
      }
   c->trusted_hosts[i] = NULL;
   return NULL;
}

static DOTCONF_CB(cb_num_nodes)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   c->num_nodes = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_num_custom_metrics)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   c->num_custom_metrics = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_mute)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   c->mute = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_deaf)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   c->deaf = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_allow_extra_data)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   c->allow_extra_data = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_debug_level)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   c->debug_level = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_no_setuid)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   c->no_setuid = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_setuid)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   c->setuid = conf_strdup( cmd->data.str );
   return NULL;
}

static DOTCONF_CB(cb_no_gexec)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   c->no_gexec = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_all_trusted)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   c->all_trusted = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_host_dmax)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   c->host_dmax = cmd->data.value;
   return NULL;
}


static int
set_defaults(gmond_config_t *config )
{
   /* Gmond defaults */
   config->name = conf_strdup("unspecified");
   config->owner = conf_strdup("unspecified");
   config->latlong = conf_strdup("unspecified");
   config->url = conf_strdup("unspecified");
   config->location = conf_strdup("unspecified");
   config->mcast_channel = conf_strdup("239.2.11.71");
   config->mcast_port = 8649;
   config->mcast_if_given = 0;
   config->mcast_ttl = 1;
   config->mcast_threads = 2;
   config->xml_port = 8649;
   config->xml_threads = 2;
   config->trusted_hosts = NULL;
   config->num_nodes = 1024;
   config->num_custom_metrics = 16;
   config->mute = 0;
   config->deaf = 0;
   config->allow_extra_data = 1;
   config->debug_level = 0;
   config->no_setuid = NO_SETUID;
   config->setuid = conf_strdup(SETUID_USER);
   config->no_gexec = 0;
   config->all_trusted = 0;
   config->host_dmax = 0;

   return 0;
}

static int
print_config(char *path, gmond_config_t *config)
{
  int i;
  char *p;

  fprintf(stdout,"/* global variables */\n");
  fprintf(stdout,"globals {\n  mute = \"%s\"\n  deaf = \"%s\"\n  allow_extra_data = \"%s\"\n  debug_level = \"%ld\"\n  setuid = \"%s\"\n  user=\"%s\"\n  gexec = \"%s\"\n  host_dmax = \"%ld\"\n}\n\n",
          config->mute? "yes":"no", 
          config->deaf? "yes":"no",
          config->allow_extra_data? "yes":"no", 
          config->debug_level, 
          config->no_setuid? "no":"yes",
          config->setuid,
          config->no_gexec? "no":"yes",
          config->host_dmax);

  fprintf(stdout,"/* info about cluster  */\n");
  fprintf(stdout,"cluster {\n  name = \"%s\"\n  owner = \"%s\"\n  latlong = \"%s\"\n  url=\"%s\"\n}\n\n",
          config->name, config->owner, config->latlong, config->url);

  fprintf(stdout,"/* info about host */\n");
  fprintf(stdout,"host {\n  location = \"%s\"\n}\n\n",
          config->location);

  fprintf(stdout,"/* channel to send multicast on mcast_channel:mcast_port */\n");
  fprintf(stdout,"udp_send_channel {\n  mcast_join = \"%s\"\n  port = \"%hd\"\n  ttl=\"%ld\"\n",
          config->mcast_channel, config->mcast_port, config->mcast_ttl);
  if(config->mcast_if_given)
    {
      fprintf(stdout,"  mcast_if = \"%s\"\n", config->mcast_if);
    }
  fprintf(stdout,"}\n\n");

  fprintf(stdout,"/* channel to receive multicast from mcast_channel:mcast_port */\n");
  fprintf(stdout,"udp_recv_channel {\n  mcast_join = \"%s\"\n  port = \"%hd\"\n  bind = \"%s\"\n",
          config->mcast_channel, config->mcast_port, config->mcast_channel);
  if(config->mcast_if_given)
    {
      fprintf(stdout,"  mcast_if = \"%s\"\n", config->mcast_if);
    }
  fprintf(stdout,"}\n\n");

  fprintf(stdout,"/* channel to export xml on xml_port */\n");
  fprintf(stdout,"tcp_accept_channel {\n  port = \"%hd\"\n", config->xml_port);
  if( config->trusted_hosts  && !config->all_trusted )
    {
      fprintf(stdout,"/* your trusted_hosts assuming ipv4 mask*/\n");
      fprintf(stdout,"  acl {\n    default=\"deny\"\n");
      for(i = 0, p = config->trusted_hosts[i]; p; i++, p = config->trusted_hosts[i])
        {
          fprintf(stdout, "    access {\n");
          fprintf(stdout, "      ip=\"%s\"\n      mask = 32\n      action = \"allow\"\n", p);
          fprintf(stdout, "    }\n");
        }
      fprintf(stdout, "    access {\n");
      fprintf(stdout, "      ip=\"127.0.0.1\"\n      mask = 32\n      action = \"allow\"\n");
      fprintf(stdout, "    }\n");
      fprintf(stdout,"  }\n");/* close acl */
    }
  fprintf(stdout,"}\n\n"); /* close tcp_accept_channel */
  fprintf(stdout,"%s\n", Ganglia_default_collection_groups());
  return 0;
}


int 
print_ganglia_25_config( char *path )
{
   configfile_t *configfile;
   FILE *fp;
   static configoption_t gmond_options[] =
      {
         {"name", ARG_STR, cb_name, &gmond_config, 0},
         {"owner", ARG_STR, cb_owner, &gmond_config, 0},
         {"latlong", ARG_STR, cb_latlong, &gmond_config, 0},
         {"url", ARG_STR, cb_url, &gmond_config, 0},
         {"location", ARG_STR, cb_location, &gmond_config, 0},
         {"mcast_channel", ARG_STR, cb_mcast_channel, &gmond_config, 0},
         {"mcast_port", ARG_INT, cb_mcast_port, &gmond_config, 0},
         {"mcast_if", ARG_STR, cb_mcast_if, &gmond_config, 0},
         {"mcast_ttl", ARG_INT, cb_mcast_ttl, &gmond_config, 0},
         {"mcast_threads", ARG_INT, cb_mcast_threads, &gmond_config, 0},
         {"xml_port", ARG_INT, cb_xml_port, &gmond_config, 0},
         {"xml_threads", ARG_INT, cb_xml_threads, &gmond_config, 0},
         {"trusted_hosts", ARG_LIST, cb_trusted_hosts, &gmond_config, 0},
         {"num_nodes", ARG_INT, cb_num_nodes, &gmond_config, 0},
         {"num_custom_metrics", ARG_INT, cb_num_custom_metrics, &gmond_config, 0},
         {"mute", ARG_TOGGLE, cb_mute, &gmond_config, 0},
         {"deaf", ARG_TOGGLE, cb_deaf, &gmond_config, 0},
         {"allow_extra_data", ARG_TOGGLE, cb_allow_extra_data, &gmond_config, 0},
         {"debug_level", ARG_INT, cb_debug_level, &gmond_config, 0},
         {"no_setuid", ARG_TOGGLE, cb_no_setuid, &gmond_config, 0},
         {"setuid", ARG_STR, cb_setuid, &gmond_config, 0},
         {"no_gexec", ARG_TOGGLE, cb_no_gexec, &gmond_config, 0},
         {"all_trusted", ARG_TOGGLE, cb_all_trusted, &gmond_config, 0},
         {"host_dmax", ARG_INT, cb_host_dmax, &gmond_config, 0},
         LAST_OPTION
      };

   if(!path)
     {
       return 1;
     }
   fp = fopen( path, "r");
   if(!fp)
     {
       fprintf(stderr,"Unable to open ganglia 2.5 configuration '%s'. Exiting.\n", path);
       return 1; 
     }

   set_defaults(&gmond_config);

   configfile = dotconf_create(path, gmond_options, 0, CASE_INSENSITIVE);
   if(!configfile)
     {
       fprintf(stderr,"dotconf_create() error for configuration '%s'. Exiting.\n", path);
       return 1;
     }

   configfile->errorhandler = (dotconf_errorhandler_t) errorhandler;

   if (dotconf_command_loop(configfile) == 0)
      {
         dotconf_cleanup(configfile);
         return 1;
      }
   dotconf_cleanup(configfile);

   print_config(path, &gmond_config);
   return 0;
}
