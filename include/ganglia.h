#ifndef GANGLIA_H
#define GANGLIA_H 1

#include <rpc/types.h>
#include <rpc/xdr.h>

#include <gm_msg.h>
#ifndef GM_PROTOCOL_GUARD
#include <netinet/in.h>
#include <gm_protocol.h>
#endif

#define SPOOF "SPOOF"
#define SPOOF_HOST SPOOF"_HOST"
#define SPOOF_HEARTBEAT SPOOF"_HEARTBEAT"
#define SPOOF_NAME SPOOF"_NAME"

enum ganglia_slope {
   GANGLIA_SLOPE_ZERO = 0,
   GANGLIA_SLOPE_POSITIVE,
   GANGLIA_SLOPE_NEGATIVE,
   GANGLIA_SLOPE_BOTH,
   GANGLIA_SLOPE_UNSPECIFIED,
   GANGLIA_SLOPE_DERIVATIVE,
   GANGLIA_SLOPE_LAST_LEGAL_VALUE=GANGLIA_SLOPE_DERIVATIVE
};
typedef enum ganglia_slope ganglia_slope_t;

typedef struct Ganglia_pool* Ganglia_pool;
typedef struct Ganglia_gmond_config* Ganglia_gmond_config;
typedef struct Ganglia_udp_send_channels* Ganglia_udp_send_channels;

struct Ganglia_metric {
   Ganglia_pool pool;
   struct Ganglia_metadata_message *msg; /* defined in ./lib/gm_protocol.x */
   char *value;
   void *extra;
};
typedef struct Ganglia_metric * Ganglia_metric;

#ifdef __cplusplus
extern "C" {
#endif

Ganglia_gmond_config
Ganglia_gmond_config_create(char *path, int fallback_to_default);
void Ganglia_gmond_config_destroy(Ganglia_gmond_config config);

Ganglia_udp_send_channels
Ganglia_udp_send_channels_create(Ganglia_pool p, Ganglia_gmond_config config);
void Ganglia_udp_send_channels_destroy(Ganglia_udp_send_channels channels);

int Ganglia_udp_send_message(Ganglia_udp_send_channels channels, char *buf, int len );

Ganglia_metric Ganglia_metric_create( Ganglia_pool parent_pool );
int Ganglia_metric_set( Ganglia_metric gmetric, char *name, char *value, char *type, char *units, unsigned int slope, unsigned int tmax, unsigned int dmax);
int Ganglia_metric_send( Ganglia_metric gmetric, Ganglia_udp_send_channels send_channels );
int Ganglia_metadata_send( Ganglia_metric gmetric, Ganglia_udp_send_channels send_channels );
int Ganglia_metadata_send_real( Ganglia_metric gmetric, Ganglia_udp_send_channels send_channels, char *override_string );
void Ganglia_metadata_add( Ganglia_metric gmetric, char *name, char *value );
int Ganglia_value_send( Ganglia_metric gmetric, Ganglia_udp_send_channels send_channels );

void Ganglia_metric_destroy( Ganglia_metric gmetric );

Ganglia_pool Ganglia_pool_create( Ganglia_pool parent );
void Ganglia_pool_destroy( Ganglia_pool pool );

ganglia_slope_t cstr_to_slope(const char* str);
const char*     slope_to_cstr(unsigned int slope);

void build_default_gmond_configuration(Ganglia_pool p);

void set_reload_required();

#ifdef __cplusplus
}
#endif

#endif
