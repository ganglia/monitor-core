#ifndef GANGLIA_PRIV_H
#define GNAGLIA_PRIV_H 1

#include "hash.h"
#include "llist.h"
#include "libmetrics.h"  /* for MAX_G_STRING_SIZE */


#include <errno.h>
#ifndef SYS_CALL
#define SYS_CALL(RC,SYSCALL) \
   do {                      \
       RC = SYSCALL;         \
   } while (RC < 0 && errno == EINTR);
#endif

typedef struct
   {
      long start_time;
      struct timeval timestamp;
      unsigned int tmax;
      unsigned int dmax;
      char   *hostname;
      char location[MAX_G_STRING_SIZE];  /* Node's location in the cluster "x,y,z" */
      hash_t *hashp;
      hash_t *user_hashp;
   }
node_data_t;   

#define MAX_VAL_LEN 256
#define MAX_TYPE_LEN  16
#define MAX_UNITS_LEN 16 
#define MAX_SLOPE_LEN 16
/* The size of an ethernet frame, minus some for the name/units. */
#define FRAMESIZE  1400

/* Value payload gets trimmed later. */
typedef struct
   {
      char type[MAX_TYPE_LEN];
      char units[MAX_UNITS_LEN];
      struct timeval timestamp;
      unsigned int tmax;
      unsigned int dmax;
      char slope[MAX_SLOPE_LEN];
      unsigned int valsize;
      char val[FRAMESIZE];
   }
metric_data_t;

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
ghost_t;

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


#endif
