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
#define MAX_METRIC_HASH_VALUE 64

/* For xml_hash */
typedef enum {
   GANGLIA_XML_TAG,
   CLUSTER_TAG,
   HOST_TAG,
   NAME_TAG,
   METRIC_TAG,
   TN_TAG,
   TMAX_TAG,
   VAL_TAG,
   TYPE_TAG,
   SLOPE_TAG,
} xml_tag_t;

struct xml_tag
   {
      const char *name;
      xml_tag_t tag;
   };

/* This value is taken from the xml_hash.c file (MAX_HASH_VALUE+1) */
#define MAX_XML_HASH_VALUE 20

typedef struct
   {
      unsigned int index;
      char *name;
      unsigned int num_sources;
      g_inet_addr **sources;
   }
data_source_list_t;

typedef union
   {
      float f;
      unsigned int uint32;
      double d;
   }
val_t;

#ifndef SYS_CALL
#define SYS_CALL(RC,SYSCALL) \
   do {                      \
       RC = SYSCALL;         \
   } while (RC < 0 && errno == EINTR);
#endif

#endif
