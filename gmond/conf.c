/* $Id$ */
#include <stdio.h>
#include <netdb.h>

#include "conf.h"

#include "lib/ganglia.h"
#include "libunp/unp.h"
#include "lib/dotconf.h"

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

static DOTCONF_CB(cb_receive_channels)
{
  int i;
  gmond_config_t *c = (gmond_config_t *)cmd->option->info;
  char *p = NULL;

  if(!c->receive_channels_given)
    {
      /* Reset the defaults */
      c->receive_channels_given = 1;
      c->num_receive_channels = 0;
      free(c->receive_channels[0]);
      free(c->receive_ports[0]);
      c->receive_channels[0] = NULL;
      c->receive_ports[0] = NULL;
    }

  for(i=0; i< cmd->arg_count; i++)
    {
      if(c->num_receive_channels < MAX_NUM_CHANNELS-1)
        {
          p = strchr( cmd->data.list[i], ':');
          if(p)
            {
              /* they specified a host */
              *p = '\0';
              c->receive_channels[c->num_receive_channels] = conf_strdup( cmd->data.list[i] );
              c->receive_ports[c->num_receive_channels]    = conf_strdup( p+1 );
            } 
          else
            {
              c->receive_channels[c->num_receive_channels] = NULL;  /* Any address */
              c->receive_ports[c->num_receive_channels]    = conf_strdup( cmd->data.list[i] );
            }
          c->num_receive_channels++;
        }
    }

  return NULL;
}

static DOTCONF_CB(cb_send_channels)
{
  int i;
  gmond_config_t *c = (gmond_config_t *)cmd->option->info;
  char *p = NULL;

  if(!c->send_channels_given)
    {
      /* Reset the defaults */
      c->send_channels_given = 1;
      c->num_send_channels = 0;
      free(c->send_channels[0]);
      free(c->send_ports[0]);
      c->send_channels[0] = NULL;
      c->send_ports[0] = NULL;
    }

  for(i=0; i< cmd->arg_count; i++)
    {
      if(c->num_send_channels < MAX_NUM_CHANNELS-1)
        {
          p = strchr( cmd->data.list[i], ':');
          if(p)
            {
              /* they specified a port */
              *p = '\0';
              c->send_channels[c->num_send_channels] = conf_strdup( cmd->data.list[i] );
              c->send_ports   [c->num_send_channels] = conf_strdup( p+1 );
            }
          else 
            {
              c->send_channels[c->num_send_channels] = conf_strdup( cmd->data.list[i] );
              c->send_ports   [c->num_send_channels] = conf_strdup( "8649" );
            }
          c->num_send_channels++;
        }
    }

  return NULL;
}

static DOTCONF_CB(cb_mcast_channel)
{
  gmond_config_t *c = (gmond_config_t *)cmd->option->info;
  free(c->send_channels[0]);
  c->send_channels[0] = conf_strdup(cmd->data.str);
  free(c->receive_channels[0]);
  c->receive_channels[0] = conf_strdup(cmd->data.str);
  return NULL;
}

static DOTCONF_CB(cb_mcast_port)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   free(c->send_ports[0]);
   c->send_ports[0] = conf_strdup(cmd->data.str);
   free(c->receive_ports[0]);
   c->receive_ports[0] = conf_strdup(cmd->data.str);
   return NULL;
}

static DOTCONF_CB(cb_mcast_if)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   /* no free.. set to NULL by default */
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
   free(c->xml_port);
   c->xml_port = conf_strdup(cmd->data.str);
   return NULL;
}

static DOTCONF_CB(cb_compressed_xml_port)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   free(c->compressed_xml_port);
   c->compressed_xml_port = conf_strdup(cmd->data.str);
   return NULL;
}

static DOTCONF_CB(cb_xml_threads)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   c->xml_threads = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_compressed_xml_threads)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   c->compressed_xml_threads = cmd->data.value;
   return NULL;
}


