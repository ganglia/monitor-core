#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "ganglia_priv.h"
#include "ganglia.h"
#include "default_conf.h"

#include <confuse.h>
#include <apr_pools.h>
#include <apr_strings.h>
#include <apr_tables.h>
#include <apr_net.h>
#include <apr_file_io.h>
#include <apr_network_io.h>
#include <apr_lib.h>
#include <apr_hash.h>
#include <apr_base64.h>
#include <apr_xml.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <fnmatch.h>
#include <curl/curl.h>
#include <openssl/hmac.h>

#define GMOND_CLOUD_CONF "/var/lib/ganglia/gmond-cloud.conf"

static char myhost[APRMAXHOSTLEN+1];

/***** IMPORTANT ************
Any changes that you make to this file need to be reconciled in ./conf.pod
in order for the documentation to be in order with the code 
****************************/

void build_default_gmond_configuration(Ganglia_pool p);
static int Ganglia_cfg_include(cfg_t *cfg, cfg_opt_t *opt, int argc,
                          const char **argv);

static cfg_opt_t cluster_opts[] = {
  CFG_STR("name", "unspecified", CFGF_NONE ),
  CFG_STR("owner", "unspecified", CFGF_NONE ),
  CFG_STR("latlong", "unspecified", CFGF_NONE ),
  CFG_STR("url", "unspecified", CFGF_NONE ),
  CFG_END()
};

static cfg_opt_t host_opts[] = {
  CFG_STR("location", "unspecified", CFGF_NONE ),
  CFG_END()
};

static cfg_opt_t globals_opts[] = {
  CFG_BOOL("daemonize", 1, CFGF_NONE),
  CFG_BOOL("setuid", 1 - NO_SETUID, CFGF_NONE),
  CFG_STR("user", SETUID_USER, CFGF_NONE),
  /* later i guess we should add "group" as well */
  CFG_INT("debug_level", 0, CFGF_NONE),
  CFG_INT("max_udp_msg_len", 1472, CFGF_NONE),
  CFG_BOOL("mute", 0, CFGF_NONE),
  CFG_BOOL("deaf", 0, CFGF_NONE),
  CFG_BOOL("allow_extra_data", 1, CFGF_NONE),
  CFG_INT("host_dmax", 86400, CFGF_NONE),
  CFG_INT("host_tmax", 20, CFGF_NONE),
  CFG_INT("cleanup_threshold", 300, CFGF_NONE),
  CFG_BOOL("gexec", 0, CFGF_NONE),
  CFG_INT("send_metadata_interval", 0, CFGF_NONE),
  CFG_STR("module_dir", NULL, CFGF_NONE),
  CFG_STR("override_hostname", NULL, CFGF_NONE),
  CFG_STR("override_ip", NULL, CFGF_NONE),
  CFG_STR("tags", NULL, CFGF_NONE),
  CFG_END()
};

static cfg_opt_t cloud_opts[] = {
  CFG_STR("access_key", NULL, CFGF_NONE),
  CFG_STR("secret_key", NULL, CFGF_NONE),
  CFG_END()
};

static cfg_opt_t discovery_opts[] = {
  CFG_STR("type", NULL, CFGF_NONE),
  CFG_STR("endpoint", "https://ec2.amazonaws.com", CFGF_NONE),
  CFG_STR_LIST("tags", NULL, CFGF_NONE),
  CFG_STR_LIST("groups", NULL, CFGF_NONE),
  CFG_STR_LIST("availability_zones", NULL, CFGF_NONE),
  CFG_STR("host_type", "private_ip", CFGF_NONE),
  CFG_INT("port", -1, CFGF_NONE ),
  CFG_INT("discover_every", 90, CFGF_NONE),
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
  CFG_INT("ttl", 1, CFGF_NONE ),
  CFG_STR("bind", NULL, CFGF_NONE),
  CFG_BOOL("bind_hostname", 0, CFGF_NONE),
  CFG_END()
};

static cfg_opt_t udp_recv_channel_opts[] = {
  CFG_STR("mcast_join", NULL, CFGF_NONE ),
  CFG_STR("bind", NULL, CFGF_NONE ),
  CFG_INT("port", -1, CFGF_NONE ),
  CFG_STR("mcast_if", NULL, CFGF_NONE),
  CFG_SEC("acl", acl_opts, CFGF_NONE), 
  CFG_STR("family", "inet4", CFGF_NONE),
  CFG_BOOL("retry_bind", cfg_true, CFGF_NONE),
  CFG_INT("buffer", 0, CFGF_NONE),
  CFG_END()
};

static cfg_opt_t tcp_accept_channel_opts[] = {
  CFG_STR("bind", NULL, CFGF_NONE ),
  CFG_INT("port", -1, CFGF_NONE ),
  CFG_STR("interface", NULL, CFGF_NONE),
  CFG_SEC("acl", acl_opts, CFGF_NONE),
  CFG_INT("timeout", -1, CFGF_NONE),
  CFG_STR("family", "inet4", CFGF_NONE),
  CFG_END()
};

static cfg_opt_t metric_opts[] = {
  CFG_STR("name", NULL, CFGF_NONE ),
#ifdef HAVE_LIBPCRE
  CFG_STR("name_match", NULL, CFGF_NONE ),
#endif
  CFG_FLOAT("value_threshold", -1, CFGF_NONE),
  CFG_STR("title", NULL, CFGF_NONE ),
  CFG_END()
};

static cfg_opt_t collection_group_opts[] = {
  CFG_SEC("metric", metric_opts, CFGF_MULTI),
  CFG_BOOL("collect_once", 0, CFGF_NONE),  
  CFG_INT("collect_every", 60, CFGF_NONE),    
  CFG_INT("time_threshold", 3600, CFGF_NONE),    /* tmax */
  CFG_END()
};

static cfg_opt_t metric_module_param[] = {
  CFG_STR("value", NULL, CFGF_NONE ),
  CFG_END()
};

