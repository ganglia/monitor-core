#ifndef GANGLIA_GEXEC_H
#define GANGLIA_GEXEC_H 1

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

#endif
