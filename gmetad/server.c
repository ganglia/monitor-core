#include <zlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdarg.h>

#include "gmetad.h"

#include "lib/llist.h"
#include "lib/gzio.h"
#include "libunp/unp.h"

extern int server_socket;
extern pthread_mutex_t  server_socket_mutex;
extern int interactive_socket;
extern pthread_mutex_t  server_interactive_mutex;

extern Source_t root;

extern gmetad_config_t gmetad_config;

extern char* getfield(char *buf, short int index);
extern struct type_tag* in_type_list (char *, unsigned int);

static int
metric_summary(datum_t *key, datum_t *val, void *arg)
{
   client_t *client = (client_t*) arg;
   char *name = (char*) key->data;
   char *type;
   char sum[256];
   Metric_t *metric = (Metric_t*) val->data;
   struct type_tag *tt;

   type = getfield(metric->strings, metric->type);

   tt = in_type_list(type, strlen(type));
   if (!tt) return 0;

   switch (tt->type)
      {
         case INT:
            sprintf(sum, "%d", metric->val.int32);
            break;
         case UINT:
            sprintf(sum, "%u", metric->val.uint32);
            break;
         case FLOAT:
            sprintf(sum, "%.*f", (int) metric->precision, metric->val.d);
            break;
         default:
            break;
      }

   return ganglia_gzprintf(client->io, "<METRICS NAME=\"%s\" SUM=\"%s\" NUM=\"%u\" "
      "TYPE=\"%s\" UNITS=\"%s\" SLOPE=\"%s\" SOURCE=\"%s\"/>\n",
      name, sum, metric->num,
      getfield(metric->strings, metric->type),
      getfield(metric->strings, metric->units),
      getfield(metric->strings, metric->slope),
      getfield(metric->strings, metric->source)) <= 0? 1: 0;
}


static int
source_summary(Source_t *source, client_t *client)
{
   int rc;

   rc=ganglia_gzprintf(client->io, "<HOSTS UP=\"%u\" DOWN=\"%u\" SOURCE=\"gmetad\"/>\n",
      source->hosts_up, source->hosts_down);
   if (rc<=0) return 1;

   return hash_foreach(source->metric_summary, metric_summary, (void*) client);
}

int
metric_report_start(Generic_t *self, datum_t *key, client_t *client, void *arg)
{
   char *name = (char*) key->data;
   Metric_t *metric = (Metric_t*) self;

   return ganglia_gzprintf(client->io, "<METRIC NAME=\"%s\" VAL=\"%s\" TYPE=\"%s\" "
      "UNITS=\"%s\" TN=\"%u\" TMAX=\"%u\" DMAX=\"%u\" SLOPE=\"%s\" "
      "SOURCE=\"%s\"/>\n",
      name, getfield(metric->strings, metric->valstr),
      getfield(metric->strings, metric->type),
      getfield(metric->strings, metric->units), metric->tn,
      metric->tmax, metric->dmax, getfield(metric->strings, metric->slope),
      getfield(metric->strings, metric->source)) <= 0? 1: 0;
}

int
metric_report_end(Generic_t *self, client_t *client, void *arg)
{
   return 0;
}

int
host_report_start(Generic_t *self, datum_t *key, client_t *client, void *arg)
{
   char *name = (char*) key->data;
   Host_t *host = (Host_t*) self;

   /* Note the hash key is the host's IP address. */
   return ganglia_gzprintf(client->io, "<HOST NAME=\"%s\" IP=\"%s\" REPORTED=\"%u\" "
      "TN=\"%u\" TMAX=\"%u\" DMAX=\"%u\" LOCATION=\"%s\" GMOND_STARTED=\"%u\">\n",
      name, getfield(host->strings, host->ip), host->reported, host->tn,
      host->tmax, host->dmax, getfield(host->strings, host->location),
      host->started) <= 0? 1: 0;
}


int
host_report_end(Generic_t *self, client_t *client, void *arg)
{
   return ganglia_gzprintf(client->io, "</HOST>\n") <= 0? 1: 0;
}


int
source_report_start(Generic_t *self, datum_t *key, client_t *client, void *arg)
{
   char *name = (char*) key->data;
   Source_t *source = (Source_t*) self;

   if (self->id == CLUSTER_NODE)
      {
            return ganglia_gzprintf(client->io, "<CLUSTER NAME=\"%s\" LOCALTIME=\"%u\" OWNER=\"%s\" "
               "LATLONG=\"%s\" URL=\"%s\">\n",
               name, source->localtime, getfield(source->strings, source->owner),
               getfield(source->strings, source->latlong),
               getfield(source->strings, source->url)) <= 0? 1: 0;
      }

   return ganglia_gzprintf(client->io, "<GRID NAME=\"%s\" AUTHORITY=\"%s\" "
         "LOCALTIME=\"%u\">\n",
          name, getfield(source->strings, source->authority_ptr), source->localtime) <= 0? 1:0;
}


int
source_report_end(Generic_t *self,  client_t *client, void *arg)
{

   if (self->id == CLUSTER_NODE)
      return ganglia_gzprintf(client->io, "</CLUSTER>\n") <= 0? 1: 0;

   return ganglia_gzprintf(client->io, "</GRID>\n") <= 0? 1: 0;
}


