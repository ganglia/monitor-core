#include "ganglia.h"

#ifdef WITH_MEMCACHED
#include <libmemcached-1.0/memcached.h>
#include <libmemcachedutil-1.0/util.h>
#endif /* WITH_MEMCACHED */

int
write_data_to_rrd ( const char *source, const char *host, const char *metric, 
                    const char *sum, const char *num, unsigned int step,
                    unsigned int process_time, ganglia_slope_t slope);

#ifdef WITH_MEMCACHED
int
write_data_to_memcached ( const char *cluster, const char *host, const char *metric, 
                    const char *sum, unsigned int process_time, unsigned int expiry );
#endif /* WITH_MEMCACHED */

int
write_data_to_carbon ( const char *source, const char *host, const char *metric, 
                    const char *sum, unsigned int process_time);
