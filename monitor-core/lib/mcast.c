/*
 * $Id$
 * This code was originally written by the authors below but
 * I rewrote it and took out all the glib dependencies.
 *
 * GNet - Networking library
 * Copyright (C) 2000  David Helder
 * Copyright (C) 2000  Andrew Lanoix
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */
#include <ganglia/net.h>

g_mcast_socket*
g_mcast_socket_new (const g_inet_addr* ia)
{
  g_mcast_socket* ms;
  const int on = 1;

  ms = (g_mcast_socket *)malloc(sizeof(g_mcast_socket));
  if(!ms)
     return NULL;

  memset(ms, 0, sizeof(g_mcast_socket));

  /* Create socket */
  ms->ref_count = 1;
  ms->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (ms->sockfd < 0)
     {
        free(ms);
        return NULL;
     }

  /* Copy address */
  ms->sa = ia->sa;

  /* Set socket option to share the UDP port */
  if (setsockopt(ms->sockfd, SOL_SOCKET, SO_REUSEADDR,
                     (void*) &on, sizeof(on)) != 0)
    {
       /* error */
       /* g_warning("Can't reuse mcast socket\n");*/
       return NULL;
    }

  /* Bind to the socket to some local address and port */
  if (bind(ms->sockfd, &ms->sa, sizeof(ms->sa)) != 0)
    {
       /* error */
       return NULL;
    }

  return ms;
}

static void
g_mcast_socket_unref(g_mcast_socket* s)
{
  if(s==NULL)
     return;

  --s->ref_count;

  if (s->ref_count == 0)
    {
      close(s->sockfd);
      free(s);
    }
}

void
g_mcast_socket_delete(g_mcast_socket* ms)
{
  if (ms != NULL)
    g_mcast_socket_unref(ms);
}

static void
g_mcast_socket_ref(g_mcast_socket* s)
{
  if(s==NULL)
     return;

  ++s->ref_count;
}

int
g_mcast_socket_connect ( g_mcast_socket *ms )
{
   return connect ( ms->sockfd, (struct sockaddr *)&(ms->sa), sizeof(struct sockaddr_in) );
}

int
g_mcast_socket_join_group (g_mcast_socket* ms, const g_inet_addr* ia)
{
  struct ip_mreq mreq;

  /* Create the multicast request structure */
  memcpy(&mreq.imr_multiaddr,
         &((struct sockaddr_in*) &ia->sa)->sin_addr,
         sizeof(struct in_addr));
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);

  /* Join the group */
  return setsockopt(ms->sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                    (void*) &mreq, sizeof(mreq));
}

int
g_mcast_socket_leave_group (g_mcast_socket* ms, const g_inet_addr* ia)
{
  struct ip_mreq mreq;

  /* Create the multicast request structure */
  memcpy(&mreq.imr_multiaddr,
         &((struct sockaddr_in*) &ia->sa)->sin_addr,
         sizeof(struct in_addr));
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);

  /* Leave the group */
  return setsockopt(ms->sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                    (void*) &mreq, sizeof(mreq));
}

int
g_mcast_socket_is_loopback (const g_mcast_socket* ms)
{
  unsigned char flag;
  socklen_t flagSize;

  flagSize = sizeof(flag);

  if (getsockopt(ms->sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &flag, &flagSize) < 0)
    return(-1);

  if(! (flagSize <= sizeof(flag) ))
     exit(-1);

  return (int) flag;
}

int
g_mcast_socket_set_loopback (g_mcast_socket* ms, int b)
{
  unsigned char flag;

  flag = (unsigned char) b;

  return setsockopt(ms->sockfd, IPPROTO_IP, IP_MULTICAST_LOOP,
                    &flag, sizeof(flag));
}

int
g_mcast_socket_get_ttl (const g_mcast_socket* ms)
{
  unsigned char ttl;
  socklen_t ttlSize;

  ttlSize = sizeof(ttl);

  if (getsockopt(ms->sockfd, IPPROTO_IP, IP_MULTICAST_TTL,
                 (void*) &ttl, &ttlSize) < 0)
    return(-1);

  if(! (ttlSize <= sizeof(ttl)))
     exit(-1);

  return(ttl);
}

int
g_mcast_socket_set_ttl(g_mcast_socket* ms, int val)
{
  unsigned char ttl;

  ttl = (unsigned char) val;
  return setsockopt(ms->sockfd, IPPROTO_IP, IP_MULTICAST_TTL, (void*) &ttl, sizeof(ttl));
}
