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
#ifdef WITH_RIEMANN
#include "riemann.pb-c.h"

g_udp_socket*
init_riemann_udp_socket (const char *hostname, uint16_t port);
g_udp_socket*
init_riemann_udp6_socket (const char *hostname, uint16_t port);

g_tcp_socket*
init_riemann_tcp_socket (const char *hostname, uint16_t port);
g_tcp_socket*
init_riemann_tcp6_socket (const char *hostname, uint16_t port);

Event *                                          /* Ganglia   =>  Riemann */
create_riemann_event (const char *grid,          /* grid      =>  grid */
                      const char *cluster,       /* cluster   =>  cluster */
                      const char *host,          /* host      =>  host */
                      const char *ip,            /* ip        =>  ip */
                      const char *metric,        /* metric    =>  service */
                      const char *value,         /* value     =>  metric (if int or float) or state (if string) */
                      const char *type,          /* "int", "float" or "string" */
                      const char *units,         /* units     =>  description */
                      const char *state,         /* not used  =>  state (overrides metric value if also supplied) */
                      unsigned int localtime,    /* localtime =>  time */
                      const char *tags,          /* tags      =>  tags */
                      const char *location,      /* location  =>  location */
                      unsigned int ttl           /* tmax      =>  ttl */
                      );
int
send_event_to_riemann (Event *event);

int
send_message_to_riemann (Msg *message);

int
destroy_riemann_event(Event *event);

int
destroy_riemann_msg(Msg *message);
#endif /* WITH_RIEMANN */
