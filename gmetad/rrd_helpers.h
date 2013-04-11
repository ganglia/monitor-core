#include "ganglia.h"

int
write_data_to_rrd ( const char *source, const char *host, const char *metric, 
                    const char *sum, const char *num, unsigned int step,
                    unsigned int process_time, ganglia_slope_t slope);
