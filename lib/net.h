#ifndef NET_H
#define NET_H 1

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include "llist.h"
#ifdef FREEBSD
#include <sys/types.h>
#endif
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <netinet/tcp.h>
#ifdef SOLARIS
#include <sys/sockio.h>
#endif

#define G_SOCKADDR_IN(s) (*((struct sockaddr_in*) &s))

typedef struct
{
  char *name;
  struct sockaddr sa;
  unsigned int ref_count;
} g_inet_addr;

typedef struct
{
  char *name;
  struct addrinfo *ai;
  unsigned int ref_count;
} g_inet6_addr;

typedef struct
{
  int sockfd;
  struct sockaddr sa;
  unsigned int ref_count;
} g_mcast_socket;

/* No difference between mcast and tcp sockets for now */
typedef g_mcast_socket g_tcp_socket;
typedef g_mcast_socket g_udp_socket;

/************** INETADDR  ****************/

const char *    g_inet_ntop( int af, void *src, char *dst, size_t cnt );

int             g_gethostbyname(const char* hostname, struct sockaddr_in* sa, char** nicename);

char*           g_gethostbyaddr(const char* addr, size_t length, int type);

int             g_getaddrinfo(const char* hostname, const char* service, g_inet6_addr* ia, char** nicename);

g_inet_addr*    g_inetaddr_new (const char* name, int port);

void            g_inetaddr_delete (g_inet_addr* ia);

int             g_inetaddr_get_port(const g_inet_addr* ia);

void            g_inetaddr_set_port(const g_inet_addr* ia, unsigned int port);

int             g_inetaddr_is_multicast (const g_inet_addr* inetaddr);

g_inet_addr*    g_inetaddr_get_interface_to (const g_inet_addr* addr);

llist_entry *   g_inetaddr_list_interfaces (void);

/************** MULTICAST ******************/

g_mcast_socket* g_mcast_in ( char *channel, unsigned short port, struct in_addr *mcast_addr);

g_mcast_socket* g_mcast_out( char *channel, unsigned short port, struct in_addr *mcast_addr, unsigned char ttl);

g_mcast_socket* g_mcast_socket_new (const g_inet_addr* ia);

void            g_mcast_socket_delete (g_mcast_socket* ms);

int             g_mcast_socket_connect(g_mcast_socket *ms);

int             g_mcast_socket_bind   (g_mcast_socket *ms);

int             g_mcast_socket_join_group (g_mcast_socket* ms, const g_inet_addr* ia, struct in_addr *interface);

int             g_mcast_socket_leave_group (g_mcast_socket* ms, const g_inet_addr* ia);

int             g_mcast_socket_is_loopback (const g_mcast_socket* ms);

int             g_mcast_socket_set_loopback (g_mcast_socket* ms, int b);

int             g_mcast_socket_get_ttl (const g_mcast_socket* ms);

int             g_mcast_socket_set_ttl(g_mcast_socket* ms, int val);

void            g_mcast_socket_ref(g_mcast_socket* s);

void            g_mcast_socket_unref(g_mcast_socket* s);

/************** TCP **************************/

g_tcp_socket*   g_tcp_socket_connect (const char* hostname, int port);

g_tcp_socket*   g_tcp_socket_new (const g_inet_addr* addr);

g_tcp_socket*   g_tcp6_socket_new (const g_inet6_addr* addr);

void            g_tcp_socket_delete(g_tcp_socket* s);

g_tcp_socket*   g_tcp_socket_server_new (int port);

g_tcp_socket*   g_tcp_socket_server_new_interface (const g_inet_addr* iface);

g_tcp_socket *  g_tcp_socket_server_accept (g_tcp_socket* socket);

#endif
