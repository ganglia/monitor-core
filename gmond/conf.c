/* $Id$ */
#include <stdio.h>
#include <string.h>
#include <netdb.h>

#include "conf.h"

#include "lib/ganglia.h"
#include "libunp/unp.h"
#include "lib/dotconf.h"

gmond_config_t gmond_config;

struct ifi_info *all_interfaces;

struct mycontext
{
  int permissions;
  const char *current_end_token;
};

enum sections
{
  ROOT_SECTION = 1,
  CHANNEL_SECTION = 2
};

static const char end_ChannelSection[] = "</Channel>";

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

  /* We are going to overwrite the first channel entry */
  c->channels[0]->address = conf_strdup(cmd->data.str);
  return NULL;
}

static void
set_channel_defaults( channel_t *channel )
{
  channel->address = NULL;
  channel->port = NULL;
  channel->action = NULL;
  channel->ttl  = 1;
  channel->num_interfaces = 0;
  channel->interfaces = NULL;
}

static DOTCONF_CB(cb_open_channel)
{
  struct mycontext *context = (struct mycontext *)ctx;
  const char *old_end_token = context->current_end_token;
  int old_override          = context->permissions;
  const char *err = 0;
  gmond_config_t *c = (gmond_config_t *)cmd->option->info;

  if(context->permissions & CHANNEL_SECTION)
    return "<Channel> cannot be nested";

  if(!c->channel_given)
    {
      /* User has specified a channel but we're just going to overwrite
         the first channel (which is set to the defaults) */
      c->channel_given = 1;
      c->num_send_channels = 0; /* reset */
      c->num_receive_channels = 0; /* reset */
    }
  else
    {
      /* Make room for the next channel */
      c->current_channel++;
      c->channels = (channel_t **)realloc( c->channels, (c->current_channel +1) * sizeof( channel_t *));
      c->channels[c->current_channel] = (channel_t *)malloc(sizeof(channel_t));
      if(!c->channels[c->current_channel])
	{
	  fprintf(stderr,"Unable to malloc space for new channel\n");
	  exit(1);
	}
    }

  set_channel_defaults( c->channels[c->current_channel] );

  context->permissions |= CHANNEL_SECTION;
  context->current_end_token = end_ChannelSection;

  while (!cmd->configfile->eof)
    {
      err = dotconf_command_loop_until_error(cmd->configfile);
      if (!err)
        {
          err = "</Channel> is missing";
          break;
        }

      if (err == context->current_end_token)
        break;

      dotconf_warning(cmd->configfile, DCLOG_ERR, 0, err);
    }

  context->current_end_token = old_end_token;
  context->permissions       = old_override;

  if (err != end_ChannelSection)
    return err;

  return NULL;
}

/* This callback is used to close all sections */
static DOTCONF_CB(cb_close_channel)
{
  struct mycontext *context = (struct mycontext *)ctx;
  gmond_config_t *c = (gmond_config_t *)cmd->option->info;
  static char buf[1024];
  channel_t *channel;
  
  if (!context->current_end_token)
    {
      snprintf(buf, 1024, "%s without matching <%s", cmd->name, cmd->name+2);
      return buf;
    }

  if (context->current_end_token != cmd->name)
    {
      snprintf(buf, 1024, "Expected '%s' but saw %s", context->current_end_token, cmd->name);
      return buf;
    }

  /* Count the number of actions required for the thread pools later */
  channel = c->channels[c->current_channel];
  if(strstr(channel->action, "send"))
     {
       c->num_send_channels++;
       if(channel->num_interfaces)
	 {
           c->num_send_channels += channel->num_interfaces;
	 }
     }
  if(strstr(channel->action, "receive"))
    {
      c->num_receive_channels++;
      if(channel->num_interfaces)
	{
	  c->num_receive_channels += channel->num_interfaces;
	}
    }

  return context->current_end_token;
}

static DOTCONF_CB(cb_address)
{
  gmond_config_t *c = (gmond_config_t *)cmd->option->info;

  c->channels[c->current_channel]->address = conf_strdup( cmd->data.str );
  return NULL;
}

static DOTCONF_CB(cb_port)
{
  gmond_config_t *c = (gmond_config_t *)cmd->option->info;

  c->channels[c->current_channel]->port = conf_strdup( cmd->data.str );
  return NULL;
}

