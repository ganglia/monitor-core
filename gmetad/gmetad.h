#ifndef GMETAD_H
#define GMETAD_H 1 
#include <ganglia/net.h>

/* For metric_hash */
typedef enum {
   UINT32,
   FLOAT,
   DOUBLE,
} metric_type;

struct ganglia_metric
   {
      const char *name;
      metric_type type;
   };

/* This value is taken from the metric_hash.c file (MAX_HASH_VALUE+1) */
#define MAX_METRIC_HASH_VALUE 73

/* For xml_hash */
typedef enum {
   GANGLIA_XML_TAG,
   GRID_TAG,
   CLUSTER_TAG,
   HOST_TAG,
   NAME_TAG,
   METRIC_TAG,
   TN_TAG,
   TMAX_TAG,
   VAL_TAG,
   TYPE_TAG,
   SLOPE_TAG,
   VERSION_TAG,
   REPORTED_TAG,
   LOCALTIME_TAG
} xml_tag_t;

struct xml_tag
   {
      const char *name;
      xml_tag_t tag;
   };

/* This value is taken from the xml_hash.c file (MAX_HASH_VALUE+1) */
#define MAX_XML_HASH_VALUE 23

typedef struct
   {
      char *name;
      unsigned int step;
      unsigned int num_sources;
      g_inet_addr **sources;
      long double  sum[MAX_METRIC_HASH_VALUE];
      unsigned int num[MAX_METRIC_HASH_VALUE];
      long double timestamp;   /* added by swagner */
      int dead;
   }
data_source_list_t;

#ifndef SYS_CALL
#define SYS_CALL(RC,SYSCALL) \
   do {                      \
       RC = SYSCALL;         \
   } while (RC < 0 && errno == EINTR);
#endif

#endif