static cfg_opt_t metric_module_opts[] = {
  CFG_STR("name", NULL, CFGF_NONE),
  CFG_STR("language", NULL, CFGF_NONE),
  CFG_BOOL("enabled", 1, CFGF_NONE),
  CFG_STR("path", NULL, CFGF_NONE),
  CFG_STR("params", NULL, CFGF_NONE),
  CFG_SEC("param", metric_module_param, CFGF_TITLE | CFGF_MULTI),
  CFG_END()
};

/* 
 * this section can't contain regular options because "modules"
 * is not defined CFGF_MULTI, even if it is presented multiple times
 * in the configuration.
 * CFGF_MULTI sections will collapse and be accessible with one
 * simple cfg_getsec() call though and are OK.
 */
static cfg_opt_t metric_modules_opts[] = {
  CFG_SEC("module", metric_module_opts, CFGF_MULTI),
  CFG_END()
};

#ifdef SFLOW
static cfg_opt_t sflow_opts[] = {
  CFG_INT("udp_port", 6343, CFGF_NONE ),
  CFG_BOOL("accept_vm_metrics", 1, CFGF_NONE ),
  CFG_BOOL("accept_http_metrics", 1, CFGF_NONE ),
  CFG_BOOL("multiple_http_instances", 0, CFGF_NONE ),
  CFG_BOOL("accept_memcache_metrics", 1, CFGF_NONE ),
  CFG_BOOL("multiple_memcache_instances", 0, CFGF_NONE ),
  CFG_BOOL("accept_jvm_metrics", 1, CFGF_NONE ),
  CFG_BOOL("multiple_jvm_instances", 0, CFGF_NONE ),
  CFG_END()
};
#endif

static cfg_opt_t gmond_opts[] = {
  CFG_SEC("cluster",   cluster_opts, CFGF_NONE),
  CFG_SEC("host",      host_opts, CFGF_NONE),
  CFG_SEC("globals",   globals_opts, CFGF_NONE),
  CFG_SEC("cloud",     cloud_opts, CFGF_NONE),
  CFG_SEC("discovery", discovery_opts, CFGF_NONE),
  CFG_SEC("udp_send_channel", udp_send_channel_opts, CFGF_MULTI),
  CFG_SEC("udp_recv_channel", udp_recv_channel_opts, CFGF_MULTI),
  CFG_SEC("tcp_accept_channel", tcp_accept_channel_opts, CFGF_MULTI),
  CFG_SEC("collection_group",  collection_group_opts, CFGF_MULTI),
  CFG_FUNC("include", Ganglia_cfg_include),
  CFG_SEC("modules",  metric_modules_opts, CFGF_NONE),
#ifdef SFLOW
CFG_SEC("sflow", sflow_opts, CFGF_NONE),
#endif
  CFG_END()
}; 

char *
Ganglia_default_collection_groups(void)
{
  return COLLECTION_GROUP_LIST;
}