static DOTCONF_CB(cb_interfaces)
{
  int i, found, want_all = 0;
  gmond_config_t *c = (gmond_config_t *)cmd->option->info;
  channel_t *channel;
  struct ifi_info *n;
  int num_interfaces;

  /* Check that the interfaces are valid */
  for(i=0; i< cmd->arg_count; i++)
    {
      if(!strcmp(cmd->data.list[i], "all"))
	{
	  want_all = 1;
	  break;
	}

      found = 0;
      for(n = all_interfaces; n; n = n->ifi_next)
        {
          /* Check if the interface exists on the machine */
          if(!strcmp( n->ifi_name, cmd->data.list[i]))
            {
              found =1;
              break;
            }
        }
      if(!found)
        {
          fprintf(stderr,"The interface '%s' was not found. Check your config file.\n", cmd->data.list[i]);
          exit(1);
        }
    }

  channel = c->channels[c->current_channel];

  if(want_all)
    {
      /* Keyword "all" detected, therefore put all interfaces in the list */
      num_interfaces = 0;
      for( n = all_interfaces; n; n = n->ifi_next )
	{
	  /* We need to count the number of interfaces in the linked list */
	  num_interfaces++;
	}

      channel->interfaces = malloc( num_interfaces * sizeof(char *));
      if(!channel->interfaces)
        {
	  fprintf(stderr,"Unable to malloc space for interfaces\n");
	  exit(1);
	}

      channel->num_interfaces = num_interfaces;

      for( i = 0, n = all_interfaces; n; n = n->ifi_next, i++ )
	{
	  channel->interfaces[i] = conf_strdup( n->ifi_name );
	}

    }
  else
    {
      /* Person doesn't want interfaces, so we'll just add the interfaces they specified */
      channel->interfaces = malloc( cmd->arg_count * sizeof(char *));
      if(!channel->interfaces)
        {
          fprintf(stderr,"Unable to malloc space for interfaces\n");
          exit(1);
        }

      channel->num_interfaces = cmd->arg_count;

      for (i = 0; i < cmd->arg_count; i++)
        {
          channel->interfaces[i] = conf_strdup( cmd->data.list[i] );
        }
    }

  return NULL;
}

static DOTCONF_CB(cb_action)
{
  gmond_config_t *c = (gmond_config_t *)cmd->option->info;

  c->channels[c->current_channel]->action = conf_strdup( cmd->data.str);
  return NULL;
}

static DOTCONF_CB(cb_ttl)
{
  return NULL;
}

static DOTCONF_CB(cb_mcast_port)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   /* We are going to overwrite the first channel with mcast_port info */
   c->channels[0]->port = conf_strdup(cmd->data.str);
   return NULL;
}

static DOTCONF_CB(cb_mcast_if)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   
   /* We overwrite the first channels interfaces */
   c->channels[0]->interfaces[0] = conf_strdup(cmd->data.str);
   c->channels[0]->num_interfaces = 1;
   return NULL;
}

static DOTCONF_CB(cb_mcast_ttl)
{
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   c->channels[0]->ttl  = cmd->data.value;
   return NULL;
}

