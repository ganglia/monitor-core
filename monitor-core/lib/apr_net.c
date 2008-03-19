#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <apr_strings.h>
#include <apr_network_io.h>
#include <apr_portable.h>

/* 
 * FIXME: apr_arch_networkio.h is not public
 *        define apr_sockaddr_vars_set locally so it can be used
 */
void apr_sockaddr_vars_set(apr_sockaddr_t *, int, apr_port_t);

#include <apr_net.h>
#include <apr_version.h>

#include <sys/ioctl.h>
#include <net/if.h>

#ifdef SOLARIS
#include <sys/sockio.h>  /* for SIOCGIFADDR */
#endif

#include "error.h"

const char *apr_inet_ntop(int af, const void *src, char *dst, apr_size_t size);

/* This function is copied directly from the 
 * apr_sockaddr_ip_get() function and modified to take a static
 * buffer instead of needing to malloc memory from a pool */
APR_DECLARE(apr_status_t) apr_sockaddr_ip_buffer_get(char *addr, int len,
                                         apr_sockaddr_t *sockaddr)
{
  if(!sockaddr || !addr || len < sockaddr->addr_str_len)
    {
      return APR_EINVAL;
    }
  /* this function doesn't malloc memory from the sockaddr pool...
   * old code...
   *addr = apr_palloc(sockaddr->pool, sockaddr->addr_str_len);
   */
    apr_inet_ntop(sockaddr->family,
                  sockaddr->ipaddr_ptr,
                  addr,
                  sockaddr->addr_str_len);
#if APR_HAVE_IPV6
    if (sockaddr->family == AF_INET6 &&
        IN6_IS_ADDR_V4MAPPED((struct in6_addr *)sockaddr->ipaddr_ptr)) {
        /* This is an IPv4-mapped IPv6 address; drop the leading
         * part of the address string so we're left with the familiar
         * IPv4 format.
         */

        /* use memmove since the memory areas overlap */
        memmove( addr, addr+7, strlen(addr+7) + 1);/* +1 for \0 */
        
        /* old code
        *addr += strlen("::ffff:");
        */
    }
#endif
    return APR_SUCCESS;
}

static apr_socket_t *
create_net_client(apr_pool_t *context, int type, char *host, apr_port_t port)
{
  apr_sockaddr_t *remotesa = NULL;
  apr_socket_t *sock = NULL;
  apr_status_t status;
  int family = APR_UNSPEC;

  status = apr_sockaddr_info_get(&remotesa, host, APR_UNSPEC, port, 0, context);
  if(status!= APR_SUCCESS)
    {
      return NULL;
    }
  family = remotesa->sa.sin.sin_family;

  /* Created the socket */
  status = apr_socket_create(&sock, family, type, 0, context);
  if(status != APR_SUCCESS)
    {
      return NULL;
    }

  /* Connect the socket to the address */
  status = apr_socket_connect(sock, remotesa);
  if(status != APR_SUCCESS)
    {
      apr_socket_close(sock);
      return NULL;
    }

  return sock;
}

apr_socket_t *
create_udp_client(apr_pool_t *context, char *host, apr_port_t port)
{
  return create_net_client(context, SOCK_DGRAM, host, port);
}

static apr_socket_t *
create_net_server(apr_pool_t *context, int32_t ofamily, int type, apr_port_t port, 
                  char *bind_addr, int blocking)
{
  apr_sockaddr_t *localsa = NULL;
  apr_socket_t *sock = NULL;
  apr_status_t stat;
  int32_t family = ofamily;

  /* We set family to the family specified in the option.  If however a bind address
   * is also specified, it's family will take precedence.
   * e.g. ofamily = APR_INET6 but the bind address is "127.0.0.1" which is IPv4 
   * the family will be set to the bind address family */
  if(bind_addr)
    {
      stat = apr_sockaddr_info_get(&localsa, bind_addr, APR_UNSPEC, port, 0, context);
      if (stat != APR_SUCCESS)
        return NULL;

      family = localsa->sa.sin.sin_family;
    }

  stat = apr_socket_create(&sock, family, type, 0, context);
  if( stat != APR_SUCCESS )
    return NULL;

  if(!blocking){
     /* This is a non-blocking server */
     stat = apr_socket_opt_set(sock, APR_SO_NONBLOCK, 1);
     if (stat != APR_SUCCESS)
     {
           apr_socket_close(sock);
           return NULL;
     }
  }

  stat = apr_socket_opt_set(sock, APR_SO_REUSEADDR, 1);
  if (stat != APR_SUCCESS)
    {
      apr_socket_close(sock);
      return NULL;
    }

  if(!localsa)
    {
      apr_socket_addr_get(&localsa, APR_LOCAL, sock);
      apr_sockaddr_vars_set(localsa, localsa->family, port);
    }

#if APR_HAVE_IPV6
   if (localsa->family == APR_INET6) 
     {
       int one = 1;
       /* Don't accept IPv4 connections on an IPv6 listening socket */
       stat = apr_socket_opt_set(sock, APR_IPV6_V6ONLY, one);
       if(stat == APR_ENOTIMPL)
         {
           err_msg("Warning: your operating system does not support IPV6_V6ONLY!\n");
           err_msg("This means that you are also listening to IPv4 traffic on port %d\n",
               port);
           err_msg("This IPv6=>IPv4 mapping may be a security risk.\n");
         }
     }
#endif

  stat = apr_socket_bind(sock, localsa);
  if( stat != APR_SUCCESS)
    {
       apr_socket_close(sock);
       return NULL;
    }

  return sock;
}

