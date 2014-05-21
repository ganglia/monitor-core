/*
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
#include <unistd.h>
#include <netdb.h>

#include <gm_msg.h>
#include "net.h"

g_tcp_socket*
g_tcp_socket_connect (const char* hostname, int port)
{
  g_inet_addr* ia;
  g_tcp_socket* socket;

  ia = g_inetaddr_new(hostname, port);
  if (ia == NULL)
    return NULL;

  socket = g_tcp_socket_new(ia);
  g_inetaddr_delete(ia);

  return socket;
}

g_tcp_socket*
g_tcp_socket_new (const g_inet_addr* addr)
{
  int          sockfd;
  g_tcp_socket*     s;
  struct sockaddr_in*   sa_in;
  int       rv;

  if( addr == NULL )
     return NULL;

  /* Create socket */
  sockfd = socket (AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    return NULL;

  s = malloc( sizeof( g_tcp_socket ) );
  memset( s, 0, sizeof( g_tcp_socket ));
  s->sockfd = sockfd;
  s->ref_count = 1;

  /* Set up address and port for connection */
  memcpy(&s->sa, &addr->sa, sizeof(s->sa));
  sa_in = (struct sockaddr_in*) &s->sa;
  sa_in->sin_family = AF_INET;

  /* Connect */
  rv = connect(sockfd, &s->sa, sizeof(s->sa));
  if (rv != 0)
    {
      close (sockfd);
      free (s);
      return NULL;
    }

  return s;
}

g_tcp_socket*
g_tcp6_socket_new (const g_inet6_addr* addr)
{
  g_tcp_socket* s;
  struct sockaddr* tsa;
  struct addrinfo *result, *rp;
  int sfd;
  
  for (rp = addr->ai; rp != NULL; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == -1)
      continue;
    if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
      tsa = rp->ai_addr;
      break; //Success
    }
    close(sfd);
  }
  if (rp == NULL)
    return NULL;
  s = malloc(sizeof( g_tcp_socket ));
  if (s == NULL) {
    close(sfd);
    return NULL;
  }
  memset( s, 0, sizeof( g_tcp_socket ));
  s->sockfd = sfd;
  s->ref_count = 1;
  memcpy(&s->sa, tsa, sizeof(s->sa));

  return s;
}

#if 0
static void
g_tcp_socket_ref(g_tcp_socket* s)
{
  if( s==NULL)
     return;

  ++s->ref_count;
}
#endif

static void
g_tcp_socket_unref(g_tcp_socket* s)
{
  if( s == NULL )
     return;

  --s->ref_count;

  if (s->ref_count == 0)
    {
       close(s->sockfd);
       free(s);
    }
}

void
g_tcp_socket_delete(g_tcp_socket* s)
{
  if (s != NULL)
    g_tcp_socket_unref(s);
}

g_tcp_socket*
g_tcp_socket_server_new (int port)
{
  g_inet_addr iface;
  struct sockaddr_in* sa_in;

  /* Set up address and port (any address, any port) */
  memset (&iface, 0, sizeof(iface));
  sa_in = (struct sockaddr_in*) &iface.sa;
  sa_in->sin_family = AF_INET;
  sa_in->sin_addr.s_addr = htonl(INADDR_ANY);
  sa_in->sin_port = htons(port);

  return g_tcp_socket_server_new_interface (&iface);
}


g_tcp_socket* 
g_tcp_socket_server_new_interface (const g_inet_addr* iface)
{
  g_tcp_socket* s;
  struct sockaddr_in* sa_in;
  socklen_t socklen;
  int rval;
  const int on = 1;

  /* Create socket */
  s = malloc( sizeof(g_tcp_socket));
  memset(s, 0, sizeof(g_tcp_socket));

  s->ref_count = 1;
  s->sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (s->sockfd < 0)
    goto error;

  /* Set up address and port for connection */
  sa_in = (struct sockaddr_in*) &s->sa;
  sa_in->sin_family = AF_INET;
  if (iface)
    {
      sa_in->sin_addr.s_addr = G_SOCKADDR_IN(iface->sa).sin_addr.s_addr;
      sa_in->sin_port = G_SOCKADDR_IN(iface->sa).sin_port;
    }
  else
    {
      sa_in->sin_addr.s_addr = htonl(INADDR_ANY);
      sa_in->sin_port = 0;
    }

  /* Set REUSEADDR so we can reuse the port */
  rval = setsockopt(s->sockfd, SOL_SOCKET, SO_REUSEADDR, (void*) &on, sizeof(on));
  if ( rval <0)
     {
        err_ret("tcp_listen() setsockopt() SO_REUSEADDR error");
        goto error;
     }

  rval = setsockopt(s->sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&on, sizeof(on));
  if ( rval <0)
     {
        err_ret("tcp_listen() setsockopt() SO_KEEPALIVE error");
        goto error;
     }
  rval = setsockopt(s->sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&on, sizeof(on));
  if ( rval <0)
     {
        err_ret("tcp_listen() setsockopt() TCP_NODELAY error");
        goto error;
     }

  /* Bind */
  if (bind(s->sockfd, &s->sa, sizeof(s->sa)) != 0)
    goto error;
  
  /* Get the socket name */
  socklen = sizeof(s->sa);
  if (getsockname(s->sockfd, &s->sa, &socklen) != 0)
    goto error;
  
  /* Listen */
  if (listen(s->sockfd, 10) != 0)
    goto error;
  
  return s;
  
 error:
  if (s) free(s);
  return NULL;
}

g_tcp_socket *
g_tcp_socket_server_accept (g_tcp_socket* socket)
{
  int sockfd;
  struct sockaddr sa;
  socklen_t n;
  fd_set fdset;
  g_tcp_socket* s;

  if( socket == NULL )
     return NULL;

 try_again:

  FD_ZERO(&fdset);
  FD_SET(socket->sockfd, &fdset);

  if (select(socket->sockfd + 1, &fdset, NULL, NULL, NULL) == -1)
    {
      if (errno == EINTR)
   goto try_again;

      return NULL;
    }

  n = sizeof(s->sa);

  if ((sockfd = accept(socket->sockfd, &sa, &n)) == -1)
    {
      if (errno == EWOULDBLOCK ||
     errno == ECONNABORTED ||
#ifdef EPROTO     /* OpenBSD does not have EPROTO */
     errno == EPROTO ||
#endif
     errno == EINTR)
   goto try_again;

      return NULL;
    }

  s = malloc( sizeof(g_tcp_socket) );
  memset( s, 0, sizeof(g_tcp_socket));
  s->ref_count = 1;
  s->sockfd = sockfd;
  memcpy(&s->sa, &sa, sizeof(s->sa));

  return s;
}


