#include <dotconf.h>
#include <string.h>
#include <ganglia/hash.h>
#include <ganglia/llist.h>
#include <gmetad.h>
#include <ganglia.h>

/* Variables that get filled in by configuration file */
extern llist_entry *trusted_hosts;
extern hash_t *sources;
extern int debug_level;
extern int xml_port;
extern int server_threads;
extern char *rrd_rootdir;
extern char *setuid_username;
extern int should_setuid;

extern int source_index;

static DOTCONF_CB(cb_trusted_hosts)
{
   int i;
   llist_entry *le;

   for (i = 0; i < cmd->arg_count; i++)
      {
         le = (llist_entry *)malloc(sizeof(llist_entry));
         le->val = strdup ( cmd->data.list[i] );
         llist_add(&(trusted_hosts), le);
      }
   return NULL;
}

static DOTCONF_CB(cb_data_source)
{
   unsigned int i, len;
   data_source_list_t *dslist;
   g_inet_addr *ia;
   datum_t key, val, *find;
   int port, rv;
   char *p, *str;
   struct sockaddr_in sa;

   source_index++;

   debug_msg("Datasource = [%s]", cmd->data.list[0]);

   dslist = (data_source_list_t *) malloc ( sizeof(data_source_list_t) );
   if(!dslist)
      {
         err_quit("Unable to malloc data source list");
      }

   dslist->num_sources = 0;

   dslist->sources = (g_inet_addr **) malloc( (cmd->arg_count-1) * sizeof(g_inet_addr *) );
   if (! dslist->sources )
      {
         err_quit("Unable to malloc sources array");
      }

   dslist->name = strdup( cmd->data.list[0] );

   for (i = 1; i< cmd->arg_count; i++)
      {
         str = cmd->data.list[i];

         p = strchr( str, ':' );
         if( p )
            {
               /* Port is specified */
               *p = '\0';
               port = atoi ( p+1 );
            }
         else
            {
               port = 8649;
            }

         rv = g_gethostbyname( cmd->data.list[i], &sa, NULL);
         if (!rv) {
            err_msg("Warning: we failed to resolve data source name %s", cmd->data.list[i]);
            continue;
         }
         str = (char*) malloc(MAXHOSTNAMELEN);
         my_inet_ntop(AF_INET, &sa.sin_addr, str, MAXHOSTNAMELEN);

         debug_msg("Trying to connect to %s:%d for [%s]", str, port, dslist->name);
         dslist->sources[dslist->num_sources] = (g_inet_addr *) g_inetaddr_new ( str, port );
         if(! dslist->sources[dslist->num_sources])
            {
               err_quit("Unable to create inetaddr [%s:%d] and save it to [%s]", str, port, dslist->name);
            }
         else
            {
               dslist->num_sources++;
            }
         free(str);
      }

   key.data = cmd->data.list[0];
   key.size = strlen ( key.data ) + 1;

   val.data = &dslist;
   val.size = sizeof(dslist);

   find  = hash_insert( &key, &val, sources );
   if(!find)
      {
         err_quit("Unable to insert list pointer into source hash\n");
      }
   debug_msg("Data inserted for [%s] into sources hash", key.data);
   return NULL;
}

static DOTCONF_CB(cb_debug_level)
{
   debug_level = cmd->data.value;
   debug_msg("Setting debug level to %d", cmd->data.value);
   return NULL;
}

static DOTCONF_CB(cb_xml_port)
{
   debug_msg("Setting xml port to %d", cmd->data.value);
   xml_port = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_server_threads)
{
   debug_msg("Setting number of xml server threads to %d", cmd->data.value);
   server_threads = cmd->data.value;
   return NULL;
}

static DOTCONF_CB(cb_rrd_rootdir)
{
   debug_msg("Setting the RRD Rootdir to %s", cmd->data.str);
   rrd_rootdir = strdup ( cmd->data.str );
   return NULL;
}

static DOTCONF_CB(cb_setuid_username)
{
   debug_msg("Setting setuid username to %s", cmd->data.str);
   setuid_username = strdup( cmd->data.str );
   return NULL;
}

static DOTCONF_CB(cb_setuid)
{
   should_setuid = cmd->data.value;
   return NULL;
}

static FUNC_ERRORHANDLER(errorhandler)
{
   err_quit("gmetad config file error: %s\n", msg);
   return NULL;
}

static configoption_t gmetad_options[] =
   {
      {"data_source", ARG_LIST, cb_data_source, NULL, 0},
      {"trusted_hosts", ARG_LIST, cb_trusted_hosts, NULL, 0},
      {"debug_level",  ARG_INT,  cb_debug_level, NULL, 0},
      {"xml_port",  ARG_INT, cb_xml_port, NULL, 0},
      {"server_threads", ARG_INT, cb_server_threads, NULL, 0},
      {"rrd_rootdir", ARG_STR, cb_rrd_rootdir, NULL, 0},
      {"setuid", ARG_TOGGLE, cb_setuid, NULL, 0},
      {"setuid_username", ARG_STR, cb_setuid_username, NULL, 0},
      LAST_OPTION
   };

int
parse_config_file ( char *config_file )
{
   configfile_t *configfile;
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
