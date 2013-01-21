#include "ganglia.h"

int
write_data_to_rrd ( const char *source, const char *host, const char *metric, 
                    const char *sum, const char *num, unsigned int step,
                    unsigned int process_time, ganglia_slope_t slope);

#ifdef WITH_MEMCACHED
int
write_data_to_memcached ( const char *source, const char *host, const char *metric, 
                    const char *sum, unsigned int process_time);
#endif /* WITH_MEMCACHED */

int
write_data_to_carbon ( const char *source, const char *host, const char *metric, 
                    const char *sum, unsigned int process_time);