apr_socket_t *
create_udp_server(apr_pool_t *context, int32_t family, apr_port_t port, 
                  char *bind_addr)
{
  return create_net_server(context, family, SOCK_DGRAM, port, bind_addr, 0);
}

apr_socket_t *
create_tcp_server(apr_pool_t *context, int32_t family, apr_port_t port, 
                  char *bind_addr, char *interface, int blocking)
{
  apr_socket_t *sock = create_net_server(context, family, SOCK_STREAM, port,
                                         bind_addr, blocking);
  if(!sock)
    {
      return NULL;
    }
  if(apr_socket_listen(sock, 5) != APR_SUCCESS) 
    {
      return NULL;
    }
  return sock;
}

/*XXX This should really be replaced by the APR mcast functions */
static apr_status_t
mcast_join( apr_pool_t *context, apr_socket_t *sock, char *mcast_channel, apr_port_t port, char *ifname )
{
  apr_status_t status;
  int rval;
  apr_sockaddr_t *sa;
  apr_os_sock_t s;

  status = apr_sockaddr_info_get(&sa, mcast_channel , APR_UNSPEC, port, 0, context);
  if(status != APR_SUCCESS)
    {
      return status;
    }

  apr_os_sock_get(&s, sock);

  switch( sa->family ) /* (*sa)->sa.sin.sin_family */
    {
    case APR_INET:
      {
        struct ip_mreq mreq[1];
        struct ifreq ifreq[1];
        
        /* &((*sa)->sa.sin.sin_addr */
        memcpy(&mreq->imr_multiaddr, &(sa->sa.sin.sin_addr), 
               sizeof mreq->imr_multiaddr);
        
        memset(&ifreq,0, sizeof(ifreq));
        if(ifname)
          {
            memset(ifreq, 0, sizeof(struct ifreq));
            strncpy(ifreq->ifr_name, ifname, IFNAMSIZ);
            if(ioctl(s, SIOCGIFADDR, ifreq) == -1)
              {
                return APR_EGENERAL;
              }
          }
        else
          {
            /* wildcard address (let the kernel decide) */
            mreq->imr_interface.s_addr = htonl(INADDR_ANY);
          }
        
        memcpy(&mreq->imr_interface, 
               &((struct sockaddr_in *)&ifreq->ifr_addr)->sin_addr, 
               sizeof mreq->imr_interface);
        
        rval = setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
                mreq, sizeof mreq);
        if(rval<0)
          {
            return APR_EGENERAL;
          }
        break;
      }
#if APR_HAVE_IPV6
    case APR_INET6:
      {
        struct ipv6_mreq mreq[1];
        struct ifreq ifreq[1];
        
        /* &((*sa)->sa.sin6.sin6_addr)*/
        memcpy(&mreq->ipv6mr_multiaddr, &(sa->sa.sin6.sin6_addr),
               sizeof mreq->ipv6mr_multiaddr);
        
        memset(&ifreq,0, sizeof(ifreq));
        if(ifname)
          {
            strncpy(ifreq->ifr_name, ifname, IFNAMSIZ);
          }
        
        if (ioctl(s, SIOCGIFADDR, ifreq) == -1)
            return -1;
        
        rval = setsockopt(s, IPPROTO_IPV6, IPV6_JOIN_GROUP, mreq, sizeof mreq);
        break;
      }
#endif
    default:
        /* Set errno to EPROTONOSUPPORT */
        return -1;
    }

  return APR_SUCCESS;
}

apr_socket_t *
create_mcast_client(apr_pool_t *context, char *mcast_ip, apr_port_t port, int ttl)
{
    apr_socket_t *socket = create_udp_client(context, mcast_ip, port);
    if(!socket)
      {
        return NULL;
      }
    apr_mcast_hops(socket, ttl);

    return socket;
}

apr_socket_t *
create_mcast_server(apr_pool_t *context, int32_t family, char *mcast_ip, apr_port_t port, char *bind_addr, char *interface)
{
  apr_status_t status = APR_SUCCESS;
  /* NOTE: If bind is set to mcast_ip in the configuration file, then we will bind the 
   * the multicast address to the socket as well as the port and prevent any 
   * datagrams that might be delivered to this port from being processed. Otherwise,
   * packets destined to the same port (but a different multicast/unicast channel) will be
   * processed. */
  apr_socket_t *socket = create_udp_server(context, family, port, bind_addr);
  if(!socket)
    {
      return NULL;
    }

  /* TODO: We can probe for a list of interfaces and perform multiple join calls for the same
   * socket to have it listen for multicast traffic on all interfaces (important for
   * multihomed machines). */
  if(interface && !apr_strnatcasecmp(interface, "ALL"))
    {
      /* for(each interface)
       * {
       *   mcast_join(...);
       * }
       */
    }
  else
    {
      status = mcast_join(context,  socket, mcast_ip, port, interface );
    }

  return status == APR_SUCCESS? socket: NULL;
}
