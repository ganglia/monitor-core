#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <string.h>
#include "dtd.h"
#include "gmetad.h"
#include "my_inet_ntop.h"
#include "server_priv.h"

extern g_tcp_socket *server_socket;
extern pthread_mutex_t  server_socket_mutex;
extern g_tcp_socket *interactive_socket;
extern pthread_mutex_t  server_interactive_mutex;

extern Source_t root;

extern gmetad_config_t gmetad_config;

extern char* getfield(char *buf, short int index);
extern struct type_tag* in_type_list (char *, unsigned int);


static inline int CHECK_FMT(2, 3)
xml_print( client_t *client, const char *fmt, ... )
{
   int rval, len;
   va_list ap;
   char buf[4096];

   va_start (ap, fmt);

   if(! client->valid ) 
      {
         va_end(ap);
         return 1;
      }

   vsnprintf (buf, sizeof (buf), fmt, ap);  

   len = strlen(buf);

   SYS_CALL( rval, write( client->fd, buf, len)); 
   if ( rval < 0 && rval != len ) 
      {
         va_end(ap);
         client->valid = 0;
         return 1;
      }

   va_end(ap);
   return 0;
}


static int
metric_summary(datum_t *key, datum_t *val, void *arg)
{
   client_t *client = (client_t*) arg;
   char *name = (char*) key->data;
   char *type;
   char sum[256];
   Metric_t *metric = (Metric_t*) val->data;
   struct type_tag *tt;
   int rc,i;

   type = getfield(metric->strings, metric->type);

   tt = in_type_list(type, strlen(type));
   if (!tt) return 0;

   /* We sum everything in double to properly combine integer sources
      (3.0) with float sources (3.1).  This also avoids wraparound
      errors: for example memory KB exceeding 4TB. */
   switch (tt->type)
      {
         case INT:
         case UINT:
            sprintf(sum, "%.f", metric->val.d);
            break;
         case FLOAT:
            sprintf(sum, "%.*f", (int) metric->precision, metric->val.d);
            break;
         default:
            break;
      }

   rc = xml_print(client, "<METRICS NAME=\"%s\" SUM=\"%s\" NUM=\"%u\" "
      "TYPE=\"%s\" UNITS=\"%s\" SLOPE=\"%s\" SOURCE=\"%s\">\n",
      name, sum, metric->num,
      "double",         /* we always report double sums */
      getfield(metric->strings, metric->units),
      getfield(metric->strings, metric->slope),
      getfield(metric->strings, metric->source));

   rc = xml_print(client, "<EXTRA_DATA>\n");

   for (i=0; !rc && i<metric->ednameslen; i++) 
     {
       rc=xml_print(client, "<EXTRA_ELEMENT NAME=\"%s\" VAL=\"%s\"/>\n",
                    getfield(metric->strings, metric->ednames[i]),
                    getfield(metric->strings, metric->edvalues[i]));
     }

   rc = xml_print(client, "</EXTRA_DATA>\n");
   rc=xml_print(client, "</METRICS>\n");
   return rc;
}


static int
source_summary(Source_t *source, client_t *client)
{
   int rc;

   rc=xml_print(client, "<HOSTS UP=\"%u\" DOWN=\"%u\" SOURCE=\"gmetad\"/>\n",
      source->hosts_up, source->hosts_down);
   if (rc) return 1;

   pthread_mutex_lock(source->sum_finished);
   rc = hash_foreach(source->metric_summary, metric_summary, (void*) client);
   pthread_mutex_unlock(source->sum_finished);
   
   return rc;
}


int
metric_report_start(Generic_t *self, datum_t *key, client_t *client, void *arg)
{
   int rc, i;
   char *name = (char*) key->data;
   Metric_t *metric = (Metric_t*) self;
   long tn = 0;

   tn = client->now.tv_sec - metric->t0.tv_sec;
   if (tn<0) tn = 0;

   if (metric->dmax && metric->dmax < tn)
     return 0;

   rc=xml_print(client, "<METRIC NAME=\"%s\" VAL=\"%s\" TYPE=\"%s\" "
      "UNITS=\"%s\" TN=\"%u\" TMAX=\"%u\" DMAX=\"%u\" SLOPE=\"%s\" "
      "SOURCE=\"%s\">\n",
      name, getfield(metric->strings, metric->valstr),
      getfield(metric->strings, metric->type),
      getfield(metric->strings, metric->units),
      (unsigned int) tn,
      metric->tmax, metric->dmax, getfield(metric->strings, metric->slope),
      getfield(metric->strings, metric->source));

   rc = xml_print(client, "<EXTRA_DATA>\n");

   for (i=0; !rc && i<metric->ednameslen; i++) 
     {
       rc=xml_print(client, "<EXTRA_ELEMENT NAME=\"%s\" VAL=\"%s\"/>\n",
                    getfield(metric->strings, metric->ednames[i]),
                    getfield(metric->strings, metric->edvalues[i]));
     }

   rc = xml_print(client, "</EXTRA_DATA>\n");
   rc=xml_print(client, "</METRIC>\n");

   return rc;
}


