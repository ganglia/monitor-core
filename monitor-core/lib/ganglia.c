/**
 * @file ganglia.c The functions in this file perform the core functions of
 * retrieving XML data and saving it to internal data structures
 */
/* $Id$ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>


#include "expat.h"
#include "hash.h"
#include "llist.h"
#include "ganglia.h"
#include "ganglia_priv.h"
#include "error.h"
#include "net.h"

#define TIMEOUT 60
#define NUM_METRICS 30
#define NUM_GMETRIC_METRICS 10


static void
start (void *data, const char *el, const char **attr)
{
  int i;
  cluster_info_t *ci = (cluster_info_t *)data;
  cluster_t *cluster;
  datum_t key, val, *rval;
  ghost_t working_host;
  static char working_host_name[MAXHOSTNAMELEN];
  static char working_host_ip[16];
  static int  working_host_last_reported;
  static int  working_host_dead;
  hash_t *hashp;

  cluster  = ci->cluster;
 
  if (! strcmp("GANGLIA_XML", el) )
     {
        ci->num_nodes=0;
        ci->num_dead_nodes=0;

        for(i=0; attr[i]; i+= 2)
           {
              if (! strcmp("VERSION", attr[i]))
                 {
                    strcpy(ci->gmond_version, attr[i+1]);
                 }
           }
     }
  else if (! strcmp("CLUSTER", el))
     {
        for(i=0; attr[i]; i+=2)
           {
              if(! strcmp("NAME", attr[i]))
                 {
                    strcpy(ci->name, attr[i+1]);
                 }
              else if(! strcmp("LOCALTIME", attr[i]))
                 {
                    ci->localtime = atoi(attr[i+1]);
                 }
           }
     }
  else if (! strcmp("HOST",el))
     {
        ci->num_nodes++;
        cluster->num_nodes++;

        for(i=0; attr[i]; i+=2)
           {
              if(! strcmp("NAME", attr[i]) )
                 {
                    strcpy(working_host_name, attr[i+1]);
                 }
              else if(! strcmp("IP", attr[i]))
                 {
                    strcpy(working_host_ip, attr[i+1]);
                 }
              else if(! strcmp("REPORTED", attr[i]))
                 {
                    working_host_last_reported = atoi(attr[i+1]);
 
                    if( (ci->localtime - working_host_last_reported) > TIMEOUT )
                       working_host_dead = 1;
                    else
                       working_host_dead = 0;
                 }
           }

        key.data = working_host_ip;
        key.size = strlen(working_host_ip)+1;
        val.data = working_host_name;
        val.size = strlen(working_host_name)+1;

        /* add this host to the host_cache .. dead or alive */
        rval = hash_insert(&key, &val, cluster->host_cache);
        if ( rval == NULL )
           {
              err_msg("hash_insert error in start()");
              return;
           }

        /* setup data in the working_host structure */
        strcpy(working_host.name, working_host_name);
        working_host.last_reported = working_host_last_reported;
        working_host.metrics = hash_create(NUM_METRICS);
        if( working_host.metrics == NULL )
           {
              err_msg("hash_create() error in start()"); 
              return;
           }
        working_host.gmetric_metrics = hash_create(NUM_GMETRIC_METRICS);
        if ( working_host.gmetric_metrics == NULL )
           {
              err_msg("hash_create() error in start()");
              return;
           }

        if(! working_host_dead )
           {
              hashp = cluster->nodes;
           }
        else
           {  
              ci->num_dead_nodes++;
              cluster->num_dead_nodes++;
              hashp = cluster->dead_nodes;
           }

        val.data = &working_host;
        val.size = sizeof(working_host);
        rval = hash_insert(&key, &val, hashp);
        if ( rval == NULL )
           {
              err_msg("hash_insert error in start()");
              return;       
           }
     }

  else if (! strcmp("METRIC", el))
     {


     }
}				

static void
end (void *data, const char *el)
{
    /* Nothing to do (yet) */
}				