/* this is not used any more */
static DOTCONF_CB(cb_mcast_threads)
{
  /*
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   c->mcast_threads = cmd->data.value;
   */
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


static DOTCONF_CB(cb_trusted_hosts)
{
   int i;
   llist_entry *le;
   gmond_config_t *c = (gmond_config_t *)cmd->option->info;
   struct addrinfo *info;
   void *addr;

   for (i = 0; i < cmd->arg_count; i++)
      {
         le = (llist_entry *)malloc(sizeof(llist_entry));
         info = host_serv( cmd->data.list[i], NULL, AF_UNSPEC, SOCK_STREAM);
         if(!info || (info->ai_family != AF_INET && info->ai_family != AF_INET6))
	   {
	     err_msg("Warning: %s is not being added as a trusted host", 
		     cmd->data.list[i]);
	     continue;
	   }

         le->val = (char*) malloc(INET6_ADDRSTRLEN + 1);

	 if( info->ai_family == AF_INET )
	   {
	     addr = &(((struct sockaddr_in *) info->ai_addr )->sin_addr);
	   }
	 else
	   {
	     addr = &(((struct sockaddr_in6 *)info->ai_addr )->sin6_addr);
	   }

         inet_ntop(info->ai_family, addr, le->val, INET6_ADDRSTRLEN);

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

static DOTCONF_CB(cb_cluster_tag)
{
  gmond_config_t *c = (gmond_config_t *)cmd->option->info;
  c->cluster_tag = cmd->data.value;
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
   config->channel_given = 0;

   config->name = conf_strdup("unspecified");
   config->owner = conf_strdup("unspecified");
   config->latlong = conf_strdup("unspecified");
   config->url = conf_strdup("unspecified");
   config->location = conf_strdup("unspecified");

   config->current_channel = 0; 
   config->num_send_channels = 1; /* The default */
   config->num_receive_channels = 1; /* channel */

   /* Make room for the first channel */
   config->channels = (channel_t **) malloc ( sizeof(channel_t *));
   if(!config->channels)
     {
       fprintf(stderr,"Unable to malloc memory array for first channel\n");
       exit(1);
     } 
   config->channels[0] = (channel_t *) malloc (sizeof(channel_t));
   if(!config->channels[0])
     {
       fprintf(stderr,"Unable to malloc memory for first channel\n");
       exit(1);
     }

   /* We want to send/receive on the default multicast channel 
    * for backward compatibility */
   config->channels[0]->address = conf_strdup("239.2.11.71");
   config->channels[0]->port = conf_strdup("8649");
   config->channels[0]->ttl  = 1;
   config->channels[0]->num_interfaces = 0;
   config->channels[0]->interfaces = NULL;
   config->channels[0]->action = conf_strdup("send_receive");

   
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


   /* Intialize information about the interfaces on the machine */
   all_interfaces = get_ifi_info( AF_INET, 0);
    
   config->cluster_tag = 1;
   return 0;
}

void
gmond_print_conf( gmond_config_t *config )
{
   int i,j;
   channel_t *c;

   printf("name is %s\n", config->name == NULL? "NULL": config->name);
   printf("owner is %s\n", config->owner == NULL? "NULL" : config->owner);
   printf("latlong is %s\n", config->latlong == NULL? "NULL" : config->latlong);
   printf("cluster URL is %s\n", config->url == NULL? "NULL" : config->url);
   printf("host location is (x,y,z): %s\n", config->location == NULL? "NULL": config->location);

   printf("There are %d channels (%d send %d receive)\n", 
	  config->current_channel+1, config->num_send_channels, config->num_receive_channels);

   for(i=0; i<= config->current_channel; i++)
     {
       c = config->channels[i];

       printf("Info for channel #%d\n", i+1);
       printf("\tAddress = %s\n", c->address == NULL? "NULL" : c->address);
       printf("\t   Port = %s\n", c->port == NULL? "NULL" : c->port);
       printf("\tThere are %d interfaces\n", c->num_interfaces);
       for(j=0; j< c->num_interfaces; j++)
         {
           printf("\t\t%s\n", c->interfaces[j]);
         }
       printf("\tThe TTL is set to %d\n", c->ttl);
       printf("\tAction is set to %s\n", c->action == NULL? "NULL" : c->action );
     }
   /*
   printf("mcast_threads is %ld\n", config->mcast_threads);
   */
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

const char *context_checker(command_t *cmd, unsigned long mask)
{
        struct mycontext *context = cmd->context;
        static char buf[1024];

        /*
     * this test is quite simple: if the permissions needed for the
         * to-be-called command are not granted, we'll deny service
     */

        /* Root Context granted and Root Context given? */
        if (!mask && !context->permissions)
                return NULL;

        if ( !(context->permissions & mask) )
        {
          snprintf(buf, 1024, "Option %s not allowed in <%s\n", cmd->name, 
                   context->current_end_token? context->current_end_token + 2 : "global>");
          return buf;
        }

        return NULL;
}


int 
get_gmond_config( char *conffile )
{
   int rval;
   configfile_t *configfile;
   char default_conffile[]=DEFAULT_GMOND_CONFIG_FILE;
   FILE *fp;
   struct mycontext context;
   static configoption_t gmond_options[] =
      {
         {"<Channel>", ARG_NONE, cb_open_channel, &gmond_config, CTX_ALL},
         {end_ChannelSection, ARG_NONE, cb_close_channel, &gmond_config, CHANNEL_SECTION},
         {"address", ARG_STR, cb_address, &gmond_config, CHANNEL_SECTION},
         {"port", ARG_STR, cb_port, &gmond_config, CHANNEL_SECTION},
         {"interfaces", ARG_LIST, cb_interfaces, &gmond_config, CHANNEL_SECTION},
         {"action", ARG_STR, cb_action, &gmond_config, CHANNEL_SECTION},
         {"ttl", ARG_INT, cb_ttl, &gmond_config, CHANNEL_SECTION},

         {"cluster_tag", ARG_TOGGLE, cb_cluster_tag, &gmond_config, CTX_ALL},

         {"name", ARG_STR, cb_name, &gmond_config, CTX_ALL},
         {"owner", ARG_STR, cb_owner, &gmond_config, CTX_ALL},
         {"latlong", ARG_STR, cb_latlong, &gmond_config, CTX_ALL},
         {"url", ARG_STR, cb_url, &gmond_config, CTX_ALL},
         {"location", ARG_STR, cb_location, &gmond_config, CTX_ALL},
         {"mcast_channel", ARG_STR, cb_mcast_channel, &gmond_config, CTX_ALL},
         {"mcast_port", ARG_STR, cb_mcast_port, &gmond_config, CTX_ALL},
         {"mcast_if", ARG_STR, cb_mcast_if, &gmond_config, CTX_ALL},
         {"mcast_ttl", ARG_INT, cb_mcast_ttl, &gmond_config, CTX_ALL},
         {"mcast_threads", ARG_INT, cb_mcast_threads, &gmond_config, CTX_ALL},
         {"xml_port", ARG_STR, cb_xml_port, &gmond_config, CTX_ALL},
         {"compressed_xml_port", ARG_STR, cb_compressed_xml_port, &gmond_config, CTX_ALL},
         {"xml_threads", ARG_INT, cb_xml_threads, &gmond_config, CTX_ALL},
         {"compressed_xml_threads", ARG_INT, cb_compressed_xml_threads, &gmond_config, CTX_ALL},
         {"trusted_hosts", ARG_LIST, cb_trusted_hosts, &gmond_config, CTX_ALL},
         {"num_nodes", ARG_INT, cb_num_nodes, &gmond_config, CTX_ALL},
         {"num_custom_metrics", ARG_INT, cb_num_custom_metrics, &gmond_config, CTX_ALL},
         {"mute", ARG_TOGGLE, cb_mute, &gmond_config, CTX_ALL},
         {"deaf", ARG_TOGGLE, cb_deaf, &gmond_config, CTX_ALL},
         {"debug_level", ARG_INT, cb_debug_level, &gmond_config, CTX_ALL},
         {"no_setuid", ARG_TOGGLE, cb_no_setuid, &gmond_config, CTX_ALL},
         {"setuid", ARG_STR, cb_setuid, &gmond_config, CTX_ALL},
         {"no_gexec", ARG_TOGGLE, cb_no_gexec, &gmond_config, CTX_ALL},
         {"all_trusted", ARG_TOGGLE, cb_all_trusted, &gmond_config, CTX_ALL},
         {"host_dmax", ARG_INT, cb_host_dmax, &gmond_config, CTX_ALL},
         {"xml_compression_level", ARG_INT, cb_xml_compression_level, &gmond_config, CTX_ALL},
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
      configfile = dotconf_create(conffile, gmond_options, (void *)&context, CASE_INSENSITIVE);
   else
      configfile = dotconf_create(default_conffile, gmond_options, (void *)&context, CASE_INSENSITIVE);

   if (!configfile)
      return -1;

   configfile->errorhandler   = (dotconf_errorhandler_t) errorhandler;
   configfile->contextchecker = (dotconf_contextchecker_t) context_checker;

   /* initialize my context structure */
   context.current_end_token = 0;
   context.permissions = 0;

   if (dotconf_command_loop(configfile) == 0)
      {
         dotconf_cleanup(configfile);
         return -1;
      }

   dotconf_cleanup(configfile);
   return 1;
}