/* These are a bit different since they always need to go out. */
int
root_report_start(client_t *client)
{
   int rc;

   rc = ganglia_gzprintf(client->io, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"no\"?>\n");
   if( rc <= 0) return 1;

   rc = ganglia_gzprintf(client->io, "<!DOCTYPE GANGLIA_XML SYSTEM \"http://ganglia.sourceforge.net/dtd/v2.dtd\">\n"); 
   if( rc <= 0) return 1;

   rc = ganglia_gzprintf(client->io, "<GANGLIA_XML VERSION=\"%s\" SOURCE=\"gmetad\">\n", 
      VERSION);
   if (rc <= 0) return 1;

   return ganglia_gzprintf(client->io, "<GRID NAME=\"%s\" AUTHORITY=\"%s\" LOCALTIME=\"%u\">\n",
       gmetad_config.gridname, getfield(root.strings, root.authority_ptr), time(0)) <= 0? 1: 0;
}


int
root_report_end(client_t *client)
{
    return ganglia_gzprintf(client->io, "</GRID>\n</GANGLIA_XML>\n") <= 0? 1: 0;
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
 * bounded, this function guarantees O(1) search time. The recursive structure 
 * does not require any memory allocations. 
 */
static int
process_path (client_t *client, char *path, datum_t *myroot, datum_t *key)
{
   char *p, *q, *pathend;
   char element[256];
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
      
         len = q-p;
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
         else
            {
               rc = process_path(client, 0, myroot, NULL);
            }
      }
   if (rc) return 1;

   /* End tag */
   if (node->report_end)
      {
         rc = node->report_end(node, client, NULL);
      }

   return rc;
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
   if (!pathlen) return 1;

   if (*path != '/') return 1;

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
   return (maxlen - nleft);
}

#define REQUESTLEN 2048

void *
server_thread (void *arg)
{
  int rval;
   int interactive = (int) arg;
   int len;
   client_t client;
   char request[REQUESTLEN];
   llist_entry *le;
   datum_t rootdatum;
   char mode[32];
   char clienthost[NI_MAXHOST];
   char clientservice[NI_MAXSERV];

   for (;;)
      {
         len = CLIENT_ADDR_SIZE;
         client.valid = 0;

         if (interactive)
            {
               pthread_mutex_lock(&server_interactive_mutex);
               SYS_CALL( client.fd, accept(interactive_socket, (struct sockaddr *) &(client.addr), &len));
               pthread_mutex_unlock(&server_interactive_mutex);
            }
         else
            {
               pthread_mutex_lock  ( &server_socket_mutex );
               SYS_CALL( client.fd, accept(server_socket, (struct sockaddr *) &(client.addr), &len));
               pthread_mutex_unlock( &server_socket_mutex );
            }
         if ( client.fd < 0 )
            {
               err_ret("server_thread() error");
               debug_msg("server_thread() %d clientfd = %d errno=%d\n",
                        pthread_self(), client.fd, errno);
               continue;
            }

	 rval = getnameinfo((struct sockaddr *)&(client.addr), len,
		            clienthost, sizeof(clienthost),
		            clientservice, sizeof(clientservice),
		            NI_NUMERICHOST);
	 if(rval != 0)
	   {
	     /* We got an error. Do the paranoid thing and close the client socket
	      * since we can't determine if we trust the client or not. */
	     close(client.fd);
	     continue;
	   }

         if ( !strcmp(clienthost, "127.0.0.1")
               || gmetad_config.all_trusted
               || (llist_search(&(gmetad_config.trusted_hosts), (void *)clienthost, strcmp, &le) == 0) )
            {
               client.valid = 1;
            }

         if(! client.valid )
            {
               debug_msg("server_thread() %s tried to connect and is not a trusted host",
                  clienthost);
               close( client.fd );
               continue;
            }
         
         client.filter=0;

         if (interactive)
            {
               len = readline(client.fd, request, REQUESTLEN);
               if (len<0)
                  {
                     err_msg("server_thread() could not read request from %s", clienthost);
                     close(client.fd);
                     continue;
                  }
               debug_msg("server_thread() received request \"%s\" from %s", request, clienthost);

               if (process_request(&client, request))
                  {
                     err_msg("Got a malformed path request from %s", clienthost);
                     /* Send them the entire tree to discourage attacks. */
                     strcpy(request, "/");
                  }
            }
         else
            strcpy(request, "/");

	 sprintf(mode, "wb%d", gmetad_config.xml_compression_level);
         client.io = ganglia_gzdopen( client.fd, mode );
         if(!client.io)
           {
             err_msg("unable to create client stream");
             close(client.fd);
             continue;
           }

         if(root_report_start(&client))
            {
               err_msg("server_thread() %d unable to write root preamble (DTD, etc)",
                         pthread_self() );
               ganglia_gzclose(client.io);
               continue;
            }

         /* Start search at the root node. */
         rootdatum.data = &root;
         rootdatum.size = sizeof(root);

         if (process_path(&client, request, &rootdatum, NULL))
            {
               err_msg("server_thread() %d unable to write XML tree info", pthread_self() );
               ganglia_gzclose(client.io);
               continue;
            }

         if(root_report_end(&client))
            {
               err_msg("server_thread() %d unable to write root epilog", pthread_self() );
            }

         ganglia_gzclose( client.io);
      }
}