/* This function will disappear soon */
static int
g_gethostbyname(const char* hostname, struct sockaddr_in* sa, char** nicename)
{
  int rv = 0;
  struct in_addr inaddr;
  struct hostent* he;  

  if (inet_aton(hostname, &inaddr) != 0)
    {
      sa->sin_family = AF_INET;
      memcpy(&sa->sin_addr, (char*) &inaddr, sizeof(struct in_addr));
      if (nicename)
         *nicename = (char *)strdup (hostname);
      return 1;
    }

  he = (struct hostent*)gethostbyname(hostname);
  if (he && he->h_addrtype==AF_INET && he->h_addr_list[0])
    {
     if (sa)
        {
           sa->sin_family = he->h_addrtype;
           memcpy(&sa->sin_addr, he->h_addr_list[0], he->h_length);
        }

      if (nicename && he->h_name)
         *nicename = (char *)strdup(he->h_name);

      rv = 1;
    }

  return rv;
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
         rv = g_gethostbyname(cmd->data.list[i], &sa, NULL);
         if (!rv) {
            err_msg("Warning: we failed to resolve trusted host name %s", cmd->data.list[i]);
            continue;
         }
         le->val = (char*) malloc(MAXHOSTNAME);
         inet_ntop(AF_INET, &sa.sin_addr, le->val, MAXHOSTNAME);
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

static DOTCONF_CB(cb_xml_compression_level)
{
  gmond_config_t *c = (gmond_config_t *)cmd->option->info;
  c->xml_compression_level = cmd->data.value;
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

   config->num_send_channels = 1;
   config->send_channels_given = 0;
   config->send_channels[0] = conf_strdup("239.2.11.71");
   config->send_ports[0]    = conf_strdup("8649");

   config->num_receive_channels = 1;
   config->receive_channels_given = 0;
   config->receive_channels[0] = conf_strdup("239.2.11.71");
   config->receive_ports[0]    = conf_strdup("8649");
#if 0
   config->msg_channel = conf_strdup("239.2.11.71");
   config->msg_port = conf_strdup("8649");
   config->msg_if_given = 0;
   config->msg_port_given = 0;
   config->msg_ttl = 1;
   config->msg_threads = 2;
#endif
   config->xml_port = conf_strdup("8649");
   config->compressed_xml_port = conf_strdup("8650");
   config->xml_threads = 2;
   config->compressed_xml_threads = 2;
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
   config->xml_compression_level = 6; /* Z_DEFAULT_COMPRESSION */
   return 0;
}

void
print_conf( gmond_config_t *config )
{
   int i;
   printf("name is %s\n", config->name);
   printf("owner is %s\n", config->owner);
   printf("latlong is %s\n", config->latlong);
   printf("cluster URL is %s\n", config->url);
   printf("host location is (x,y,z): %s\n", config->location);
   printf("There are %d send channels\n", config->num_send_channels);
   for(i=0; i< config->num_send_channels; i++)
     {
       printf("\t%s : %s\n", config->send_channels[i], config->send_ports[i]);
     }
   printf("There are %d receive channels\n", config->num_receive_channels);
   for(i=0; i< config->num_receive_channels; i++)
     {
       printf("\t%s : %s\n", config->receive_channels[i], config->receive_ports[i]);
     }
   if(config->mcast_if_given)
      printf("mcast_if is %s\n", config->mcast_if);
   else
      printf("mcast_if is chosen by the kernel\n");
   printf("mcast_ttl is %ld\n", config->mcast_ttl);
   printf("mcast_threads is %ld\n", config->mcast_threads);
   printf("xml_port is %s\n", config->xml_port);
   printf("compressed_xml_port is %s\n", config->compressed_xml_port);
   printf("xml_threads is %ld\n", config->xml_threads);
   printf("compressed_xml_threads is %ld\n", config->compressed_xml_threads);
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
         {"mcast_channel", ARG_STR, cb_mcast_channel, &gmond_config, 0},
         {"mcast_port", ARG_STR, cb_mcast_port, &gmond_config, 0},
         {"mcast_if", ARG_STR, cb_mcast_if, &gmond_config, 0},
         {"mcast_ttl", ARG_INT, cb_mcast_ttl, &gmond_config, 0},
         {"mcast_threads", ARG_INT, cb_mcast_threads, &gmond_config, 0},
         {"send_channels", ARG_LIST, cb_send_channels, &gmond_config, 0},
         {"receive_channels", ARG_LIST, cb_receive_channels, &gmond_config, 0},
         {"xml_port", ARG_STR, cb_xml_port, &gmond_config, 0},
         {"compressed_xml_port", ARG_STR, cb_compressed_xml_port, &gmond_config, 0},
         {"xml_threads", ARG_INT, cb_xml_threads, &gmond_config, 0},
         {"compressed_xml_threads", ARG_INT, cb_compressed_xml_threads, &gmond_config, 0},
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
         {"xml_compression_level", ARG_INT, cb_xml_compression_level, &gmond_config, 0},
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
