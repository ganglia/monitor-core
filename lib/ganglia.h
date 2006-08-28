#ifndef GANGLIA_H
#define GANGLIA_H 1

#define GANGLIA_DEFAULT_MCAST_CHANNEL "239.2.11.71"
#define GANGLIA_DEFAULT_MCAST_PORT    8649
#define GANGLIA_DEFAULT_XML_PORT      8649

#ifndef SYNAPSE_FAILURE
#define SYNAPSE_FAILURE -1
#endif
#ifndef SYNAPSE_SUCCESS
#define SYNAPSE_SUCCESS 0
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#ifdef TIME_WITH_SYS_TIME
#include <time.h>
#endif
#endif

/* For C++ */
#ifdef __cplusplus
#define BEGIN_C_DECLS extern "C" {
#define END_C_DECLS }
#else
#define BEGIN_C_DECLS
#define END_C_DECLS
#endif

typedef struct apr_pool_t *         Ganglia_pool;
typedef struct cfg_t *              Ganglia_gmond_config;
typedef struct apr_array_header_t * Ganglia_udp_send_channels;

struct Ganglia_gmetric {
   Ganglia_pool pool;
   struct Ganglia_gmetric_message *msg; /* defined in ./lib/protocol.x */
};
typedef struct Ganglia_gmetric * Ganglia_gmetric;

Ganglia_gmond_config
Ganglia_gmond_config_create(char *path, int fallback_to_default);
void Ganglia_gmond_config_destroy(Ganglia_gmond_config config);

Ganglia_udp_send_channels
Ganglia_udp_send_channels_create(Ganglia_pool context,Ganglia_gmond_config config);
void Ganglia_udp_send_channels_destroy(Ganglia_udp_send_channels channels);

Ganglia_pool Ganglia_pool_create( Ganglia_pool parent );
void Ganglia_pool_destroy( Ganglia_pool pool );

int Ganglia_udp_send_message(Ganglia_udp_send_channels channels, char *buf, int len );

Ganglia_gmetric Ganglia_gmetric_create( Ganglia_pool parent_pool );
int Ganglia_gmetric_set( Ganglia_gmetric gmetric, char *name, char *value, char *type, char *units, unsigned int slope, unsigned int tmax, unsigned int dmax);
int Ganglia_gmetric_send( Ganglia_gmetric gmetric, Ganglia_udp_send_channels send_channels );
// Yemi
int Ganglia_gmetric_send_spoof( Ganglia_gmetric gmetric, Ganglia_udp_send_channels send_channels, char* spoof_info,int heartbeat);
void Ganglia_gmetric_destroy( Ganglia_gmetric gmetric );

void build_default_gmond_configuration(Ganglia_pool context);
char *Ganglia_default_collection_groups(void);



extern int gexec_errno;

#define GEXEC_TIMEOUT 60

#define GEXEC_HOST_STRING_LEN  256
struct gexec_host_t {
  char ip[64];
  char name[GEXEC_HOST_STRING_LEN];
  char domain[GEXEC_HOST_STRING_LEN];
  double load_one;
  double load_five;
  double load_fifteen;
  double cpu_user;
  double cpu_nice;
  double cpu_system;
  double cpu_idle;
  double cpu_wio;
  unsigned int proc_run;
  unsigned int proc_total;
  unsigned int cpu_num;
  time_t last_reported;
  int gexec_on;
  int name_resolved;
};
typedef struct gexec_host_t gexec_host_t;

typedef struct
   {
      char name[256];
      time_t localtime;
      unsigned int num_hosts;
      void *hosts;
      unsigned int num_gexec_hosts;
      void *gexec_hosts;
      unsigned int num_dead_hosts;
      void *dead_hosts;

      /* Used internally */
      int malloc_error;
      gexec_host_t *host;
      int host_up;
      int start;
	}
gexec_cluster_t;

BEGIN_C_DECLS
int gexec_cluster_free ( gexec_cluster_t *cluster );
int gexec_cluster (gexec_cluster_t *cluster, char *ip, unsigned short port);
END_C_DECLS

#endif
