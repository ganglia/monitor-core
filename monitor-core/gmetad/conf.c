#include <string.h>

#include "lib/dotconf.h"
#include "lib/ganglia.h"
#include "lib/hash.h"
#include "lib/llist.h"
#include "libunp/unp.h"

#include "gmetad.h"
#include "conf.h"

/* Variables that get filled in by configuration file */
extern Source_t root;

extern hash_t *sources;

gmetad_config_t gmetad_config;

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


static DOTCONF_CB(cb_gridname)
{
    gmetad_config_t *c = (gmetad_config_t*) cmd->option->info;
    debug_msg("Grid name %s", cmd->data.str);
    c->gridname = strdup(cmd->data.str);
    return NULL;
}

static DOTCONF_CB(cb_authority)
{
   /* See gmetad.h for why we record strings this way. */
    debug_msg("Grid authority %s", cmd->data.str);
    root.authority_ptr = 0;
    strcpy(root.strings, cmd->data.str);
    root.stringslen += strlen(root.strings) + 1;
    return NULL;
}

static DOTCONF_CB(cb_all_trusted)
{
   gmetad_config_t *c = (gmetad_config_t*) cmd->option->info;
   debug_msg("All hosts are trusted!");
   c->all_trusted = 1;
   return NULL;
}

static DOTCONF_CB(cb_rras)
{
  int i;
  gmetad_config_t *c = (gmetad_config_t *)cmd->option->info;

  /* free the old data */
  for(i=0; i< c->num_rras; i++)
    {
      free(c->rras[i]);
    }
  free(c->rras);

  c->num_rras = cmd->arg_count;
  c->rras = (char **) malloc (sizeof(char *)* c->num_rras);
  if(!c->rras)
    {
      fprintf(stderr,"Unable to malloc memory for round-robin archives\n");
      exit(1);
    }

  for(i=0; i< c->num_rras; i++)
    {
      c->rras[i] = strdup( cmd->data.list[i] );
    }

  return NULL;
}

static DOTCONF_CB(cb_trusted_hosts)
{
   int i,rv;
   llist_entry *le;
   struct sockaddr_in sa;
   gmetad_config_t *c = (gmetad_config_t*) cmd->option->info;

   for (i = 0; i < cmd->arg_count; i++)
      {
         le = (llist_entry *)malloc(sizeof(llist_entry));
         rv = g_gethostbyname( cmd->data.list[i], &sa, NULL);
         if (!rv) {
            err_msg("Warning: we failed to resolve trusted host name %s", cmd->data.list[i]);
            continue;
         }
         le->val = (char*) malloc(MAXHOSTNAME);
         inet_ntop(AF_INET, &sa.sin_addr, le->val, MAXHOSTNAME);
         llist_add(&(c->trusted_hosts), le);
      }
   return NULL;
}

static DOTCONF_CB(cb_data_source)
{
   unsigned int i;
   data_source_list_t *dslist;
   datum_t key, val, *find;
   unsigned long step;
   char *p, *str, *port;
   char *endptr;

   dslist = (data_source_list_t *) malloc ( sizeof(data_source_list_t) );
   if(!dslist)
      {
         err_quit("Unable to malloc data source list");
      }

   dslist->name = strdup( cmd->data.list[0] );

  /* Set data source step (avg polling interval). Default is 15s.
   * Be careful of the case where the source is an ip address,
   * in which case endptr = '.'
   */
  i=1;
  step=strtoul(cmd->data.list[i], &endptr, 10);
   if (step && *endptr == '\0')
      {
         dslist->step = step;
         i++;
      }
   else
      dslist->step = 15;

   dslist->names = (char **) malloc( (cmd->arg_count - 1) * sizeof(char *) );
   if (! dslist->names )
      err_quit("Unable to malloc names array");

   dslist->ports = (char **) malloc( (cmd->arg_count - 1) * sizeof(char *));
   if (! dslist->ports )
      err_quit("Unable to malloc ports array");

   dslist->num_sources = 0;

   for ( ; i< cmd->arg_count; i++)
      {
         str = cmd->data.list[i];
   
         p = strchr( str, ':' );
         if( p )
            {
               /* Port is specified */
               *p = '\0';
               port = strdup( p+1 );
            }
         else
            {
               port = strdup("8649");
            }

         dslist->names[dslist->num_sources] = strdup(str);
         dslist->ports[dslist->num_sources] = port;
 
         dslist->num_sources++;
      }

   key.data = cmd->data.list[0];
   key.size = strlen(key.data) + 1;

   val.data = &dslist;
   val.size = sizeof(dslist);

   find  = hash_insert( &key, &val, sources );
   if(!find)
         err_quit("Unable to insert list pointer into source hash\n");
   
   debug_msg("Data inserted for [%s] into sources hash", key.data);
   return NULL;
}

static DOTCONF_CB(cb_debug_level)
{
   gmetad_config_t *c = (gmetad_config_t*) cmd->option->info;
   c->debug_level = cmd->data.value;
   debug_msg("Setting the debug level to %d", cmd->data.value);
   return NULL;
}

