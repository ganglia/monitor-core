#ifndef GANGLIA_H
#define GANGLIA_H 1

#ifdef __cplusplus
extern "C" {
#endif

#define SYNAPSE_SUCCESS 0
#define SYNAPSE_FAILURE -1

/* Public GEXEC Functions and Structures */
extern int gexec_errno;

#define GEXEC_TIMEOUT 60
#define MAXHOSTNAME 128

typedef struct
   {
      char ip[16];
      char name[MAXHOSTNAME];
      char domain[MAXHOSTNAME];
      double load_one;
      double load_five;
      double load_fifteen;
      double cpu_user;
      double cpu_nice;
      double cpu_system;
      double cpu_idle;
      unsigned int proc_run;
      unsigned int proc_total;
      unsigned int cpu_num;
      unsigned long last_reported;
      int gexec_on;
      int name_resolved;
   }
gexec_host_t;

typedef struct
   {
      char name[256];
      unsigned long localtime;
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
/* End GEXEC functions and structures */


#define GANGLIA_DEFAULT_MCAST_CHANNEL "239.2.11.71"
#define GANGLIA_DEFAULT_MCAST_PORT 8649
#define GANGLIA_DEFAULT_XML_PORT 8649

/* 
 * Max multicast message: 1500 bytes (Ethernet max frame size)
 * minus 20 bytes for IPv4 header, minus 8 bytes for UDP header.
 */
#ifndef MAX_MCAST_MSG
#define MAX_MCAST_MSG 1472
#endif

#ifdef __cplusplus
}
#endif




#endif /* GANGLIA_H */
