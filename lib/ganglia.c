/**
 * @file gexec_funcs.c Functions to support gexec, gstat et al
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <expat.h>

#include "ganglia_gexec.h"
#include "ganglia_priv.h"
#include "net.h"
#include "llist.h"
#include "hash.h"

int gexec_errno = 0;

static void
start (void *data, const char *el, const char **attr)
{
   int n;
   gexec_cluster_t *cluster = (gexec_cluster_t *)data;
   char *p;

   if (! strcmp("CLUSTER", el))
      {
         strncpy( cluster->name, attr[1], 256 );
         for(n=0; attr[n] && strcmp(attr[n], "LOCALTIME"); n+=2){}
         cluster->localtime = atol(attr[n+1]);
      }
   else if (! strcmp("HOST",el))
      {
         cluster->host = (gexec_host_t *)calloc(1, sizeof(gexec_host_t) );
         if ( cluster->host == NULL )
            {
               /* We'll catch this error later */
               cluster->malloc_error = 1;
               gexec_errno = 1;
               return;
            } 

         if(! strcmp( attr[1], attr[3]))
            {
               /* The IP address did not resolve at all */
               cluster->host->name_resolved = 0;
               strcpy(cluster->host->name,   attr[1]);
               strcpy(cluster->host->domain, "unresolved");
            }
         else
            {
               cluster->host->name_resolved = 1;
               p = strchr( attr[1], '.' );
               if( p )
                  {
                     /* The IP DID resolve AND we have a domainname */
                     n = p - attr[1];
                     strncpy(cluster->host->name, attr[1], n);
                     cluster->host->name[n] = '\0';
                     p++;
                     strncpy(cluster->host->domain, p, GEXEC_HOST_STRING_LEN);
                  }
               else
                  {
                     /* The IP DID resolve BUT we DON'T have a domainname */
                     strncpy(cluster->host->name, attr[1], GEXEC_HOST_STRING_LEN);
                     strcpy(cluster->host->domain, "unspecified");
                  }
            }

         strcpy(cluster->host->ip, attr[3]);
         cluster->host->last_reported = atol(attr[7]);

         if( abs(cluster->localtime - cluster->host->last_reported) < GEXEC_TIMEOUT )
            {
               cluster->host_up = 1;
            }
         else
            {
               cluster->host_up = 0;
            }
      }
   else if (! strcmp("METRIC", el))
      {
         if( cluster->malloc_error )
            {
               return;
            } 
         if(! strcmp( attr[1], "cpu_num" ))
            {
               cluster->host->cpu_num = atoi( attr[3] );
            }
         else if(! strcmp( attr[1], "load_one" ))
            {
               cluster->host->load_one = atof( attr[3] );
            }
         else if(! strcmp( attr[1], "load_five" ))
            {
               cluster->host->load_five = atof( attr[3] );
            }
         else if(! strcmp( attr[1], "load_fifteen" ))
            {
               cluster->host->load_fifteen = atof( attr[3] );
            }
         else if(! strcmp( attr[1], "proc_run" ))
            {
               cluster->host->proc_run = atoi( attr[3] );
            }
         else if(! strcmp( attr[1], "proc_total" ))
            {
               cluster->host->proc_total = atoi( attr[3] );
            }
         else if(! strcmp( attr[1], "cpu_user" ))
            {
               cluster->host->cpu_user = atof( attr[3] );
            }
         else if(! strcmp( attr[1], "cpu_nice" ))
            {
               cluster->host->cpu_nice = atof( attr[3] );
            }
         else if(! strcmp( attr[1], "cpu_system"))
            {
               cluster->host->cpu_system = atof( attr[3] );
            }
         else if(! strcmp( attr[1], "cpu_idle"))
            {
               cluster->host->cpu_idle = atof( attr[3] );
            }
         else if(! strcmp( attr[1], "cpu_wio"))
            {
               cluster->host->cpu_wio = atof( attr[3] );
            }
         else if(! strcmp( attr[1], "gexec" ))
            {
               if(! strcmp( attr[3], "ON" ))
                  cluster->host->gexec_on = 1;
            }
      }
}

