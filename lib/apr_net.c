#include <stdio.h>
#include <stdlib.h>
#include "apr_network_io.h"
#include "apr_arch_networkio.h"

#include <sys/ioctl.h>
#include <net/if.h>

#ifdef SOLARIS2
#include <sys/sockio.h>  /* for SIOCGIFADDR */
#endif

apr_socket_t *
create_udp_client(apr_pool_t *context, char *ipaddr, apr_port_t port)
{
  apr_sockaddr_t *remotesa = NULL;
  apr_socket_t *sock = NULL;
  apr_status_t status;
  int family = APR_UNSPEC;

  status = apr_sockaddr_info_get(&remotesa, ipaddr, APR_UNSPEC, port, 0, context);
  if(status!= APR_SUCCESS)
    {
      return NULL;
    }
  family = remotesa->sa.sin.sin_family;

  /* Created the socket */
  status = apr_socket_create(&sock, family, SOCK_DGRAM, context);
  if(status != APR_SUCCESS)
    {
      return NULL;
    }

  /* Connect the socket to the address */
  status = apr_connect(sock, remotesa);
  if(status != APR_SUCCESS)
    {
      apr_socket_close(sock);
      return NULL;
    }

  return sock;
}

apr_socket_t *
create_udp_server(apr_pool_t *context, apr_port_t port, char *bind)
{
  apr_sockaddr_t *localsa = NULL;
  apr_socket_t *sock = NULL;
  apr_status_t stat;
  int family = APR_UNSPEC;

  if(bind)
    {
      stat = apr_sockaddr_info_get(&localsa, bind, APR_UNSPEC, port, 0, context);
      if (stat != APR_SUCCESS)
        return NULL;

      family = localsa->sa.sin.sin_family;
    }

  stat = apr_socket_create(&sock, family, SOCK_DGRAM, context);
  if( stat != APR_SUCCESS )
    return NULL;

  /* Setup to be non-blocking */
  stat = apr_setsocketopt(sock, APR_SO_NONBLOCK, 1);
  if (stat != APR_SUCCESS)
    {
      apr_socket_close(sock);
      return NULL;
    }

  stat = apr_setsocketopt(sock, APR_SO_REUSEADDR, 1);
  if (stat != APR_SUCCESS)
    {
      apr_socket_close(sock);
      return NULL;
    }

  if(!localsa)
    {
      apr_socket_addr_get(&localsa, APR_LOCAL, sock);
      apr_sockaddr_port_set(localsa, port);
    }

  stat = apr_bind(sock, localsa);
  if( stat != APR_SUCCESS)
    {
       apr_socket_close(sock);
       /*
       fprintf(stderr, "Could not bind: %s\n", apr_strerror(stat, buf, sizeof buf));
       */
       return NULL;
    }

  return sock;
}

#if 0
apr_socket_t *
create_mcast_server_socket( apr_pool_t *context, char *mcastaddr, apr_port_t port, char *interface_name, apr_sockaddr_t **sa)
{
  apr_socket_t *sock = create_udp_server( context, mcastaddr, port, sa );
  apr_multicast_join( sock, sa, interface_name );
  return sock;
}
#endif

apr_socket_t *
create_tcp_server_socket(apr_pool_t *context, char *ipaddr, apr_port_t port, apr_sockaddr_t **sa )
{
  apr_socket_t *sock = NULL;
  apr_status_t stat;
  char buf[128];

  stat = apr_sockaddr_info_get(sa, ipaddr, APR_UNSPEC, port, 0, context);
  if (stat != APR_SUCCESS) 
    {
      fprintf(stderr, "Couldn't build the socket address correctly: %s\n", 
	      apr_strerror(stat, buf, sizeof buf));
      exit(-1);
    }

  stat = apr_socket_create(&sock, (*sa)->sa.sin.sin_family , SOCK_STREAM, context);
  if ( stat != APR_SUCCESS)
    {
      fprintf(stderr, "Couldn't create socket\n");
      exit(-1);
    }

  /* Setup to be non-blocking */
  stat = apr_setsocketopt(sock, APR_SO_NONBLOCK, 1);
  if (stat != APR_SUCCESS) 
    {
      apr_socket_close(sock);
      fprintf(stderr, "Couldn't set socket option non-blocking\n");
      exit(-1);
    }

  stat = apr_setsocketopt(sock, APR_SO_REUSEADDR, 1);
  if (stat != APR_SUCCESS)
    {
      apr_socket_close(sock);
      fprintf(stderr, "Couldn't set socket option reuseaddr\n");
      exit(-1);
    }

  stat = apr_bind(sock, *sa);
  if( stat != APR_SUCCESS) 
    {
      apr_socket_close(sock);
      fprintf(stderr, "Could not bind: %s\n", apr_strerror(stat, buf, sizeof buf));
      exit(-1);
    }
          
  stat = apr_listen(sock, 5);
  if( stat != APR_SUCCESS) 
    {
      apr_socket_close(sock);
      fprintf(stderr, "Could not listen\n"); 
      exit(-1);
    }

  return sock;
}