int
metric_report_end(Generic_t *self, client_t *client, void *arg)
{
   return 0;
}


int
host_report_start(Generic_t *self, datum_t *key, client_t *client, void *arg)
{
   int rc;
   char *name = (char*) key->data;
   Host_t *host = (Host_t*) self;
   long tn = 0;

   tn = client->now.tv_sec - host->t0.tv_sec;
   if (tn<0) tn = 0;

   /* Note the hash key is the host's IP address. */
   rc = xml_print(client, "<HOST NAME=\"%s\" IP=\"%s\" REPORTED=\"%u\" "
      "TN=\"%u\" TMAX=\"%u\" DMAX=\"%u\" LOCATION=\"%s\" GMOND_STARTED=\"%u\" TAGS=\"%s\">\n",
      name, getfield(host->strings, host->ip), host->reported,
      (unsigned int) tn,
      host->tmax, host->dmax, getfield(host->strings, host->location),
      host->started, getfield(host->strings, host->tags));

   return rc;
}


int
host_report_end(Generic_t *self, client_t *client, void *arg)
{
   return xml_print(client, "</HOST>\n");
}


int
source_report_start(Generic_t *self, datum_t *key, client_t *client, void *arg)
{
   int rc;
   char *name = (char*) key->data;
   Source_t *source = (Source_t*) self;

   if (self->id == CLUSTER_NODE)
      {
            rc=xml_print(client, "<CLUSTER NAME=\"%s\" LOCALTIME=\"%u\" OWNER=\"%s\" "
               "LATLONG=\"%s\" URL=\"%s\">\n",
               name, source->localtime, getfield(source->strings, source->owner),
               getfield(source->strings, source->latlong),
               getfield(source->strings, source->url));
      }
   else
      {
            rc=xml_print(client, "<GRID NAME=\"%s\" AUTHORITY=\"%s\" "
               "LOCALTIME=\"%u\">\n",
               name, getfield(source->strings, source->authority_ptr), source->localtime);
      }

   return rc;
}


int
source_report_end(Generic_t *self,  client_t *client, void *arg)
{

   if (self->id == CLUSTER_NODE)
      return xml_print(client, "</CLUSTER>\n");
   else
      return xml_print(client, "</GRID>\n");
}


/* These are a bit different since they always need to go out. */
int
root_report_start(client_t *client)
{
   int rc;

   if (client->http)
      {
         rc = xml_print(client, "HTTP/1.0 200 OK\r\n"
                                "Server: gmetad/" GANGLIA_VERSION_FULL "\r\n"
                                "Content-Type: application/xml\r\n"
                                "Connection: close\r\n"
                                "\r\n");
         if (rc)
            return rc;
      }

   rc = xml_print(client, DTD);
   if (rc) return 1;

   rc = xml_print(client, "<GANGLIA_XML VERSION=\"%s\" SOURCE=\"gmetad\">\n", 
      VERSION);

   rc = xml_print(client, "<GRID NAME=\"%s\" AUTHORITY=\"%s\" LOCALTIME=\"%u\">\n",
       gmetad_config.gridname, getfield(root.strings, root.authority_ptr),
       (unsigned int) time(0));

   return rc;
}


int
root_report_end(client_t *client)
{
    return xml_print(client, "</GRID>\n</GANGLIA_XML>\n");
}


/* Returns true if the filter type can be applied to this node. */
int
applicable(filter_type_t filter, Generic_t *node)
{
   switch (filter)
      {
      case NO_FILTER:
         return 1;

      case SUMMARY:
         return (node->id==ROOT_NODE
            || node->id==CLUSTER_NODE 
            || node->id==GRID_NODE);
         break;

      default:
         break;
      }
   return 0;
}

int
applyfilter(client_t *client, Generic_t *node)
{
   filter_type_t f = client->filter;

   /* Grids are always summarized. */
   if (node->id == GRID_NODE)
      f = SUMMARY;

   if (!applicable(f, node))
      return 1;   /* A non-fatal error. */

   switch (f)
      {
      case NO_FILTER:
         return 0;

      case SUMMARY:
         return source_summary((Source_t*) node, client);

      default:
         break;
      }
   return 0;
}


