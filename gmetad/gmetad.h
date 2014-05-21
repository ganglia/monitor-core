#ifndef GMETAD_H
#define GMETAD_H 1 

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ganglia_priv.h"
#include "net.h"
#include "hash.h"
#include "llist.h"
#include "conf.h"
#include "become_a_nobody.h"
#include "daemon_init.h"
#include "my_inet_ntop.h"

#ifdef WITH_MEMCACHED
#include <libmemcached-1.0/memcached.h>
#include <libmemcachedutil-1.0/util.h>
#endif /* WITH_MEMCACHED */

/* For metric_hash */
typedef enum {
   INT,
   UINT,
   FLOAT,
   TIMESTAMP,
   STRING
} metric_type_t;

struct type_tag
   {
      const char *name;
      metric_type_t type;
   };

/* The default number of nodes in a cluster */
#define DEFAULT_CLUSTERSIZE 1024

/* The default number of metrics per node */
#define DEFAULT_METRICSIZE 50

/* For xml_hash */
typedef enum
   {
      GANGLIA_XML_TAG,
      GRID_TAG,
      CLUSTER_TAG,
      HOST_TAG,
      NAME_TAG,
      METRIC_TAG,
      TN_TAG,
      TMAX_TAG,
      DMAX_TAG,
      VAL_TAG,
      TYPE_TAG,
      SLOPE_TAG,
      SOURCE_TAG,
      VERSION_TAG,
      REPORTED_TAG,
      LOCALTIME_TAG,
      OWNER_TAG,
      LATLONG_TAG,
      URL_TAG,
      AUTHORITY_TAG,
      IP_TAG,
      LOCATION_TAG,
      STARTED_TAG,
      UNITS_TAG,
      HOSTS_TAG,
      UP_TAG,
      DOWN_TAG,
      METRICS_TAG,
      SUM_TAG,
      NUM_TAG,
      EXTRA_DATA_TAG,
      EXTRA_ELEMENT_TAG,
      TAGS_TAG
   }
xml_tag_t;

/* To identify hash tree nodes. */
typedef enum
   {
      ROOT_NODE,
      GRID_NODE,
      CLUSTER_NODE,
      HOST_NODE,
      METRIC_NODE
   }
node_type_t;

/* The types of filters we support. */
typedef enum
   {
      NO_FILTER,
      SUMMARY
   }
filter_type_t;

struct xml_tag
   {
      const char *name;
      xml_tag_t tag;
   };

typedef struct
   {
      char *name;
      unsigned int step;
      unsigned int num_sources;
      g_inet6_addr **sources;
      int dead;
      int last_good_index;
   }
data_source_list_t;

/* The size of an ethernet frame, minus IP/UDP headers (1472)
 * plus a bit since gmond sends meta data in binary format, while
 * we get everything in ascii.
 */
#define GMETAD_FRAMESIZE 1572

/* We convert numeric types to binary so we can use the
 * metric_t to compute summaries. */
typedef union
   {
      double d;
      int str;
   }
metric_val_t;

typedef struct
   {
      int fd;
      unsigned int valid:1;
      unsigned int http:1;
      struct sockaddr_in addr;
      filter_type_t filter;
      struct timeval now;
   }
client_t;

/* sacerdoti: The base class for a hash node.
 * Any hash node type can be cast to this (OO style). */
typedef struct Generic_type
   {
      node_type_t id;
      int (*report_start)(struct Generic_type *self, datum_t *key,
                          client_t *client, void *arg);
      int (*report_end)(struct Generic_type *self, client_t *client, void *arg);
      hash_t *children;
      char *therest;
   }
Generic_t;

/* The reporting functions for all node types. */
typedef int (*report_start_func)(Generic_t *self, datum_t *key,
                                 client_t *client, void *arg);
typedef int (*report_end_func)(Generic_t *self, client_t *client, void *arg);

/* sacerdoti: these are used for root, clusters, and grids. */
typedef struct
   {
      node_type_t id;
      report_start_func report_start;
      report_end_func report_end;
      hash_t *authority; /* Null for a grid. */
      short int authority_ptr; /* An authority URL. */
      hash_t *metric_summary;
      hash_t *metric_summary_pending;
      pthread_mutex_t *sum_finished; /* A lock held during summarization. */
      data_source_list_t *ds;
      uint32_t hosts_up;
      uint32_t hosts_down;
      uint32_t localtime;
      short int owner;
      short int latlong;
      short int url;
      short int stringslen;
      char strings[GMETAD_FRAMESIZE];
   }
Source_t;

/* See Metric_t struct below for an explanation of the strings buffer.
 * The hash key is the node's name for easier subtree addressing. */
typedef struct
   {
      node_type_t id;
      report_start_func report_start;
      report_end_func report_end;
      hash_t *metrics;
      struct timeval t0; /* A local timestamp, for TN */
      short int ip;
      uint32_t tn;
      uint32_t tmax;
      uint32_t dmax;
      short int location;
      short int tags;
      uint32_t reported;
      uint32_t started;
      short int stringslen;
      char strings[GMETAD_FRAMESIZE];
   }
Host_t;

#define MAX_EXTRA_ELEMENTS  32

/* sacerdoti: Since we don't know the length of the string fields,
 * we place them sequentially in the strings buffer. The
 * order of strings in the buffer is usually:
 * "val, type, units, slope, source"  (name is hash key). 
 * The value of these fields are offsets into the strings buffer.
 */
typedef struct
   {
      node_type_t id;
      report_start_func report_start;
      report_end_func report_end;
      hash_t *leaf; /* Always NULL. */
      struct timeval t0;
      metric_val_t val;
      short int name;
      short int valstr; /* An optimization to speed queries. */
      short int precision; /* Number of decimal places for floats. */
      uint32_t num;
      short int type;
      short int units;
      uint32_t tn;
      uint32_t tmax;
      uint32_t dmax;
      short int slope;
      short int source;
      short int ednameslen;
      short int edvalueslen;
      short int ednames[MAX_EXTRA_ELEMENTS];
      short int edvalues[MAX_EXTRA_ELEMENTS];
      short int stringslen;
      char strings[GMETAD_FRAMESIZE];
   }
Metric_t;

#ifndef SYS_CALL
#define SYS_CALL(RC,SYSCALL) \
   do {                      \
       RC = SYSCALL;         \
   } while (RC < 0 && errno == EINTR);
#endif

#ifdef WITH_MEMCACHED
memcached_pool_st* memcached_connection_pool;
#endif /* WITH_MEMCACHED */

#endif

#ifdef WITH_RIEMANN
#define RIEMANN_CB_CLOSED 0
#define RIEMANN_CB_OPEN 1
#define RIEMANN_CB_HALF_OPEN 2

#define RIEMANN_TCP_TIMEOUT 500 /* ms */
#define RIEMANN_RETRY_TIMEOUT 15 /* seconds */
#define RIEMANN_MAX_FAILURES 5
#endif /* WITH_RIEMANN */
