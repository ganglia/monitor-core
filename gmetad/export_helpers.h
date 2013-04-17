#include "ganglia.h"

#ifdef WITH_MEMCACHED
#include <libmemcached-1.0/memcached.h>
#include <libmemcachedutil-1.0/util.h>
#endif /* WITH_MEMCACHED */

/* Tracks the macros we need to interpret in the graphite_path */
typedef struct 
{
	char *torepl;
	char *replwith;
}
graphite_path_macro;

#ifdef WITH_MEMCACHED
int
write_data_to_memcached ( const char *cluster, const char *host, const char *metric, 
                    const char *sum, unsigned int process_time, unsigned int expiry );
#endif /* WITH_MEMCACHED */

g_udp_socket*
init_carbon_udp_socket (const char *hostname, uint16_t port);
int
write_data_to_carbon ( const char *source, const char *host, const char *metric, 
                    const char *sum, unsigned int process_time);
