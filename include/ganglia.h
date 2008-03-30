#ifndef GANGLIA_H
#define GANGLIA_H 1
#include <rpc/rpc.h>

#include "gm_protocol.h"

#include <confuse.h>
#include <apr_pools.h>

/* For C++ */
#ifdef __cplusplus
#define BEGIN_C_DECLS extern "C" {
#define END_C_DECLS }
#else
#define BEGIN_C_DECLS
#define END_C_DECLS
#endif

#define SPOOF "SPOOF"
#define SPOOF_HOST SPOOF"_HOST"
#define SPOOF_HEARTBEAT SPOOF"_HEARTBEAT"

enum ganglia_slope {
	GANGLIA_SLOPE_ZERO = 0,
    GANGLIA_SLOPE_POSITIVE,
    GANGLIA_SLOPE_NEGATIVE,
    GANGLIA_SLOPE_BOTH,
    GANGLIA_SLOPE_UNSPECIFIED
};
typedef enum ganglia_slope ganglia_slope_t;

typedef struct apr_pool_t *         Ganglia_pool;
typedef struct cfg_t *              Ganglia_gmond_config;
typedef struct apr_array_header_t * Ganglia_udp_send_channels;

struct Ganglia_metric {
   apr_pool_t *pool;
   struct Ganglia_metadata_message *msg; /* defined in ./lib/gm_protocol.x */
   char *value;
   void *extra;
};
typedef struct Ganglia_metric * Ganglia_metric;

BEGIN_C_DECLS
Ganglia_gmond_config
Ganglia_gmond_config_create(char *path, int fallback_to_default);
void Ganglia_gmond_config_destroy(Ganglia_gmond_config config);

Ganglia_udp_send_channels
Ganglia_udp_send_channels_create(Ganglia_pool context,Ganglia_gmond_config config);
void Ganglia_udp_send_channels_destroy(Ganglia_udp_send_channels channels);

int Ganglia_udp_send_message(Ganglia_udp_send_channels channels, char *buf, int len );

Ganglia_metric Ganglia_metric_create( Ganglia_pool parent_pool );
int Ganglia_metric_set( Ganglia_metric gmetric, char *name, char *value, char *type, char *units, unsigned int slope, unsigned int tmax, unsigned int dmax);
int Ganglia_metric_send( Ganglia_metric gmetric, Ganglia_udp_send_channels send_channels );
int Ganglia_metadata_send( Ganglia_metric gmetric, Ganglia_udp_send_channels send_channels );
void Ganglia_metadata_add( Ganglia_metric gmetric, char *name, char *value );
int Ganglia_value_send( Ganglia_metric gmetric, Ganglia_udp_send_channels send_channels );

void Ganglia_metric_destroy( Ganglia_metric gmetric );

Ganglia_pool Ganglia_pool_create( Ganglia_pool parent );
void Ganglia_pool_destroy( Ganglia_pool pool );

ganglia_slope_t cstr_to_slope(const char* str);
const char*     slope_to_cstr(unsigned int slope);

void debug_msg(const char *format, ...); 
void set_debug_msg_level(int level);
int get_debug_msg_level();
void err_msg (const char *fmt, ...);

#define END_C_DECLS


#endif