#if 0
#include <netinet/in.h>
static int is_multicast(apr_sockaddr_t *address)
{
    switch (address->family)
    {
        case AF_INET:
            return IN_MULTICAST(ntohl(*(long *)&addr->sin);

#ifdef AF_INET6
        case AF_INET6:
            return IN6_IS_ADDR_MULTICAST(&addr->sin6);
#endif
    }

    return 0;
}
#endif

#if 0
static int
mcast_set_if(apr_socket_t *sock, apr_sockaddr_t **sa, const char *ifname)
{
	switch ( (*sa)->sa.sin.sin_family ) {
	case AF_INET: {
		struct in_addr		inaddr;
		struct ifreq		ifreq;

		if (ifname) 
		  {
	            strncpy(ifreq.ifr_name, ifname, IFNAMSIZ);
		    if (ioctl(sock->socketdes, SIOCGIFADDR, &ifreq) < 0)
		      return(-1);
		    memcpy(&inaddr, 
			   &((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr,
			   sizeof(struct in_addr));
                  } 
		else
		  {
		    inaddr.s_addr = htonl(INADDR_ANY);	/* remove prev. set default */
		  }

		return(setsockopt(sock->socketdes, IPPROTO_IP, IP_MULTICAST_IF,
                                  &inaddr, sizeof(struct in_addr)));
	}

#ifdef	AF_INET6
	case AF_INET6: {
		u_int	index;

		if ( (index = ifindex) == 0) {
			if (ifname == NULL) {
				errno = EINVAL;	/* must supply either index or name */
				return(-1);
			}
			if ( (index = if_nametoindex(ifname)) == 0) {
				errno = ENXIO;	/* i/f name not found */
				return(-1);
			}
		}
		return(setsockopt(sock->socketdes, IPPROTO_IPV6, IPV6_MULTICAST_IF,
						  &index, sizeof(index)));
	}
#endif

	default:
		errno = EPROTONOSUPPORT;
		return(-1);
	}
}
#endif

static int
set_interface( apr_socket_t *sock, struct ifreq *ifreq, char *ifname )
{
  memset(ifreq, 0, sizeof(struct ifreq));
  strncpy(ifreq->ifr_name, ifname, IFNAMSIZ);
  return ioctl(sock->socketdes, SIOCGIFADDR, ifreq);
}

apr_status_t
apr_multicast_join( apr_socket_t *sock, apr_sockaddr_t **sa, char *ifname )
{
  int rval;

  switch( (*sa)->sa.sin.sin_family )
    {
    case AF_INET:
	{
	  struct ip_mreq mreq[1];
	  struct ifreq ifreq[1];

	  memcpy(&mreq->imr_multiaddr, &((*sa)->sa.sin.sin_addr), 
		 sizeof mreq->imr_multiaddr);

	  memset(&ifreq,0, sizeof(ifreq));
	  if(ifname)
	    {
	      if(set_interface(sock, ifreq, ifname) == -1)
		{
		  /* error */
		}
	    }
	  else
	    {
	      mreq->imr_interface.s_addr = htonl(INADDR_ANY);
	    }

	  memcpy(&mreq->imr_interface, 
		 &((struct sockaddr_in *)&ifreq->ifr_addr)->sin_addr, 
		 sizeof mreq->imr_interface);

	  rval = setsockopt(sock->socketdes, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
			    mreq, sizeof mreq);
	  if(rval<0)
	    {
	      /* handle error */
	    }
	  break;
	}
#ifdef AF_INET6
    case AF_INET6:
	{
	  struct ipv6_mreq mreq[1];
	  struct ifreq ifreq[1];

          memcpy(&mreq->ipv6mr_multiaddr, &((*sa)->sa.sin6.sin6_addr),
		                  sizeof mreq->ipv6mr_multiaddr);

          memset(&ifreq,0, sizeof(ifreq));
	  if(ifname)
	    {
	      strncpy(ifreq->ifr_name, ifname, IFNAMSIZ);
	    }

	  if (ioctl(sock->socketdes, SIOCGIFADDR, ifreq) == -1)
	                return -1;

	  rval = setsockopt(sock->socketdes, IPPROTO_IPV6, IPV6_JOIN_GROUP, mreq, sizeof mreq);

	  break;
	}
#endif
    default:
      /* Set errno to EPROTONOSUPPORT */
      return -1;
    }

  return APR_SUCCESS;
}

apr_status_t
apr_multicast_leave( apr_socket_t *sock, apr_sockaddr_t **sa )
{
  return APR_SUCCESS;
}
