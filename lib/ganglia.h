/**
 * @file ganglia.h Main Ganglia Cluster Toolkit Library Header File
 */

#ifndef GANGLIA_H
#define GANGLIA_H 1

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

#define GANGLIA_DEFAULT_MCAST_CHANNEL "239.2.11.71"
#define GANGLIA_DEFAULT_MCAST_PORT 8649
#define GANGLIA_DEFAULT_XML_PORT 8649

#ifndef MAX_MCAST_MSG
#define MAX_MCAST_MSG 1024
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <ganglia/error.h>
#include <ganglia/hash.h>
#include <ganglia/llist.h>
#include <ganglia/net.h>
#include <ganglia/xmlparse.h>

#ifndef SYS_CALL
#define SYS_CALL(RC,SYSCALL) \
   do {                      \
       RC = SYSCALL;         \
   } while (RC < 0 && errno == EINTR);
#endif


/**
 * Ganglia Metric Datatype.
 */
typedef struct
   {
      datum_t * val;   /**< Value of the metric. */
      datum_t * type;  /**< The Metric Datatype (string, uint16, etc) */
      datum_t * units; /**< Units of Measure */
      datum_t * source;/**< Where the metric was reported from */
   }
metric_info_t;

/**
 * Ganglia Host Datatype.
 */
typedef struct
   {
      char name[MAXHOSTNAMELEN]; /**< The name of the host */
      int last_reported; /**< When the host last reported on the ganglia multicast */
      hash_t *metrics; /**< All builtin metrics it has reported */
      hash_t *gmetric_metrics; /**< All custom metrics it has reported */
   }
host_t;

/**
 * Ganglia Cluster Datatype.
 */
typedef struct
   {
      char name[256]; /**< Name supplied by ganglia_cluster_init() . */

      llist_entry *source_list; /**<  List of all XML data sources for this cluster
                                      added with ganglia_add_datasource(). */ 
      unsigned long num_sources; /**< Number of clusters in the source_list. */

      hash_t *host_cache;  /**< All cluster hosts key=ip val=hostname for resolving names */

      unsigned int num_nodes; /**< Total Number of Cluster Nodes */
      hash_t *nodes; /**< Data for the Cluster Nodes that are Up */

      unsigned int num_dead_nodes; /**< Number of Dead Nodes Over All Clusters */
      hash_t *dead_nodes; /**< Data for the Cluster Nodes that are DOWN */ 

      long last_updated; /**< Last time we synchronized hash with XML */

      llist_entry *llist; /**< This linked-list is necessary for the sort, filter, reduce functions */
   }
cluster_t;

/**
 * Ganglia Cluster Datasource Type
 */
typedef struct
{
   cluster_t *cluster;  /**< Pointer to parent cluster */
   char name[256];      /**< The name of the parent cluster */
   char group[256];     /**< The group this datasource is assigned to */
   long localtime;      /**< The localtime at the datasource */
   unsigned long num_nodes; /**< The number of nodes (hosts) reporting at the datasource */
   unsigned long num_dead_nodes; /**< The number of dead nodes reporting */
   char gmond_ip[16];   /**< The ip address of this datasource (gmond) */
   unsigned short gmond_port; /**< The port number of this datasource */
   int  gmond_compressed; /**< Is the data compressed by zlib? */
   char gmond_version[32]; /**< What version of gmond is the datasource running? */
}
cluster_info_t;

/* In order for C++ programs to be able to use my C library */
#ifdef __cplusplus
#define BEGIN_C_DECLS extern "C" {
#define END_C_DECLS }
#else
#define BEGIN_C_DECLS
#define END_C_DECLS
#endif

BEGIN_C_DECLS
int ganglia_cluster_init(cluster_t *cluster, char *name, unsigned long num_nodes_in_cluster);
int ganglia_add_datasource(cluster_t *cluster, char *group_name, char *ip, unsigned short port, int opts);
int ganglia_cluster_print(cluster_t *cluster);
END_C_DECLS

#endif