/* Processes filter name and sets client->filter. 
 * Assumes path has already been 'cleaned', and that
 * filter points to something like 'filter=[name]'.
 */
int
processfilter(client_t *client, const char *filter)
{
   const char *p=filter;

   client->filter = NO_FILTER;

   p = strchr(p, '=');
   if (!p)
         return 1;

   p++;
   /* This could be done with a gperf hash, etc. */
   if (!strcmp(p, "summary"))
      client->filter = SUMMARY;
   else
      err_msg("Got unknown filter %s", filter);

   return 0;
}


/* sacerdoti: An Object Oriented design in C.
 * We use function pointers to approximate virtual method functions.
 * A recursive-descent design.
 */
static int
tree_report(datum_t *key, datum_t *val, void *arg)
{
   client_t *client = (client_t*) arg;
   Generic_t *node = (Generic_t*) val->data;
   int rc=0;

   if (client->filter && !applicable(client->filter, node)) return 1;

   rc = node->report_start(node, key, client, NULL);
   if (rc) return 1;

   applyfilter(client, node);

   if (node->children)
      {
         /* Allow this to stop early (return code = 1) */
         hash_foreach(node->children, tree_report, arg);
      }

   rc = node->report_end(node, client, NULL);

   return rc;
}

   
/* sacerdoti: This function does a tree walk while respecting the filter path.
 * Will return valid XML even if we have chosen a subtree. Since tree depth is
 * bounded, this function guarantees O(1) search time.
 */
static int
process_path (client_t *client, char *path, datum_t *myroot, datum_t *key)
{
   char *p, *q, *pathend;
   char *element;
   int rc, len;
   datum_t *found;
   datum_t findkey;
   Generic_t *node;

   node = (Generic_t*) myroot->data;

   /* Base case */
   if (!path)
      {
         /* Show the subtree. */
         applyfilter(client, node);

         if (node->children)
            {
               /* Allow this to stop early (return code = 1) */
               hash_foreach(node->children, tree_report, (void*) client);
            }
         return 0;
      }

   /* Start tag */
   if (node->report_start)
      {
         rc = node->report_start(node, key, client, NULL);
         if (rc) return 1;
      }

   /* Subtree body */
   pathend = path + strlen(path);
   p = path+1;

   if (!node->children || p >= pathend)
      rc = process_path(client, 0, myroot, NULL);
   else
      {
         /* Advance to the next element along path. */
         q = strchr(p, '/');
         if (!q) q=pathend;
      
         /* len is limited in size by REQUESTLEN through readline() */
         len = q-p;
         element = malloc(len + 1);
         if ( element == NULL )
             return 1;

         strncpy(element, p, len);
         element[len] = '\0';
      
         /* err_msg("Skipping to %s (%d)", element, len); */
      
         /* look for element in hash table. */
         findkey.data = element;
         findkey.size = len+1;
         found = hash_lookup(&findkey, node->children);
         if (found)
            {
               /* err_msg("Found %s", element); */
               
               rc = process_path(client, q, found, &findkey);
               
               datum_free(found);
            }
         else if (!client->http)
            {
               /* report this element */
               rc = process_path(client, 0, myroot, NULL);
            }
         else if (!strcmp(element, "*"))
            {
               /* wildcard detected -> process every child */
               struct request_context ctxt;
               ctxt.path = q;
               ctxt.client = client;
               hash_foreach(node->children, process_path_adapter, (void*) &ctxt);
            }
         else
            {
               /* element not found */
               rc = 0;
            }
         free(element);
      }
   if (rc) return 1;

   /* End tag */
   if (node->report_end)
      {
         rc = node->report_end(node, client, NULL);
      }

   return rc;
}


static int
process_path_adapter (datum_t *key, datum_t *val, void *arg)
{
   struct request_context *ctxt = (struct request_context*) arg;
   return process_path(ctxt->client, ctxt->path, val, key);
}


/* 'Cleans' the request and calls the appropriate handlers.
 * This function alters the path to prepare it for processpath().
 */
static int
process_request (client_t *client, char *path)
{
   char *p;
   int rc, pathlen;

   pathlen = strlen(path);
   if (!pathlen)
       return 1;

   if (pathlen >= 12 && memcmp(path, "GET ", 4) == 0)
      {
         /* looks like a http request, validate it and update path_p */
         char *http_p = path + pathlen - 9;
         if (memcmp(http_p, " HTTP/1.0", 9) && memcmp(http_p, " HTTP/1.1", 9))
            return 1;
         *http_p = 0;
         pathlen = strlen(path + 4);
         memmove(path, path + 4, pathlen + 1);
         client->http = 1;
      }

   if (*path != '/')
       return 1;

   /* Check for illegal characters in element */
   if (strcspn(path, ">!@#$%`;|\"\\'<") < pathlen)
      return 1;

   p = strchr(path, '?');
   if (p)
      {
         *p = 0;
         debug_msg("Found subtree %s and %s", path, p+1);
         rc = processfilter(client, p+1);
         if (rc) return 1;
      }

   return 0;
}


