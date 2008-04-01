#ifndef GANGLIA_PRIV_H
#define GANGLIA_PRIV_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#ifdef TIME_WITH_SYS_TIME
#include <time.h>
#endif
#endif

#include <gm_msg.h>

#include <errno.h>
#ifndef SYS_CALL
#define SYS_CALL(RC,SYSCALL) \
   do {                      \
       RC = SYSCALL;         \
   } while (RC < 0 && errno == EINTR);
#endif

#define GANGLIA_DEFAULT_MCAST_CHANNEL "239.2.11.71"
#define GANGLIA_DEFAULT_MCAST_PORT    8649
#define GANGLIA_DEFAULT_XML_PORT      8649

#ifndef SYNAPSE_FAILURE
#define SYNAPSE_FAILURE -1
#endif
#ifndef SYNAPSE_SUCCESS
#define SYNAPSE_SUCCESS 0
#endif

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

int gexec_cluster_free ( gexec_cluster_t *cluster );
int gexec_cluster (gexec_cluster_t *cluster, char *ip, unsigned short port);
char *Ganglia_default_collection_groups(void);

#endif