void
build_default_gmond_configuration(Ganglia_pool p)
{
  apr_pool_t *context=(apr_pool_t*)p;

  default_gmond_configuration = apr_pstrdup(context, BASE_GMOND_CONFIGURATION);
  default_gmond_configuration = apr_pstrcat(context, default_gmond_configuration, DISCOVERY_CONFIGURATION, NULL);
#ifdef SFLOW
  default_gmond_configuration = apr_pstrcat(context, default_gmond_configuration, SFLOW_CONFIGURATION, NULL);
#endif
  default_gmond_configuration = apr_pstrcat(context, default_gmond_configuration, COLLECTION_GROUP_LIST, NULL);
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
Ganglia_pool_create( Ganglia_pool p )
{
  apr_status_t status;
  apr_pool_t *pool=NULL, *parent=(apr_pool_t*)p;

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
  return (Ganglia_pool)pool;
}

void
Ganglia_pool_destroy( Ganglia_pool pool )
{
  apr_pool_destroy((apr_pool_t*)pool);
}

Ganglia_gmond_config
Ganglia_gmond_config_create(char *path, int fallback_to_default)
{
  cfg_t *config = NULL;
  /* Make sure we process ~ in the filename if the shell doesn't */
  char *tilde_expanded = cfg_tilde_expand( path );
  config = cfg_init( gmond_opts, CFGF_NOCASE );

  switch( cfg_parse( config, tilde_expanded ) )
    {
    case CFG_FILE_ERROR:
      /* Unable to open file so we'll go with the configuration defaults */
      err_msg("Configuration file '%s' not found.\n", tilde_expanded);
      if(!fallback_to_default)
        {
          /* Don't fallback to the default configuration.. just exit. */
          exit(1);
        }
      /* .. otherwise use our default configuration */
      if(cfg_parse_buf(config, default_gmond_configuration) == CFG_PARSE_ERROR)
        {
          err_msg("Your default configuration buffer failed to parse. Exiting.\n");
          exit(1);
        }
      break;
    case CFG_PARSE_ERROR:
      err_msg("Parse error for '%s'\n", tilde_expanded );
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
  return (Ganglia_gmond_config)config;
}

struct MemoryStruct
{
  char *memory;
  size_t size;
};

static size_t
WriteMemoryCallback (void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *) userp;

  mem->memory = realloc (mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL)
    {
      /* out of memory! */
      printf ("not enough memory (realloc returned NULL)\n");
      exit (EXIT_FAILURE);
    }

  memcpy (&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

const char *
ec2type (const char *type)
{
  if (!apr_strnatcasecmp (type, "private_ip"))
    {
      return ("privateIpAddress");	/* private-ip-adress */
    }
  else if (!apr_strnatcasecmp (type, "public_ip"))
    {
      return ("ipAddress");	/* ip-address */
    }
  else if (!apr_strnatcasecmp (type, "private_dns"))
    {
      return ("privateDnsName");	/* private-dns-name */
    }
  else
    return ("dnsName");		/* dns-name */
}

Ganglia_udp_send_channels discovered_udp_send_channels = NULL;

Ganglia_udp_send_channels
Ganglia_udp_send_channels_discover (Ganglia_pool p, Ganglia_gmond_config config)
{
  cfg_t *cfg = (cfg_t *) config;
  apr_pool_t *context = (apr_pool_t *) p;
  if (discovered_udp_send_channels == NULL)
    discovered_udp_send_channels = apr_array_make (context, 10, sizeof (apr_socket_t *));	/* init array size of 10 is not max */

  apr_array_header_t *send_channels = (apr_array_header_t *) discovered_udp_send_channels;
  int i;

  if (send_channels)
    {
      for (i = 0; i < send_channels->nelts; i++)
	{
	  apr_sockaddr_t *remotesa = NULL;
	  char remoteip[256];

	  apr_socket_t *s = ((apr_socket_t **) (send_channels->elts))[i];
	  apr_socket_addr_get (&remotesa, APR_REMOTE, s);
	  apr_sockaddr_ip_buffer_get (remoteip, 256, remotesa);
	  debug_msg ("discovered send channels now = %s", remoteip);
	}
    }
  else
    {
      debug_msg ("No discovered channels yet");
    }


  char *cloud_access_key = NULL;
  char *cloud_secret_key = NULL;

  cfg_t *cloud;
  cloud = cfg_getsec (cfg, "cloud");
  cloud_access_key = cfg_getstr (cloud, "access_key");
  cloud_secret_key = cfg_getstr (cloud, "secret_key");

  if (cloud_access_key == NULL || cloud_secret_key == NULL)
    {
      err_msg ("[discovery.cloud] Must supply both the AWS access key id and the AWS secret key for EC2 discovery.\n");
      exit (1);
    }

  char *last_four;
  last_four = cloud_secret_key + strlen(cloud_secret_key) - 4;
  debug_msg ("[discovery.cloud] access key=%s, secret key=************************************%s", cloud_access_key, last_four);

  char *discovery_type = NULL;
  char *discovery_host_type = NULL;
  int discover_every = 0;

  char *filters = "&Filter.1.Name=instance-state-name&Filter.1.Value=running";
  int filter_num = 1;

  char *groups = "", *tags = "", *zones = "";

  cfg_t *discovery;
  discovery = cfg_getsec (cfg, "discovery");
  discovery_type = cfg_getstr (discovery, "type");
  discovery_host_type = cfg_getstr (discovery, "host_type");
  discover_every = cfg_getint (discovery, "discover_every");

  int port;
  port = cfg_getint (discovery, "port");

  CURLcode res;
  CURL *curl_handle;

  curl_handle = curl_easy_init ();

  struct MemoryStruct chunk;
  chunk.memory = malloc (1);	/* will be grown as needed by the realloc above */
  chunk.size = 0;		/* no data at this point */

  if (curl_handle)
    {
      /* Construct filter using security groups, tags and availability zones */
      char *key, *value, *last, *tag;
      for (i = 0; i < cfg_size (discovery, "tags"); i++)
	{
	  filter_num++;
	  tag = apr_pstrdup (context, cfg_getnstr (discovery, "tags", i));
	  key = apr_strtok (tag, "= ", &last);
	  value = apr_strtok (NULL, "= ", &last);
	  filters =
	    apr_pstrcat (context, filters, "&Filter.", apr_itoa(context, filter_num), ".Name=tag%3A",
			 curl_easy_escape (curl_handle, key, 0), "&Filter.", apr_itoa(context, filter_num), ".Value=", curl_easy_escape (curl_handle, value, 0), NULL);
	  tags = apr_pstrcat (context, cfg_getnstr (discovery, "tags", i), ",", tags, NULL);
	}
      if (strlen (tags))
	tags[strlen (tags) - 1] = '\0';

      for (i = 0; i < cfg_size (discovery, "groups"); i++)
	{
	  filter_num++;
	  filters =
	    apr_pstrcat (context, filters, "&Filter.", apr_itoa(context, filter_num),
			 ".Name=group-name", "&Filter.", apr_itoa(context, filter_num), ".Value=", curl_easy_escape (curl_handle, cfg_getnstr (discovery, "groups", i), 0), NULL);
	  groups = apr_pstrcat (context, cfg_getnstr (discovery, "groups", i), ",", groups, NULL);
	}
      if (strlen (groups))
	groups[strlen (groups) - 1] = '\0';

      for (i = 0; i < cfg_size (discovery, "availability_zones"); i++)
	{
	  filter_num++;
	  filters =
	    apr_pstrcat (context, filters, "&Filter.", apr_itoa(context, filter_num),
			 ".Name=availability-zone", "&Filter.", apr_itoa(context, filter_num), ".Value=", curl_easy_escape (curl_handle,
												      cfg_getnstr (discovery, "availability_zones", i), 0),
			 NULL);
	  zones = apr_pstrcat (context, cfg_getnstr (discovery, "availability_zones", i), ",", zones, NULL);
	}
      if (strlen (zones))
	zones[strlen (zones) - 1] = '\0';

      debug_msg ("[discovery.%s] using host_type [%s], tags [%s], groups [%s], availability_zones [%s]", discovery_type, discovery_host_type, tags, groups,
		 zones);

      /* Query EC2 API using curl */
      char timestamp[30];
      apr_size_t len;
      apr_time_exp_t t;
      apr_time_exp_lt (&t, apr_time_now ());
      apr_strftime (timestamp, &len, sizeof (timestamp), "%Y-%m-%dT%H%%3A%M%%3A%SZ", &t);

      char *endpoint;
      endpoint = cfg_getstr (discovery, "endpoint");

      char *request_uri;
      request_uri = endpoint;
      if (strncasecmp (request_uri, "http://", 7) == 0)
	request_uri += 7;
      else if (strncasecmp (request_uri, "https://", 8) == 0)
	request_uri += 8;

      debug_msg ("[discovery.%s] using endpoint %s -> %s", discovery_type, endpoint, request_uri);

      char *query_string;
      query_string =
	apr_pstrcat (context, "AWSAccessKeyId=", cloud_access_key,
		     "&Action=DescribeInstances",
		     (filters ? filters : ""), "&SignatureMethod=HmacSHA256", "&SignatureVersion=2", "&Timestamp=", timestamp, "&Version=2012-08-15", NULL);

      char *signature_string;
      signature_string = apr_pstrcat (context, "GET\n", request_uri, "\n", "/\n", query_string, NULL);

      char hash[EVP_MAX_MD_SIZE];
      unsigned int hlen;
      HMAC (EVP_sha256 (), cloud_secret_key, strlen (cloud_secret_key), (unsigned char *) signature_string, strlen (signature_string), (unsigned char *) hash,
	    &hlen);

      /* base64 encode the signature string */
      int elen;
      char *encbuf;
      elen = apr_base64_encode_len (hlen);
      encbuf = apr_palloc (context, elen);
      apr_base64_encode (encbuf, hash, hlen);

      char *urlencoded_hash = curl_easy_escape (curl_handle, encbuf, 0);

      char *request;
      request = apr_pstrcat (context, endpoint, "?", query_string, "&Signature=", urlencoded_hash, NULL);

      debug_msg ("[discovery.%s] URL-encoded API request %s", discovery_type, request);

      curl_easy_setopt (curl_handle, CURLOPT_URL, request);

      /* send all data to this function  */
      curl_easy_setopt (curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

      /* we pass our 'chunk' struct to the callback function */
      curl_easy_setopt (curl_handle, CURLOPT_WRITEDATA, (void *) &chunk);

      res = curl_easy_perform (curl_handle);

      /* Check for errors */
      if (res != CURLE_OK)
	{
	  err_msg ("[discovery.%s] curl_easy_perform() failed: %s", discovery_type, curl_easy_strerror (res));
	  curl_easy_cleanup (curl_handle);
	  if (chunk.memory)
	    free (chunk.memory);
	  return (Ganglia_udp_send_channels) discovered_udp_send_channels;
	}

      long http_code = 0;
      curl_easy_getinfo (curl_handle, CURLINFO_RESPONSE_CODE, &http_code);

      /* always cleanup */
      curl_easy_cleanup (curl_handle);
      debug_msg ("[discovery.%s] HTTP response code %ld, %lu bytes retrieved", discovery_type, http_code, (long) chunk.size);
    }

  char *response = chunk.memory;
  if (chunk.memory)
    free (chunk.memory);

  // debug_msg("[discovery.%s] API response %s", discovery_type, response);

  /* Parse XML response */
  apr_xml_doc *doc;
  apr_xml_parser *parser = apr_xml_parser_create (context);

  if (apr_xml_parser_feed (parser, response, strlen (response)) != APR_SUCCESS)
    {
      err_msg ("[discovery.%s] could not start the XML parser", discovery_type);
      return (Ganglia_udp_send_channels) discovered_udp_send_channels;
    }
  if (apr_xml_parser_done (parser, &doc) != APR_SUCCESS)
    {
      err_msg ("[discovery.%s] could not parse XML", discovery_type);
      return (Ganglia_udp_send_channels) discovered_udp_send_channels;
    }

  apr_xml_elem *root;
  root = doc->root;
  const apr_xml_elem *elem;
  char *instance_id = NULL;
  char *ec2host = NULL;

  apr_hash_t *instances = NULL;
  instances = apr_hash_make (context);

  for (elem = root->first_child; elem; elem = elem->next)
    {
      const apr_xml_elem *elem2;
      for (elem2 = elem->first_child; elem2; elem2 = elem2->next)
	{
	  const apr_xml_elem *elem3;
	  for (elem3 = elem2->first_child; elem3; elem3 = elem3->next)
	    {
	      const apr_xml_elem *elem4;
	      for (elem4 = elem3->first_child; elem4; elem4 = elem4->next)
		{
		  const apr_xml_elem *elem5;
		  for (elem5 = elem4->first_child; elem5; elem5 = elem5->next)
		    {
		      if (apr_strnatcmp (elem5->name, "instanceId") == 0 && elem5->first_cdata.first != NULL)
			{
			  instance_id = apr_pstrdup (context, elem5->first_cdata.first->text);
			}
		      if (apr_strnatcmp (elem5->name, ec2type (discovery_host_type)) == 0 && elem5->first_cdata.first != NULL)
			{
			  ec2host = apr_pstrdup (context, elem5->first_cdata.first->text);
			  apr_hash_set (instances, ec2host, APR_HASH_KEY_STRING, instance_id);
			}
		      if (apr_strnatcmp (elem5->name, "groupSet") == 0)
			{
			  const apr_xml_elem *elem6;
			  for (elem6 = elem5->first_child; elem6; elem6 = elem6->next)
			    {
			      const apr_xml_elem *elem7;
			      for (elem7 = elem6->first_child; elem7; elem7 = elem7->next)
				{
				  if (apr_strnatcmp (elem7->name, "groupName") == 0 && elem7->first_cdata.first != NULL)
				    debug_msg ("[discovery.%s] %s security group %s", discovery_type, instance_id, elem7->first_cdata.first->text);
				}
			    }
			}
		    }
		}
	    }
	}
    }

  if (apr_hash_count (instances) == 0)
    {
      err_msg ("[discovery.%s] Did not discover any matching nodes. Leaving current send channels unchanged. Retry in %d seconds...", discovery_type,
	       discover_every);
      return (Ganglia_udp_send_channels) discovered_udp_send_channels;
    }
  else
    {
      debug_msg ("[discovery.%s] Found %d matching instances", discovery_type, apr_hash_count (instances));
    }

  apr_hash_t *open_sockets = NULL;
  open_sockets = apr_hash_make (context);

  apr_array_header_t *tmp_send_channels = NULL;
  tmp_send_channels = apr_array_make (context, 10, sizeof (apr_socket_t *));	/* init array size of 10 is not max */

  if (discovered_udp_send_channels != NULL)
    {
      apr_array_header_t *chnls = (apr_array_header_t *) discovered_udp_send_channels;
      for (i = 0; i < chnls->nelts; i++)
	{
	  apr_sockaddr_t *remotesa = NULL;
	  char remoteip[256];

	  apr_socket_t *s = ((apr_socket_t **) (chnls->elts))[i];
	  apr_socket_addr_get (&remotesa, APR_REMOTE, s);
	  apr_sockaddr_ip_buffer_get (remoteip, 256, remotesa);
	  if (!apr_hash_get (instances, remoteip, APR_HASH_KEY_STRING))
	    {
              debug_msg ("[discovery.%s] removing udp send channel %s %s:%d", discovery_type, discovery_host_type, remoteip, port);
	      apr_socket_close (s);
	    }
	  else
	    {
              /* keep track of opened sockets */
	      *(apr_socket_t **) apr_array_push (tmp_send_channels) = s;
	      apr_hash_set (open_sockets, apr_pstrdup (context, remoteip), APR_HASH_KEY_STRING, "OK");
	    }
	}
    }

  apr_hash_index_t *hi;
  const void *k;
  void *v;
  char *hkey;
  char *result;

  cfg_t *globals = (cfg_t*) cfg_getsec( (cfg_t *)cfg, "globals" );
  char *override_hostname = cfg_getstr( globals, "override_hostname" );
  char *override_ip = cfg_getstr( globals, "override_ip" );

  char *cloud_conf = "# This file is generated by cloud-enabled version of Ganglia gmond\n\n";
  cloud_conf = apr_pstrcat (context, cloud_conf, "globals {\n", NULL);
  if (override_hostname) {
    cloud_conf = apr_pstrcat (context, cloud_conf, "override_hostname = ", override_hostname, "\n", NULL);
  if (override_ip) {
    cloud_conf = apr_pstrcat (context, cloud_conf, "override_ip = ", override_ip, "\n", NULL);
  cloud_conf = apr_pstrcat (context, cloud_conf, "}\n\n", NULL);

  for (hi = apr_hash_first (context, instances); hi; hi = apr_hash_next (hi))
    {
      apr_hash_this (hi, &k, NULL, &v);
      hkey = k;

      if (!apr_hash_get (open_sockets, hkey, APR_HASH_KEY_STRING))
	{
          debug_msg ("[discovery.%s] adding %s, udp send channel %s %s:%d", discovery_type, v, discovery_host_type, hkey, port);
	  apr_socket_t *socket = NULL;
	  apr_pool_t *pool = NULL;
	  apr_pool_create (&pool, context);

	  socket = create_udp_client (pool, hkey, port, NULL, NULL, 0);
	  if (!socket)
	    {
	      err_msg ("Unable to create UDP client for %s:%d. Exiting.\n", hkey, port);
	      exit (1);
	    }

	  *(apr_socket_t **) apr_array_push (tmp_send_channels) = socket;
	}

      cloud_conf = apr_pstrcat (context, cloud_conf, "udp_send_channel {\n  bind_hostname = yes\n  host = ", hkey, "\n  port = ", apr_itoa(context, port), "\n}\n\n", NULL);
    }

  /* Write out discovered UDP send channels to file for gmetric */
  apr_file_t *file;
  const char *fname = GMOND_CLOUD_CONF;

  if (apr_file_open (&file, fname, APR_WRITE | APR_CREATE, APR_UREAD | APR_UWRITE | APR_GREAD, context) != APR_SUCCESS)
    {
      err_msg ("[discovery.%s] Failed to open cloud configuration file %s", discovery_type, GMOND_CLOUD_CONF);
    }
  else
    {
      apr_size_t bytes = strlen(cloud_conf);
      if (apr_file_write (file, cloud_conf, &bytes) != APR_SUCCESS)
	{
	  err_msg ("[discovery.%s] Failed to write cloud configuration to file %s", discovery_type, GMOND_CLOUD_CONF);
	}
      apr_file_trunc (file, bytes);
      apr_file_close (file);
    }

  discovered_udp_send_channels = apr_array_copy (context, tmp_send_channels);

  return (Ganglia_udp_send_channels) discovered_udp_send_channels;
}

Ganglia_udp_send_channels
Ganglia_udp_send_channels_create( Ganglia_pool p, Ganglia_gmond_config config )
{
  apr_array_header_t *send_channels = NULL;
  cfg_t *cfg=(cfg_t *)config;
  int i, num_udp_send_channels = cfg_size( cfg, "udp_send_channel");
  apr_pool_t *context = (apr_pool_t*)p;

  /* Return null if there are no send channels specified */
  if(num_udp_send_channels <= 0)
    return (Ganglia_udp_send_channels)send_channels;

  /* Create my UDP send array */
  send_channels = apr_array_make( context, num_udp_send_channels, 
                                  sizeof(apr_socket_t *));

  for(i = 0; i< num_udp_send_channels; i++)
    {
      cfg_t *udp_send_channel;
      char *mcast_join, *mcast_if, *host;
      int port, ttl, bind_hostname;
      apr_socket_t *socket = NULL;
      apr_pool_t *pool = NULL;
      char *bind_address;

      udp_send_channel = cfg_getnsec( cfg, "udp_send_channel", i);
      host           = cfg_getstr( udp_send_channel, "host" );
      mcast_join     = cfg_getstr( udp_send_channel, "mcast_join" );
      mcast_if       = cfg_getstr( udp_send_channel, "mcast_if" );
      port           = cfg_getint( udp_send_channel, "port");
      ttl            = cfg_getint( udp_send_channel, "ttl");
      bind_address   = cfg_getstr( udp_send_channel, "bind" );
      bind_hostname  = cfg_getbool( udp_send_channel, "bind_hostname");

      debug_msg("udp_send_channel mcast_join=%s mcast_if=%s host=%s port=%d\n",
                mcast_join? mcast_join:"NULL", 
                mcast_if? mcast_if:"NULL",
                host? host:"NULL",
                port);

      if(bind_address != NULL && bind_hostname == cfg_true)
        {
          err_msg("udp_send_channel: bind and bind_hostname are mutually exclusive, both parameters can't be specified for the same udp_send_channel\n");
          exit(1);
        }

      /* Create a subpool */
      apr_pool_create(&pool, context);

      /* Join the specified multicast channel */
      if( mcast_join )
        {
          /* We'll be listening on a multicast channel */
          socket = create_mcast_client(pool, mcast_join, port, ttl, mcast_if, bind_address, bind_hostname);
          if(!socket)
            {
              err_msg("Unable to join multicast channel %s:%d. Exiting\n",
                  mcast_join, port);
              exit(1);
            }
        }
      else
        {
          /* Create a UDP socket */
          socket = create_udp_client( pool, host, port, mcast_if, bind_address, bind_hostname );
          if(!socket)
            {
              err_msg("Unable to create UDP client for %s:%d. Exiting.\n",
                      host? host: "NULL", port);
              exit(1);
            }
        }

      /* Add the socket to the array */
      *(apr_socket_t **)apr_array_push(send_channels) = socket;
    }

  return (Ganglia_udp_send_channels)send_channels;
}


/* This function will send a datagram to every udp_send_channel specified */
int
Ganglia_udp_send_message(Ganglia_udp_send_channels channels, char *buf, int len )
{
  apr_status_t status;
  int i;
  int num_errors = 0;
  apr_size_t size;
  apr_array_header_t *chnls=(apr_array_header_t*)channels;

  if(!chnls || !buf || len<=0)
    return 1;

  for(i=0; i< chnls->nelts; i++)
    {
      apr_socket_t *socket = ((apr_socket_t **)(chnls->elts))[i];
      size   = len;
      status = apr_socket_send( socket, buf, &size );
      if(status != APR_SUCCESS)
        {
          num_errors++;
        }
    }
  return num_errors;
}

Ganglia_metric
Ganglia_metric_create( Ganglia_pool parent_pool )
{
  apr_pool_t *pool = (apr_pool_t*)Ganglia_pool_create(parent_pool);
  Ganglia_metric gmetric;
  if(!pool)
    {
      return NULL;
    }
  gmetric = apr_pcalloc( pool, sizeof(struct Ganglia_metric));
  if(!gmetric)
    {
      Ganglia_pool_destroy((Ganglia_pool)pool);
      return NULL;
    }

  gmetric->pool = (Ganglia_pool)pool;
  gmetric->msg  = apr_pcalloc( pool, sizeof(struct Ganglia_metadata_message));
  if(!gmetric->msg)
    {
      Ganglia_pool_destroy((Ganglia_pool)pool);
      return NULL;
    }
  gmetric->extra = (void*)apr_table_make(pool, 2);

  return gmetric;
}

int
Ganglia_metadata_send( Ganglia_metric gmetric, Ganglia_udp_send_channels send_channels )
{
  return Ganglia_metadata_send_real( gmetric, send_channels, NULL );
}

int
Ganglia_metadata_send_real( Ganglia_metric gmetric, Ganglia_udp_send_channels send_channels, char *override_string )
{
  int len, i;
  XDR x;
  char gmetricmsg[GANGLIA_MAX_MESSAGE_LEN];
  Ganglia_metadata_msg msg;
  const apr_array_header_t *arr;
  const apr_table_entry_t *elts;
  const char *spoof = SPOOF;
  apr_pool_t *gm_pool=(apr_pool_t*)gmetric->pool;

  if (myhost[0] == '\0') 
      apr_gethostname( (char*)myhost, APRMAXHOSTLEN+1, gm_pool);

  msg.id = gmetadata_full;
  memcpy( &(msg.Ganglia_metadata_msg_u.gfull.metric), gmetric->msg, sizeof(Ganglia_metadata_message));
  msg.Ganglia_metadata_msg_u.gfull.metric_id.name = apr_pstrdup (gm_pool, gmetric->msg->name);
  debug_msg("  msg.Ganglia_metadata_msg_u.gfull.metric_id.name: %s\n", msg.Ganglia_metadata_msg_u.gfull.metric_id.name);
  if ( override_string != NULL )
    {
      msg.Ganglia_metadata_msg_u.gfull.metric_id.host = apr_pstrdup (gm_pool, (char*)override_string);
      debug_msg("  msg.Ganglia_metadata_msg_u.gfull.metric_id.host: %s\n", msg.Ganglia_metadata_msg_u.gfull.metric_id.host);
      msg.Ganglia_metadata_msg_u.gfull.metric_id.spoof = TRUE;
    }
    else
    {
      msg.Ganglia_metadata_msg_u.gfull.metric_id.host = apr_pstrdup (gm_pool, (char*)myhost);
      debug_msg("  msg.Ganglia_metadata_msg_u.gfull.metric_id.host: %s\n", msg.Ganglia_metadata_msg_u.gfull.metric_id.host);
      msg.Ganglia_metadata_msg_u.gfull.metric_id.spoof = FALSE;
    }

  arr = apr_table_elts(gmetric->extra);
  elts = (const apr_table_entry_t *)arr->elts;
  msg.Ganglia_metadata_msg_u.gfull.metric.metadata.metadata_len = arr->nelts;
  msg.Ganglia_metadata_msg_u.gfull.metric.metadata.metadata_val = 
      (Ganglia_extra_data*)apr_pcalloc(gm_pool, sizeof(Ganglia_extra_data)*arr->nelts);

  /* add all of the metadata to the packet */
  for (i = 0; i < arr->nelts; ++i) {
      if (elts[i].key == NULL)
          continue;

      /* Replace the host name with the spoof host if it exists in the metadata */
      if ((apr_toupper(elts[i].key[0]) == spoof[0]) && strcasecmp(SPOOF_HOST, elts[i].key) == 0) 
        {
          msg.Ganglia_metadata_msg_u.gfull.metric_id.host = apr_pstrdup (gm_pool, elts[i].val);
          msg.Ganglia_metadata_msg_u.gfull.metric_id.spoof = TRUE;
        }
      if ((apr_toupper(elts[i].key[0]) == spoof[0]) && strcasecmp(SPOOF_HEARTBEAT, elts[i].key) == 0) 
        {
          msg.Ganglia_metadata_msg_u.gfull.metric_id.name = apr_pstrdup (gm_pool, "heartbeat");
          msg.Ganglia_metadata_msg_u.gfull.metric.name = msg.Ganglia_metadata_msg_u.gfull.metric_id.name;
          msg.Ganglia_metadata_msg_u.gfull.metric_id.spoof = TRUE;
        }

      msg.Ganglia_metadata_msg_u.gfull.metric.metadata.metadata_val[i].name = 
          apr_pstrdup(gm_pool, elts[i].key);
      msg.Ganglia_metadata_msg_u.gfull.metric.metadata.metadata_val[i].data = 
          apr_pstrdup(gm_pool, elts[i].val);
  }

  /* Send the message */
  xdrmem_create(&x, gmetricmsg, GANGLIA_MAX_MESSAGE_LEN, XDR_ENCODE);
  if(!xdr_Ganglia_metadata_msg(&x, &msg))
    {
      return 1;
    }
  len = xdr_getpos(&x); 
  /* Send the encoded data along...*/
  return Ganglia_udp_send_message( send_channels, gmetricmsg, len);
}

int
Ganglia_value_send_real( Ganglia_metric gmetric, Ganglia_udp_send_channels send_channels, char *override_string )
{
  int len, i;
  XDR x;
  char gmetricmsg[GANGLIA_MAX_MESSAGE_LEN];
  Ganglia_value_msg msg;
  const apr_array_header_t *arr;
  const apr_table_entry_t *elts;
  const char *spoof = SPOOF;
  apr_pool_t *gm_pool=(apr_pool_t*)gmetric->pool;

  if (myhost[0] == '\0') 
      apr_gethostname( (char*)myhost, APRMAXHOSTLEN+1, gm_pool);

  msg.id = gmetric_string;
  if (override_string != NULL)
    {
      msg.Ganglia_value_msg_u.gstr.metric_id.host = apr_pstrdup (gm_pool, (char*)override_string);
      msg.Ganglia_value_msg_u.gstr.metric_id.spoof = TRUE;
    }
    else
    {
      msg.Ganglia_value_msg_u.gstr.metric_id.host = apr_pstrdup (gm_pool, (char*)myhost);
      msg.Ganglia_value_msg_u.gstr.metric_id.spoof = FALSE;
    }
  msg.Ganglia_value_msg_u.gstr.metric_id.name = apr_pstrdup (gm_pool, gmetric->msg->name);
  msg.Ganglia_value_msg_u.gstr.fmt = apr_pstrdup (gm_pool, "%s");
  msg.Ganglia_value_msg_u.gstr.str = apr_pstrdup (gm_pool, gmetric->value);

  arr = apr_table_elts(gmetric->extra);
  elts = (const apr_table_entry_t *)arr->elts;

  /* add all of the metadata to the packet */
  for (i = 0; i < arr->nelts; ++i) {
      if (elts[i].key == NULL)
          continue;

      /* Replace the host name with the spoof host if it exists in the metadata */
      if ((apr_toupper(elts[i].key[0]) == spoof[0]) && strcasecmp(SPOOF_HOST, elts[i].key) == 0) 
        {
          msg.Ganglia_value_msg_u.gstr.metric_id.host = apr_pstrdup (gm_pool, elts[i].val);
          msg.Ganglia_value_msg_u.gstr.metric_id.spoof = TRUE;
        }
      if ((apr_toupper(elts[i].key[0]) == spoof[0]) && strcasecmp(SPOOF_HEARTBEAT, elts[i].key) == 0) 
        {
          msg.Ganglia_value_msg_u.gstr.metric_id.name = apr_pstrdup (gm_pool, "heartbeat");
          msg.Ganglia_value_msg_u.gstr.metric_id.spoof = TRUE;
        }
  }

  /* Send the message */
  xdrmem_create(&x, gmetricmsg, GANGLIA_MAX_MESSAGE_LEN, XDR_ENCODE);
  if(!xdr_Ganglia_value_msg(&x, &msg))
    {
      return 1;
    }
  len = xdr_getpos(&x); 
  /* Send the encoded data along...*/
  return Ganglia_udp_send_message( send_channels, gmetricmsg, len);
}

int
Ganglia_value_send( Ganglia_metric gmetric, Ganglia_udp_send_channels send_channels )
{
  return Ganglia_value_send_real( gmetric, send_channels, NULL );
}

int
Ganglia_metric_send( Ganglia_metric gmetric, Ganglia_udp_send_channels send_channels )
{
  int ret = Ganglia_metadata_send(gmetric, send_channels);
  if (!ret)
      ret = Ganglia_value_send(gmetric, send_channels);
  return ret;
}

void
Ganglia_metric_destroy( Ganglia_metric gmetric )
{
  if(!gmetric)
    return;
  Ganglia_pool_destroy(gmetric->pool);
}


int
check_value( char *type, char* value)
{
char *tail;
int ret=1;
double d;
long l;

  if (strcmp(type,"float")||strcmp(type,"double"))
    d = strtod(value,&tail);
  else
    l = strtol(value,&tail,10);

  if(strlen(tail)==0)
    ret=0;

  return ret;
}

/*
 * struct Ganglia_metadata_message {
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
Ganglia_metric_set( Ganglia_metric gmetric, char *name, char *value, char *type, char *units, unsigned int slope, unsigned int tmax, unsigned int dmax)
{
  apr_pool_t *gm_pool;

  /* Make sure all the params look ok */
  if(!gmetric||!name||!value||!type||!units||slope>4)
    return 1;

  gm_pool = (apr_pool_t*)gmetric->pool;

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

  /* Make sure we have a number for (int8|uint8|int16|uint16|int32|uint32|float|double)*/
  if(strcmp(type, "string") != 0)
    {
      if(check_value(type,value)) return 4;
    }

  /* All the data is there and validated... copy it into the structure */
  gmetric->msg->name = apr_pstrdup( gm_pool, name);
  gmetric->value = apr_pstrdup( gm_pool, value);
  gmetric->msg->type  = apr_pstrdup( gm_pool, type);
  gmetric->msg->units = apr_pstrdup( gm_pool, units);
  gmetric->msg->slope = slope;
  gmetric->msg->tmax = tmax;
  gmetric->msg->dmax = dmax;

  return 0;
}

void
Ganglia_metadata_add( Ganglia_metric gmetric, char *name, char *value)
{
  apr_table_add((apr_table_t*)gmetric->extra, name, value);
  return;
}

ganglia_slope_t cstr_to_slope(const char* str)
{
    if (str == NULL) {
        return GANGLIA_SLOPE_UNSPECIFIED;
    }
    
    if (!strcmp(str, "zero")) {
        return GANGLIA_SLOPE_ZERO;
    }
    
    if (!strcmp(str, "positive")) {
        return GANGLIA_SLOPE_POSITIVE;
    }
    
    if (!strcmp(str, "negative")) {
        return GANGLIA_SLOPE_NEGATIVE;
    }
    
    if (!strcmp(str, "both")) {
        return GANGLIA_SLOPE_BOTH;
    }
    
    if (!strcmp(str, "derivative")) {
        return GANGLIA_SLOPE_DERIVATIVE;
    }
    
    /*
     * well, it might just be _wrong_ too
     * but we'll handle that situation another time
     */
    return GANGLIA_SLOPE_UNSPECIFIED;
}

const char* slope_to_cstr(unsigned int slope)
{
    /* 
     * this function takes a raw int, not a
     * ganglia_slope_t in order to help future
     * unit testing (where any value can be passed in)
     */
    switch (slope) {
    case GANGLIA_SLOPE_ZERO:
        return "zero";
    case GANGLIA_SLOPE_POSITIVE:
        return "positive";
    case GANGLIA_SLOPE_NEGATIVE:
        return "negative";
    case GANGLIA_SLOPE_BOTH:
        return "both";
    case GANGLIA_SLOPE_DERIVATIVE:
        return "derivative";
    case GANGLIA_SLOPE_UNSPECIFIED:
        return "unspecified";
    }
    /*
     * by NOT using a default in the switch statement
     * the compiler will complain if anyone adds
     * to the enum without changing this function.
     */
    return "unspecified";
}

int has_wildcard(const char *pattern)
{
    int nesting;

    nesting = 0;
    while (*pattern) {
        switch (*pattern) {
            case '?':
            case '*':
                return 1;
        
            case '\\':
                if (*pattern++ == '\0') {
                    return 0;
                }
                break;
        
            case '[':   
                ++nesting;
                break;
        
            case ']':
                if (nesting) {
                    return 1;
                }
                break;
        }
        ++pattern;
    }
    return 0;
}

static int 
Ganglia_cfg_include(cfg_t *cfg, cfg_opt_t *opt, int argc,
                          const char **argv)
{
    char *fname = (char*)argv[0];
    struct stat statbuf;
    DIR *dir;
    struct dirent *entry;

    if(argc != 1)
    {
        cfg_error(cfg, "wrong number of arguments to cfg_include()");
        return 1;
    }

    if (stat (fname, &statbuf) == 0) 
    {
        return cfg_include(cfg, opt, argc, argv);
    }
    else if (has_wildcard(fname))
    {
        int ret;
        char *path = calloc(strlen(fname) + 1, sizeof(char));
        char *pattern = NULL;
        char *idx = strrchr(fname, '/');
        apr_pool_t *p;
        apr_file_t *ftemp;
        char *dirname = NULL;
        char tn[] = "gmond.tmp.XXXXXX";

        if (idx == NULL) {
            idx = strrchr(fname, '\\');
        }

        if (idx == NULL) {
            strncpy (path, ".", 1);
            pattern = fname;
        }
        else {
            strncpy (path, fname, idx - fname);
            pattern = idx + 1;
        }

        apr_pool_create(&p, NULL);
        if (apr_temp_dir_get((const char**)&dirname, p) != APR_SUCCESS) {
#ifndef LINUX
            cfg_error(cfg, "failed to determine the temp dir");
            apr_pool_destroy(p);
            return 1;
#else
            /*
             * workaround APR BUG46297 by using the POSIX shared memory
             * ramdrive that is available since glibc 2.2
             */
            dirname = apr_psprintf(p, "%s", "/dev/shm");
#endif
        }
        dirname = apr_psprintf(p, "%s/%s", dirname, tn);

        if (apr_file_mktemp(&ftemp, dirname, 
                            APR_CREATE | APR_READ | APR_WRITE | APR_DELONCLOSE, 
                            p) != APR_SUCCESS) {
            cfg_error(cfg, "unable to create a temporary file %s", dirname);
            apr_pool_destroy(p);
            return 1;
        }

        dir = opendir(path);

        if(dir != NULL){
            while((entry = readdir(dir)) != NULL) {
                ret = fnmatch(pattern, entry->d_name, 
                              FNM_PATHNAME|FNM_PERIOD);
                if (ret == 0) {
                    char *newpath, *line;

                    newpath = apr_psprintf (p, "%s/%s", path, entry->d_name);
                    line = apr_pstrcat(p, "include ('", newpath, "')\n", NULL);
                    apr_file_puts(line, ftemp);
                }
            }
            closedir(dir);
            free (path);

            argv[0] = dirname;
            if (cfg_include(cfg, opt, argc, argv))
                cfg_error(cfg, "failed to process include file %s", fname);
            else
                debug_msg("processed include file %s\n", fname);
        }

        apr_file_close(ftemp);
        apr_pool_destroy(p);

        argv[0] = fname;
    }
    else 
    {
        cfg_error(cfg, "invalid include path");
        return 1;
    }

    return 0;
}