/* sacerdoti: A version of Steven's thread-safe readn.
 * Only works for reading one line. */
int
readline(int fd, char *buf, int maxlen)
{
   int i, justread, nleft, stop;
   char c;
   char *ptr;

   stop = 0;
   ptr = buf;
   nleft = maxlen;
   while (nleft > 0 && !stop)
      {
         justread = read(fd, ptr, nleft);
         if (justread < 0)
            {
               if (errno == EINTR)
                  justread = 0;  /* and call read() again. */
               else
                  return -1;
            }
         else if (justread==0)
            break;   /* EOF */

         /* Examine buf for end-of-line indications. */
         for (i=0; i<justread; i++)
            {
               c = ptr[i];
               if (c=='\n' || c=='\r')
                  {
                     stop = 1;
                     justread = i;  /* Throw out everything after newline. */
                     break;
                  }
            }
         nleft -= justread;
         ptr += justread;
      }

   *ptr = 0;
   if ((stop == 0) && (nleft == 0)) /* Overflow */
      return -1;

   return (maxlen - nleft);
}

#define REQUESTLEN 2048

void *
server_thread (void *arg)
{
   int interactive = (arg != NULL);
   socklen_t len;
   int request_len;
   client_t client;
   char remote_ip[16];
   char request[REQUESTLEN + 1];
   llist_entry *le;
   datum_t rootdatum;

   for (;;)
      {
         client.valid = 0;
         len = sizeof(client.addr);

         if (interactive)
            {
               pthread_mutex_lock(&server_interactive_mutex);
               SYS_CALL( client.fd, accept(interactive_socket->sockfd, (struct sockaddr *) &(client.addr), &len));
               pthread_mutex_unlock(&server_interactive_mutex);
            }
         else
            {
               pthread_mutex_lock  ( &server_socket_mutex );
               SYS_CALL( client.fd, accept(server_socket->sockfd, (struct sockaddr *) &(client.addr), &len));
               pthread_mutex_unlock( &server_socket_mutex );
            }
         if ( client.fd < 0 )
            {
               err_ret("server_thread() error");
               debug_msg("server_thread() %lx clientfd = %d errno=%d\n",
                        (unsigned long) pthread_self(), client.fd, errno);
               continue;
            }

         my_inet_ntop( AF_INET, (void *)&(client.addr.sin_addr), remote_ip, 16 );

         if ( !strcmp(remote_ip, "127.0.0.1")
               || gmetad_config.all_trusted
               || (llist_search(&(gmetad_config.trusted_hosts), (void *)remote_ip, strcmp, &le) == 0) )
            {
               client.valid = 1;
            }

         if(! client.valid )
            {
               debug_msg("server_thread() %s tried to connect and is not a trusted host",
                  remote_ip);
               close( client.fd );
               continue;
            }

         client.filter=0;
         client.http=0;
         gettimeofday(&client.now, NULL);

         if (interactive)
            {
               request_len = readline(client.fd, request, REQUESTLEN);
               if (request_len < 0)
                  {
                     err_msg("server_thread() could not read request from %s", remote_ip);
                     close(client.fd);
                     continue;
                  }
               debug_msg("server_thread() received request \"%s\" from %s", request, remote_ip);

               if (process_request(&client, request))
                  {
                     err_msg("Got a malformed path request from %s", remote_ip);
                     close(client.fd);
                     continue;
                  }
            }
         else
            strcpy(request, "/");

         if(root_report_start(&client))
            {
               err_msg("server_thread() %lx unable to write root preamble (DTD, etc)",
                       (unsigned long) pthread_self() );
               close(client.fd);
               continue;
            }

         /* Start search at the root node. */
         rootdatum.data = &root;
         rootdatum.size = sizeof(root);

         if (process_path(&client, request, &rootdatum, NULL))
            {
               err_msg("server_thread() %lx unable to write XML tree info",
                       (unsigned long) pthread_self() );
               close(client.fd);
               continue;
            }

         if(root_report_end(&client))
            {
               err_msg("server_thread() %lx unable to write root epilog",
                       (unsigned long) pthread_self() );
            }

         close(client.fd);
      }
}