static void
end (void *data, const char *el)
{
   gexec_cluster_t *cluster = (gexec_cluster_t *)data;
   llist_entry *e, *e2;

   if ( strcmp( "HOST", el ) )
      return;

   e = malloc( sizeof( llist_entry ) );
   if ( e == NULL )
      {
         if( cluster->host )
           free(cluster->host);
         gexec_errno = 1; 
         return;
      }

   e2 = malloc( sizeof( llist_entry ) );
   if ( e == NULL )
      {
         if( cluster->host )
           free(cluster->host);
         gexec_errno = 1;
         free(e);
         return;
      }

   e->val  = e2->val =  cluster->host;

   if(cluster->host_up)
      {
         cluster->num_hosts++;
         llist_add((llist_entry **) &(cluster->hosts), e);

         if(cluster->host->gexec_on)
            {
               cluster->num_gexec_hosts++;
               llist_add((llist_entry **) &(cluster->gexec_hosts), e2);
            }
         else
            {
               free(e2);
            }
      }
   else
      {
         cluster->num_dead_hosts++;
         llist_add((llist_entry **) &(cluster->dead_hosts), e);
      }
}

int
gexec_cluster_free ( gexec_cluster_t *cluster )
{
   llist_entry *ei, *next;

   if(cluster == NULL)
      {
         gexec_errno = 2;
         return gexec_errno;
      }

   for (ei = cluster->hosts; ei != NULL; ei = next )
      {
         next = ei->next;
         if(ei->val)
            free(ei->val);
         free(ei);
      }
   for (ei = cluster->gexec_hosts; ei != NULL; ei = next )
      {
         next = ei->next;
         /* Values were freed above */
         free(ei);
      }
   for (ei = cluster->dead_hosts; ei != NULL; ei = next)
      {
         next = ei->next;
         if(ei->val)
            free(ei->val);
         free(ei);
      }

   gexec_errno = 0;
   return gexec_errno;
}

static int
load_sort( llist_entry *a, llist_entry *b )
{
   double ai, bi;

   ai = ((gexec_host_t*)(a->val))->load_one - ((gexec_host_t *)(a->val))->cpu_num;
   bi = ((gexec_host_t*)(b->val))->load_one - ((gexec_host_t *)(b->val))->cpu_num;   

   if (ai>bi) return 1;
   return 0;
}

static int
cluster_dead_hosts_sort( llist_entry *a, llist_entry *b )
{
   double ai, bi;

   ai = ((gexec_host_t*)(a->val))->last_reported;
   bi = ((gexec_host_t*)(b->val))->last_reported;

   if (ai<bi) return 1;
   return 0;
}

int
gexec_cluster (gexec_cluster_t *cluster, char *ip, unsigned short port)
{
   XML_Parser xml_parser;
   g_tcp_socket *gmond_socket;
#if 0
   int gmond_fd;
#endif

   if ( cluster == NULL )
      {
         gexec_errno = 2;
         return gexec_errno;
      }

   gmond_socket = g_tcp_socket_connect( ip, port ); 
   if (!gmond_socket)
      {
         gexec_errno = 3;
         return gexec_errno;
      }

   debug_msg("Connected to socket %s:%d", ip, port);
 
   xml_parser = XML_ParserCreate (NULL);
   if (! xml_parser)
      {
         gexec_errno = 4;
         return gexec_errno;
      }

   debug_msg("Created the XML Parser");

   memset( cluster, 0, sizeof(gexec_cluster_t));
   cluster->localtime = time(NULL);

   XML_SetElementHandler (xml_parser, start, end);
   XML_SetUserData       (xml_parser, cluster); 

   for (;;) 
      {
         int bytes_read;
         void *buff = XML_GetBuffer(xml_parser, BUFSIZ);
         if (buff == NULL) 
            {
               gexec_errno = 5;
               goto error;
            }

         debug_msg("Got the XML Buffer");

         SYS_CALL( bytes_read, read(gmond_socket->sockfd, buff, BUFSIZ));
         if (bytes_read < 0) 
            {
               gexec_errno = 6;
               goto error;
            }

        debug_msg("Read %d bytes of data", bytes_read);

        if (! XML_ParseBuffer(xml_parser, bytes_read, bytes_read == 0)) 
           {
              gexec_errno = 7;
              err_msg ("gexec_cluster() XML_ParseBuffer() error at line %d:\n%s\n",
              XML_GetCurrentLineNumber (xml_parser),
              XML_ErrorString (XML_GetErrorCode (xml_parser)));
              goto error;
           }

        if (bytes_read == 0)
           break;
      }  

   /* sort the list with least loaded on top */
   llist_sort( cluster->hosts, load_sort);
   /* sort the list with least loaded on top */
   llist_sort( cluster->gexec_hosts, load_sort);
   /* sort the list from latest crash to oldest */
   llist_sort( cluster->dead_hosts, cluster_dead_hosts_sort);


   gexec_errno = 0;

  error:
   XML_ParserFree(xml_parser);
   g_tcp_socket_delete(gmond_socket);
   return gexec_errno;
}