/**
 * @fn int ganglia_cluster_init (cluster_t *cluster, char *name, unsigned long num_nodes_in_cluster)
 * Initialize a ganglia cluster datatype.
 * Before any ganglia cluster datatype can be used it must be initialized.
 * @param cluster A pointer to the ganglia cluster datatype
 * @param name A character string you wish to identify this cluster
 * @param num_nodes_in_cluster A best-guess of the total number of machines this cluster will contain
 * @return an integer
 * @retval 0 on success
 * @retval -1 on error
 * @see ganglia_add_datasource
 */
int
ganglia_cluster_init(cluster_t *cluster, char *name, unsigned long num_nodes_in_cluster)
{
   if ( cluster == NULL )
      {
         err_msg("ganglia_cluster_init() was passed a NULL pointer");
         return -1;
      }

   strncpy( cluster->name, name, 256);

   cluster->num_sources = 0;

   if (! num_nodes_in_cluster )
      num_nodes_in_cluster = 1024;

   cluster->host_cache = hash_create( num_nodes_in_cluster );
   if ( cluster->host_cache == NULL )
      {
         err_msg("ganglia_cluster_init() host_cache hash_create malloc error");
         goto host_cache_error;
      }
   cluster->nodes      = hash_create( num_nodes_in_cluster );
   if ( cluster->nodes == NULL )
      {
         err_msg("ganglia_cluster_init() nodes hash_create malloc error");
         goto nodes_error;
      }
   cluster->dead_nodes = hash_create( num_nodes_in_cluster * 0.10 );
   if ( cluster->dead_nodes == NULL )
      {
         err_msg("ganglia_cluster_init() dead_nodes hash_create malloc error");
         goto dead_nodes_error;
      }

   cluster->source_list = malloc( sizeof(llist_entry) );
   if ( cluster->source_list == NULL )
      {
         err_msg("ganglia_cluster_init() source_list malloc error");
         goto source_list_error;
      }
   cluster->llist       = malloc( sizeof(llist_entry) );
   if ( cluster->llist == NULL )
      {
         err_msg("ganglia_cluster_init() llist malloc error");
         goto llist_error;
      }

   return 0;

 llist_error:
   free(cluster->source_list);
 source_list_error:
   hash_destroy(cluster->dead_nodes);
 dead_nodes_error:
   hash_destroy(cluster->nodes);
 nodes_error:
   hash_destroy(cluster->host_cache);
 host_cache_error:
   return -1;
}

/**
 * @fn int ganglia_add_datasource ( cluster_t *cluster, char *group_name, char *ip, unsigned short port, int opts )
 * Add a ganglia data source to an initialized cluster type.  
 * The group name allows you to group datasources allowing for redundant sources for the same cluster.  It one
 * of the datasources (gmonds) is down then another datasource (gmond) is used to collect the data.
 * <BR>
 * \code
 * cluster_t cluster;
 * ganglia_cluster_init  (&cluster, "CS Department Clusters", 500);
 * ganglia_add_datasource(&cluster, "Cluster 1 in Room 505", "3.3.3.3", 8649, 0);
 * ganglia_add_datasource(&cluster, "Cluster 1 in Room 505", "3.3.3.4", 8649, 0);
 * ganglia_add_datasource(&cluster, "Cluster 2 in Room 345", "3.3.2.2", 8649, 0);
 * \endcode 
 * @param cluster A pointer to a cluster type cluster_a cluster type cluster_tt
 * @param group_name An arbitrary character string you wish to identify this datasource
 * @param ip The ip address of the datasource (gmond)
 * @param port The port of the datasource (gmond)
 * @param opts Options (not used at this time)
 * @return an integer
 * @retval -1 on error
 * @retval 0 on success
 * @see ganglia_cluster_init
 */
int
ganglia_add_datasource ( cluster_t *cluster, char *group_name, char *ip, unsigned short port, int opts )
{
   llist_entry *li;
   cluster_info_t *ci;

   if (cluster == NULL || ip == NULL || !port)
      {
         err_msg("ganglia_add_datasource() was passed invalid parameters");
         return -1;
      }

   ci = malloc( sizeof(cluster_info_t) );
   if ( ci == NULL )
      {
         err_msg("ganglia_add_datasource() ci malloc error");
         return -1;
      }

   li = malloc( sizeof( llist_entry ) );
   if ( li == NULL )
      {
         free(ci);
         err_msg("ganglia_add_datasource() li malloc error");
         return -1;
      }

   ci->cluster = cluster;
   strcpy( ci->group, group_name );
   strcpy( ci->gmond_ip, ip);
   ci->gmond_port = port;

   li->val = ci;

   llist_add( &(cluster->source_list), li);

   cluster->num_sources++;
   return 0;
}

