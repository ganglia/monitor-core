#ifndef METRIC_H
#define METRIC_H 1

#include "libmetrics.h"

typedef g_val_t (*metric_func)(void);

/*
 * Each host parameter is assigned it's own metric_t structure.  This
 * structure is used by both the monitoring and multicasting threads.
 */
typedef struct
{
   const char name[16];          /* the name of the metric */
   metric_func func; 
   int check_threshold;
   int mcast_threshold;
   g_val_t now;
   g_val_t last_mcast;     /* the value at last multicast */
   int value_threshold; 
   int check_min;          /* check_min and check_max define a range of secs */
   int check_max;          /*    which pass between each /proc check for the metric */
   int mcast_min;          /* msg_min and msg_max define a range of secs */
   int mcast_max;          /*    which a mcast of the metric is forced */
   g_type_t type;          /* type of data in our union */
   char units[32];         /* units the value are in */
   char fmt[16];           /* how to format the binary to a human-readable string */
   int key;                /* the unique key for this metric */
}
metric_t;          

#endif  /* METRIC_H */