static DOTCONF_CB(cb_xml_port)
{
   gmetad_config_t *c = (gmetad_config_t*) cmd->option->info;
   debug_msg("Setting xml port to %d", cmd->data.value);
   c->xml_port = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_interactive_port)
{
   gmetad_config_t *c = (gmetad_config_t*) cmd->option->info;
   debug_msg("Setting interactive port to %d", cmd->data.value);
   c->interactive_port = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_server_threads)
{
   gmetad_config_t *c = (gmetad_config_t*) cmd->option->info;
   debug_msg("Setting number of xml server threads to %d", cmd->data.value);
   c->server_threads = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_rrd_rootdir)
{
   gmetad_config_t *c = (gmetad_config_t*) cmd->option->info;
   debug_msg("Setting the RRD Rootdir to %s", cmd->data.str);
   c->rrd_rootdir = strdup (cmd->data.str);
   return NULL;
}

static DOTCONF_CB(cb_setuid_username)
{
   gmetad_config_t *c = (gmetad_config_t*) cmd->option->info;
   debug_msg("Setting setuid username to %s", cmd->data.str);
   c->setuid_username = strdup(cmd->data.str);
   return NULL;
}

static DOTCONF_CB(cb_setuid)
{
   gmetad_config_t *c = (gmetad_config_t*) cmd->option->info;
   c->should_setuid = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_xml_compression_level)
{
   gmetad_config_t *c = (gmetad_config_t*) cmd->option->info;
   c->xml_compression_level = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_scalable)
{  
   gmetad_config_t *c = (gmetad_config_t*) cmd->option->info;
   debug_msg("Setting scalable = %s", cmd->data.str);
   if (!strcmp(cmd->data.str, "off"))
      c->scalable_mode = 0;
   return NULL;
}

static FUNC_ERRORHANDLER(errorhandler)
{
   err_quit("gmetad config file error: %s\n", msg);
   return 0;
}

static configoption_t gmetad_options[] =
   {
      {"data_source", ARG_LIST, cb_data_source, &gmetad_config, 0},
      {"gridname", ARG_STR, cb_gridname, &gmetad_config, 0},
      {"authority", ARG_STR, cb_authority, &gmetad_config, 0},
      {"trusted_hosts", ARG_LIST, cb_trusted_hosts, &gmetad_config, 0},
      {"all_trusted", ARG_INT, cb_all_trusted, &gmetad_config, 0},
      {"debug_level",  ARG_INT,  cb_debug_level, &gmetad_config, 0},
      {"xml_port",  ARG_INT, cb_xml_port, &gmetad_config, 0},
      {"interactive_port", ARG_INT, cb_interactive_port, &gmetad_config, 0},
      {"server_threads", ARG_INT, cb_server_threads, &gmetad_config, 0},
      {"rrd_rootdir", ARG_STR, cb_rrd_rootdir, &gmetad_config, 0},
      {"setuid", ARG_TOGGLE, cb_setuid, &gmetad_config, 0},
      {"setuid_username", ARG_STR, cb_setuid_username, &gmetad_config, 0},
      {"scalable", ARG_STR, cb_scalable, &gmetad_config, 0},
      {"xml_compression_level", ARG_INT, cb_xml_compression_level, &gmetad_config, 0},
      {"round_robin_archives", ARG_STR, cb_rras, &gmetad_config, 0},
      LAST_OPTION
   };

static void
set_defaults (gmetad_config_t *config)
{
   /* Gmetad defaults */
   config->gridname = "unspecified";
   config->xml_port = 8651;
   config->xml_compression_level = 0; /* no compression */
   config->interactive_port = 8652;
   config->server_threads = 4;
   config->trusted_hosts = NULL;
   config->debug_level = 0;
   config->should_setuid = 1;
   config->setuid_username = "nobody";
   config->rrd_rootdir = "/var/lib/ganglia/rrds";
   config->scalable_mode = 1;
   config->all_trusted = 0;
   /* round-robin archives */
   config->num_rras = 4;
   config->rras = (char **) malloc (sizeof(char *) * config->num_rras);
   config->rras[0] = strdup("RRA:AVERAGE:0.5:15:240");  /* 1 hour of 15 sec samples */
   config->rras[1] = strdup("RRA:AVERAGE:0.5:360:240"); /* 1 day of 6 minute samples */
   config->rras[2] = strdup("RRA:AVERAGE:0.5:3600:744");/* 1 month of hourly samples */
   config->rras[3] = strdup("RRA:AVERAGE:0.5:86400:365");/* 1 year of daily samples */
}

int
parse_config_file ( char *config_file )
{
   configfile_t *configfile;

   set_defaults(&gmetad_config);

   configfile = dotconf_create( config_file, gmetad_options, 0, CASE_INSENSITIVE );
   if (!configfile)
      {
         err_quit("Unable to open config file: %s\n", config_file);
      }

   configfile->errorhandler = (dotconf_errorhandler_t) errorhandler;

   if (dotconf_command_loop(configfile) == 0)
      {
         dotconf_cleanup(configfile);
         err_quit("dotconf_command_loop error");
      }
   return 0;
}

int
number_of_datasources ( char *config_file )
{
   int number_of_sources = 0;
   char buf[1024];
   configfile_t *configfile;

   configfile = dotconf_create( config_file, gmetad_options, 0, CASE_INSENSITIVE );

   while (! dotconf_get_next_line( buf, 1024, configfile ))
      {
         if( strstr( buf, "data_source" ) && (buf[0] != '#') )
            {
               number_of_sources++;
            }
      }
   dotconf_cleanup(configfile);
   return number_of_sources;
}
