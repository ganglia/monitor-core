/*
 * cloud.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "cloud.h"

#include <confuse.h>
#include <apr_strings.h>
#include <apr_network_io.h>
#include <apr_net.h>
#include <apr_pools.h>
#include <apr_file_io.h>
#include <apr_lib.h>
#include <apr_hash.h>
#include <apr_base64.h>
#include <apr_xml.h>
#include <curl/curl.h>
#include <openssl/hmac.h>

#define GMOND_CLOUD_CONF "/var/lib/ganglia/gmond-cloud.conf"

struct MemoryStruct
{
  char *memory;
  size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL)
    {
      err_msg("Curl write memory callback failed - not enough memory\n");
      exit(1);
    }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

const char *
ec2type(const char *type)
{
  if (!apr_strnatcasecmp(type, "private_ip"))
    {
      return ("privateIpAddress");      /* private-ip-adress */
    }
  else if (!apr_strnatcasecmp(type, "public_ip"))
    {
      return ("ipAddress");     /* ip-address */
    }
  else if (!apr_strnatcasecmp(type, "private_dns"))
    {
      return ("privateDnsName");        /* private-dns-name */
    }
  else
    return ("dnsName");         /* dns-name */
}

Ganglia_udp_send_channels
Ganglia_udp_send_channels_discover(Ganglia_pool p, Ganglia_gmond_config config)
{
  apr_pool_t *context = (apr_pool_t *) p;
  cfg_t *cfg = (cfg_t *) config;

  static apr_array_header_t *discovered_udp_send_channels = NULL;

  if (discovered_udp_send_channels == NULL)
    discovered_udp_send_channels = apr_array_make(context, 10, sizeof(apr_socket_t *)); /* init array size of 10 is not max */

  cfg_t *cloud = cfg_getsec(cfg, "cloud");
  char *cloud_access_key = cfg_getstr(cloud, "access_key");
  char *cloud_secret_key = cfg_getstr(cloud, "secret_key");

  if (cloud_access_key == NULL || cloud_secret_key == NULL)
    {
      err_msg("[discovery.cloud] Must supply both the access key id and secret key for cloud discovery.\n");
      exit(1);
    }

  char *last_four = cloud_secret_key + strlen(cloud_secret_key) - 4;
  debug_msg("[discovery.cloud] access key=%s, secret key=************************************%s", cloud_access_key, last_four);

  cfg_t *discovery = cfg_getsec(cfg, "discovery");
  char *discovery_type = cfg_getstr(discovery, "type");
  char *discovery_host_type = cfg_getstr(discovery, "host_type");
  int discover_every = cfg_getint(discovery, "discover_every");
  int port = cfg_getint(discovery, "port");

  CURL *curl_handle = curl_easy_init();

  struct MemoryStruct chunk;
  chunk.memory = malloc(1);     /* will be grown as needed by the realloc above */
  chunk.size = 0;               /* no data at this point */

  if (curl_handle)
    {
      /* Construct filter using tags, security groups and availability zones */
      char *filters = "&Filter.1.Name=instance-state-name&Filter.1.Value=running";
      int filter_num = 1;
      char *tags = "", *groups = "", *zones = "";

      for (int i = 0; i < cfg_size(discovery, "tags"); i++)
        {
          filter_num++;
          char *tag = apr_pstrdup(context, cfg_getnstr(discovery, "tags", i));
          char *last;
          char *key = apr_strtok(tag, "= ", &last);
          char *value = apr_strtok(NULL, "= ", &last);
          if (!value)
            {
              err_msg("[discovery.%s] Parse error for discovery tags in '/etc/ganglia/gmond.conf' : '%s' has no tag value", discovery_type, key);
              exit(1);
            }
          filters =
            apr_pstrcat(context, filters, "&Filter.",
                        apr_itoa(context, filter_num), ".Name=tag%3A",
                        curl_easy_escape(curl_handle, key, 0), "&Filter.",
                        apr_itoa(context, filter_num), ".Value=", curl_easy_escape(curl_handle, value, 0), NULL);
          tags = apr_pstrcat(context, cfg_getnstr(discovery, "tags", i), ",", tags, NULL);
        }
      if (strlen(tags))
        tags[strlen(tags) - 1] = '\0';

      for (int i = 0; i < cfg_size(discovery, "groups"); i++)
        {
          filter_num++;
          filters =
            apr_pstrcat(context, filters, "&Filter.",
                        apr_itoa(context, filter_num),
                        ".Name=group-name", "&Filter.",
                        apr_itoa(context, filter_num), ".Value=", curl_easy_escape(curl_handle, cfg_getnstr(discovery, "groups", i), 0), NULL);
          groups = apr_pstrcat(context, cfg_getnstr(discovery, "groups", i), ",", groups, NULL);
        }
      if (strlen(groups))
        groups[strlen(groups) - 1] = '\0';

      for (int i = 0; i < cfg_size(discovery, "availability_zones"); i++)
        {
          filter_num++;
          filters =
            apr_pstrcat(context, filters, "&Filter.",
                        apr_itoa(context, filter_num),
                        ".Name=availability-zone", "&Filter.",
                        apr_itoa(context, filter_num), ".Value=", curl_easy_escape(curl_handle, cfg_getnstr(discovery, "availability_zones", i), 0), NULL);
          zones = apr_pstrcat(context, cfg_getnstr(discovery, "availability_zones", i), ",", zones, NULL);
        }
      if (strlen(zones))
        zones[strlen(zones) - 1] = '\0';

      debug_msg
        ("[discovery.%s] using host_type [%s], tags [%s], groups [%s], availability_zones [%s]", discovery_type, discovery_host_type, tags, groups, zones);

      /* Query EC2 API using curl */
      char timestamp[30];
      apr_size_t len;
      apr_time_exp_t t;

      apr_time_exp_lt(&t, apr_time_now());
      apr_strftime(timestamp, &len, sizeof(timestamp), "%Y-%m-%dT%H%%3A%M%%3A%SZ", &t);

      char *endpoint = cfg_getstr(discovery, "endpoint");
      char *request_uri = endpoint;

      if (strncasecmp(request_uri, "http://", 7) == 0)
        request_uri += 7;
      else if (strncasecmp(request_uri, "https://", 8) == 0)
        request_uri += 8;

      debug_msg("[discovery.%s] using endpoint %s -> %s", discovery_type, endpoint, request_uri);

      char *query_string =
        apr_pstrcat(context, "AWSAccessKeyId=", cloud_access_key,
                    "&Action=DescribeInstances", filters,
                    "&SignatureMethod=HmacSHA256", "&SignatureVersion=2",
                    "&Timestamp=", timestamp, "&Version=2012-08-15", NULL);

      char *signature_string = apr_pstrcat(context, "GET\n", request_uri, "\n", "/\n", query_string, NULL);

      char hash[EVP_MAX_MD_SIZE];
      unsigned int hlen;
      HMAC(EVP_sha256(), cloud_secret_key, strlen(cloud_secret_key),
           (unsigned char *) signature_string, strlen(signature_string), (unsigned char *) hash, &hlen);

      /* base64 encode the signature string */
      int elen = apr_base64_encode_len(hlen);
      char *encbuf = apr_palloc(context, elen);
      apr_base64_encode(encbuf, hash, hlen);

      char *urlencoded_hash = curl_easy_escape(curl_handle, encbuf, 0);
      char *request = apr_pstrcat(context, endpoint, "?", query_string, "&Signature=", urlencoded_hash, NULL);

      debug_msg("[discovery.%s] URL-encoded API request %s", discovery_type, request);

      curl_easy_setopt(curl_handle, CURLOPT_URL, request);
      curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
      curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *) &chunk);

      CURLcode res = curl_easy_perform(curl_handle);

      if (res != CURLE_OK)
        {
          err_msg("[discovery.%s] curl_easy_perform() failed: %s", discovery_type, curl_easy_strerror(res));
          curl_easy_cleanup(curl_handle);
          if (chunk.memory)
            free(chunk.memory);
          return (Ganglia_udp_send_channels) discovered_udp_send_channels;
        }

      long http_code = 0;
      curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
      curl_easy_cleanup(curl_handle);
      debug_msg("[discovery.%s] HTTP response code %ld, %lu bytes retrieved", discovery_type, http_code, (long) chunk.size);
    }

  char *response = chunk.memory;

  if (chunk.memory)
    free(chunk.memory);

  /* debug_msg("[discovery.%s] API response %s", discovery_type, response); */

  /* Parse XML response */
  apr_xml_doc *doc;
  apr_xml_parser *parser = apr_xml_parser_create(context);

  if (apr_xml_parser_feed(parser, response, strlen(response)) != APR_SUCCESS)
    {
      err_msg("[discovery.%s] could not start the XML parser", discovery_type);
      return (Ganglia_udp_send_channels) discovered_udp_send_channels;
    }
  if (apr_xml_parser_done(parser, &doc) != APR_SUCCESS)
    {
      err_msg("[discovery.%s] could not parse XML", discovery_type);
      return (Ganglia_udp_send_channels) discovered_udp_send_channels;
    }

  apr_xml_elem *root = doc->root;
  char *instance_id = NULL;
  char *ec2host = NULL;

  apr_hash_t *instances = apr_hash_make(context);

  for (const apr_xml_elem *elem = root->first_child; elem; elem = elem->next)
    {
      for (const apr_xml_elem *elem2 = elem->first_child; elem2; elem2 = elem2->next)
        {
          for (const apr_xml_elem *elem3 = elem2->first_child; elem3; elem3 = elem3->next)
            {
              for (const apr_xml_elem *elem4 = elem3->first_child; elem4; elem4 = elem4->next)
                {
                  for (const apr_xml_elem *elem5 = elem4->first_child; elem5; elem5 = elem5->next)
                    {
                      if (apr_strnatcmp(elem5->name, "instanceId") == 0 && elem5->first_cdata.first != NULL)
                        {
                          instance_id = apr_pstrdup(context, elem5->first_cdata.first->text);
                        }
                      if (apr_strnatcmp(elem5->name, ec2type(discovery_host_type)) == 0 && elem5->first_cdata.first != NULL)
                        {
                          ec2host = apr_pstrdup(context, elem5->first_cdata.first->text);
                          apr_hash_set(instances, ec2host, APR_HASH_KEY_STRING, instance_id);
                        }
                      if (apr_strnatcmp(elem5->name, "groupSet") == 0)
                        {
                          for (const apr_xml_elem *elem6 = elem5->first_child; elem6; elem6 = elem6->next)
                            {
                              for (const apr_xml_elem *elem7 = elem6->first_child; elem7; elem7 = elem7->next)
                                {
                                  if (apr_strnatcmp(elem7->name, "groupName") == 0 && elem7->first_cdata.first != NULL)
                                    debug_msg("[discovery.%s] %s security group %s", discovery_type, instance_id, elem7->first_cdata.first->text);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

  if (apr_hash_count(instances) == 0)
    {
      err_msg
        ("[discovery.%s] Did not discover any matching nodes. Leaving current send channels unchanged. Retry in %d seconds...", discovery_type, discover_every);
      return (Ganglia_udp_send_channels) discovered_udp_send_channels;
    }
  else
    {
      debug_msg("[discovery.%s] Found %d matching instances", discovery_type, apr_hash_count(instances));
    }

  apr_hash_t *open_sockets = apr_hash_make(context);
  apr_array_header_t *tmp_send_channels = apr_array_make(context, 10, sizeof(apr_socket_t *));      /* init array size of 10 is not max */

  if (discovered_udp_send_channels != NULL)
    {
      apr_array_header_t *chnls = (apr_array_header_t *) discovered_udp_send_channels;

      for (int i = 0; i < chnls->nelts; i++)
        {
          apr_sockaddr_t *remotesa = NULL;
          char remoteip[256];
          apr_socket_t *s = ((apr_socket_t **) (chnls->elts))[i];

          apr_socket_addr_get(&remotesa, APR_REMOTE, s);
          apr_sockaddr_ip_buffer_get(remoteip, 256, remotesa);
          if (!apr_hash_get(instances, remoteip, APR_HASH_KEY_STRING))
            {
              debug_msg("[discovery.%s] removing udp send channel %s %s:%d", discovery_type, discovery_host_type, remoteip, port);
              apr_socket_close(s);
            }
          else
            {
              /* keep track of opened sockets */
              *(apr_socket_t **) apr_array_push(tmp_send_channels) = s;
              apr_hash_set(open_sockets, apr_pstrdup(context, remoteip), APR_HASH_KEY_STRING, "OK");
            }
        }
    }

  cfg_t *globals = (cfg_t *) cfg_getsec((cfg_t *) cfg, "globals");
  char *override_hostname = cfg_getstr(globals, "override_hostname");
  char *override_ip = cfg_getstr(globals, "override_ip");
  char *cloud_conf = "# This file is generated by cloud-enabled version of Ganglia gmond\n\n";

  cloud_conf = apr_pstrcat(context, cloud_conf, "globals {\n", NULL);
  if (override_hostname)
    cloud_conf = apr_pstrcat(context, cloud_conf, "  override_hostname = ", override_hostname, "\n", NULL);
  if (override_ip)
    cloud_conf = apr_pstrcat(context, cloud_conf, "  override_ip = ", override_ip, "\n", NULL);
  cloud_conf = apr_pstrcat(context, cloud_conf, "}\n\n", NULL);

  for (apr_hash_index_t *hi = apr_hash_first(context, instances); hi; hi = apr_hash_next(hi))
    {
      const void *k;
      void *v;
      char *hkey;
      char *val;

      apr_hash_this(hi, &k, NULL, &v);
      hkey = (char *) k;
      val = v;

      if (!apr_hash_get(open_sockets, hkey, APR_HASH_KEY_STRING))
        {
          debug_msg("[discovery.%s] adding %s, udp send channel %s %s:%d", discovery_type, val, discovery_host_type, hkey, port);
          apr_socket_t *socket = NULL;

          apr_pool_t *pool = NULL;

          apr_pool_create(&pool, context);

          socket = create_udp_client(pool, hkey, port, NULL, NULL, 0);
          if (!socket)
            {
              err_msg("Unable to create UDP client for %s:%d. Exiting.\n", hkey, port);
              exit(1);
            }

          *(apr_socket_t **) apr_array_push(tmp_send_channels) = socket;
        }

      cloud_conf =
        apr_pstrcat(context, cloud_conf, "udp_send_channel {\n  bind_hostname = yes\n  host = ", hkey, "\n  port = ", apr_itoa(context, port), "\n}\n\n", NULL);
    }

  /* Write out discovered UDP send channels to file for gmetric */
  apr_file_t *file;
  const char *fname = GMOND_CLOUD_CONF;

  if (apr_file_open(&file, fname, APR_WRITE | APR_CREATE, APR_UREAD | APR_UWRITE | APR_GREAD, context) != APR_SUCCESS)
    {
      err_msg("[discovery.%s] Failed to open cloud configuration file %s", discovery_type, GMOND_CLOUD_CONF);
    }
  else
    {
      apr_size_t bytes = strlen(cloud_conf);

      if (apr_file_write(file, cloud_conf, &bytes) != APR_SUCCESS)
        {
          err_msg("[discovery.%s] Failed to write cloud configuration to file %s", discovery_type, GMOND_CLOUD_CONF);
        }
      apr_file_trunc(file, bytes);
      apr_file_close(file);
    }

  discovered_udp_send_channels = apr_array_copy(context, tmp_send_channels);

  return (Ganglia_udp_send_channels) discovered_udp_send_channels;
}
