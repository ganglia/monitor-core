/* $Id$ */
#include <stdio.h>
#include <dotconf.h>
#include "conf.h"
#include "net.h"
#include "error.h"
#include "hash.h"
#include "llist.h"
#include "net.h"
#include "expat.h"
#include "ganglia.h"

gmond_config_t gmond_config;
const char * my_inet_ntop( int af, void *src, char *dst, size_t cnt );

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

static DOTCONF_CB(cb_send_bind)
{
  gmond_config_t *c = (gmond_config_t *)cmd->option->info;
  free(c->send_bind);
  c->send_bind_given = 1;
  c->send_bind = conf_strdup(cmd->data.str);
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
   int i, rv;
   llist_entry *le;
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   struct sockaddr_in sa;

   for (i = 0; i < cmd->arg_count; i++)
      {
         le = (llist_entry *)malloc(sizeof(llist_entry));
         rv = g_gethostbyname( cmd->data.list[i], &sa, NULL);
         if (!rv) {
            err_msg("Warning: we failed to resolve trusted host name %s", cmd->data.list[i]);
            continue;
         }
         le->val = (char*) malloc(MAXHOSTNAMELEN);
         my_inet_ntop(AF_INET, &sa.sin_addr, le->val, MAXHOSTNAMELEN);
         /* printf("Adding trusted host %s (IP %s)\n", cmd->data.list[i], le->val); */
         llist_add(&(c->trusted_hosts), le);
      }
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
   config->send_bind = conf_strdup("unspecified");
   config->send_bind_given = 0;
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
   config->debug_level = 0;
   config->no_setuid = 0;
   config->setuid = conf_strdup("nobody");
   config->no_gexec = 0;
   config->all_trusted = 0;
   config->host_dmax = 0;

   return 0;
}

static void
print_conf( gmond_config_t *config )
{
   printf("name is %s\n", config->name);
   printf("owner is %s\n", config->owner);
   printf("latlong is %s\n", config->latlong);
   printf("Cluster URL is %s\n", config->url);
   printf("Host location is (x,y,z): %s\n", config->location);
   printf("mcast_channel is %s\n", config->mcast_channel);
   printf("mcast_port is %d\n", config->mcast_port);
   if(config->mcast_if_given)
      printf("mcast_if is %s\n", config->mcast_if);
   else
      printf("mcast_if is chosen by the kernel\n");
   printf("mcast_ttl is %ld\n", config->mcast_ttl);
   printf("mcast_threads is %ld\n", config->mcast_threads);
   printf("xml_port is %d\n", config->xml_port);
   printf("xml_threads is %ld\n", config->xml_threads);
   printf("trusted hosts are: ");
   llist_print(&(config->trusted_hosts));
   printf("\n");
   printf("num_nodes is %ld\n", config->num_nodes);
   printf("num_custom_metrics is %ld\n", config->num_custom_metrics);
   printf("mute is %ld\n", config->mute);
   printf("deaf is %ld\n", config->deaf);
   printf("debug_level is %ld\n", config->debug_level);
   printf("no_setuid is %ld\n", config->no_setuid);
   printf("setuid is %s\n", config->setuid);
   printf("no_gexec is %ld\n", config->no_gexec);
   printf("all_trusted is %ld\n", config->all_trusted);
   printf("host_dmax is %ld\n", config->host_dmax);
}

int 
get_gmond_config( char *conffile )
{
   int rval;
   configfile_t *configfile;
   char default_conffile[]=DEFAULT_GMOND_CONFIG_FILE;
   FILE *fp;
   static configoption_t gmond_options[] =
      {
         {"name", ARG_STR, cb_name, &gmond_config, 0},
         {"owner", ARG_STR, cb_owner, &gmond_config, 0},
         {"latlong", ARG_STR, cb_latlong, &gmond_config, 0},
         {"url", ARG_STR, cb_url, &gmond_config, 0},
         {"location", ARG_STR, cb_location, &gmond_config, 0},
	 {"send_channel",  ARG_STR, cb_mcast_channel, &gmond_config, 0},
         {"mcast_channel", ARG_STR, cb_mcast_channel, &gmond_config, 0},
	 {"send_port",  ARG_INT, cb_mcast_port, &gmond_config, 0},
         {"mcast_port", ARG_INT, cb_mcast_port, &gmond_config, 0},
	 {"send_bind", ARG_STR, cb_send_bind, &gmond_config, 0},
         {"mcast_if",  ARG_STR, cb_mcast_if, &gmond_config, 0},
         {"mcast_ttl", ARG_INT, cb_mcast_ttl, &gmond_config, 0},
         {"mcast_threads", ARG_INT, cb_mcast_threads, &gmond_config, 0},
         {"xml_port", ARG_INT, cb_xml_port, &gmond_config, 0},
         {"xml_threads", ARG_INT, cb_xml_threads, &gmond_config, 0},
         {"trusted_hosts", ARG_LIST, cb_trusted_hosts, &gmond_config, 0},
         {"num_nodes", ARG_INT, cb_num_nodes, &gmond_config, 0},
         {"num_custom_metrics", ARG_INT, cb_num_custom_metrics, &gmond_config, 0},
         {"mute", ARG_TOGGLE, cb_mute, &gmond_config, 0},
         {"deaf", ARG_TOGGLE, cb_deaf, &gmond_config, 0},
         {"debug_level", ARG_INT, cb_debug_level, &gmond_config, 0},
         {"no_setuid", ARG_TOGGLE, cb_no_setuid, &gmond_config, 0},
         {"setuid", ARG_STR, cb_setuid, &gmond_config, 0},
         {"no_gexec", ARG_TOGGLE, cb_no_gexec, &gmond_config, 0},
         {"all_trusted", ARG_TOGGLE, cb_all_trusted, &gmond_config, 0},
         {"host_dmax", ARG_INT, cb_host_dmax, &gmond_config, 0},
         LAST_OPTION
      };

   rval = set_defaults(&gmond_config );
   if( rval<0)
      return -1;

   /* Check if the conffile exists, it's not an error if it doesn't */ 
   if(conffile)
      fp = fopen( conffile, "r");
   else
      fp = fopen( default_conffile, "r");

   if(!fp)
      return 0; 

   if(conffile)
      configfile = dotconf_create(conffile, gmond_options, 0, CASE_INSENSITIVE);
   else
      configfile = dotconf_create(default_conffile, gmond_options, 0, CASE_INSENSITIVE);

   if (!configfile)
      return -1;

   configfile->errorhandler = (dotconf_errorhandler_t) errorhandler;

   if (dotconf_command_loop(configfile) == 0)
      {
         dotconf_cleanup(configfile);
         return -1;
      }

   if(gmond_config.debug_level)
      {
         fprintf(stderr,"%s configuration\n", conffile);
         print_conf( &gmond_config );
      }

   dotconf_cleanup(configfile);
   return 1;
}