static int
ganglia_sync_hash_with_xml (cluster_t *cluster )
{
   char buf[BUFSIZ];
   cluster_info_t *ci;
   llist_entry *li;
   XML_Parser xml_parser;
   int gmond_fd;
   FILE *gmond_fp;
   struct timeval tv;

   if ( cluster == NULL )
      {
         err_msg("ganglia_sync_hash_with_xml got a NULL cluster pointer");
         return -1;
      }

   /* Check if we even need to sync the hash */
   gettimeofday(&tv, NULL);
   if ( (tv.tv_sec - cluster->last_updated) < 30 )
      {
         return 0;
      }

   /* Reset */
   cluster->num_nodes = 0;

   /* Collect data from all cluster sources */
   for (li = cluster->source_list; li != NULL; li = li->next)
      {
         if(li->val == NULL)
            continue;

         ci = (cluster_info_t *)li->val;
         gmond_fd = tcp_connect (ci->gmond_ip, ci->gmond_port);
         if (gmond_fd < 0)
            {
               err_msg("ganglia_sync_hash_with_xml unable to connect with %s:%hd",
                        ci->gmond_ip, ci->gmond_port);
               return -1;
            }
 
         gmond_fp = fdopen (gmond_fd, "r");
         if (gmond_fp == NULL)
            {
               err_msg("ganglia_sync_hash_with_xml fdopen() error");
               return -1;
            }
 
         xml_parser = XML_ParserCreate (NULL);
         if (! xml_parser)
            {
               err_msg ( "Couldn't allocate memory for parser");
               return -1;
            }

         XML_SetElementHandler (xml_parser, start, end);
         XML_SetUserData( xml_parser, ci); 

         for (;;)
            {
               int done;
               int len;

               len = fread (buf, 1, BUFSIZ, gmond_fp);
               if (ferror (gmond_fp))
	               {
                     err_msg("ganglia_sync_hash_with_xml fread error");
	                  return -1;
	               }
               done = feof (gmond_fp);

               if (!XML_Parse (xml_parser, buf, len, done))
	               {
	                  err_msg ("ganglia_sync_hash_with_xml() parse error at line %d:\n%s\n",
		                         XML_GetCurrentLineNumber (xml_parser),
		                         XML_ErrorString (XML_GetErrorCode (xml_parser)));
                     return -1;
	               }

               if (done)
	               break;
            }
         close(gmond_fd);
      }
  gettimeofday(&tv, NULL);
  cluster->last_updated = tv.tv_sec;
  return 0;
}

static int
print_foo ( datum_t *key, datum_t *val, void *data )
{
   cluster_t *cluster = (cluster_t *)data;
   datum_t *name;
   printf("%16.16s ", (char *)key->data);
   name = hash_lookup( key, cluster->host_cache );
   printf("%s\n", (char *)name->data); 
   if(name)
      datum_free(name);
   return 0;
}

/**
 * @fn int ganglia_cluster_print (cluster_t *cluster)
 * Print details about the cluster data structure (cluster_t).
 * Used primarily for debugging.
 */
int
ganglia_cluster_print (cluster_t *cluster)
{
   struct timeval tv;

   gettimeofday(&tv, NULL);

   ganglia_sync_hash_with_xml(cluster);

   printf("Summary report for Cluster [%s] at %s\n\n", cluster->name, ctime(&(tv.tv_sec)) );

   printf("There are %ld sources of data\n", cluster->num_sources);
   printf("         Data Freshness: %s", ctime( &(cluster->last_updated) )  );

   printf("Number of Cluster Nodes: %d\n", cluster->num_nodes);
   printf("   Number of Dead Nodes: %d\n\n", cluster->num_dead_nodes);

   printf("Healthy Nodes\n");
   hash_foreach(cluster->nodes, print_foo, (void *)cluster);
   printf("Dead Nodes\n");
   hash_foreach(cluster->dead_nodes, print_foo, (void *)cluster);   
   return 0;
}
