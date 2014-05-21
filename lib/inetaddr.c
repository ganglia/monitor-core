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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#include "llist.h"
#include "net.h"

pthread_mutex_t gethostbyname_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Need to free "nicename" later */
int
g_gethostbyname(const char* hostname, struct sockaddr_in* sa, char** nicename)
{
  int rv = 0;
  struct in_addr inaddr;
  struct hostent* he;  

  if (inet_aton(hostname, &inaddr) != 0)
    {
      sa->sin_family = AF_INET;
      memcpy(&sa->sin_addr, (char*) &inaddr, sizeof(struct in_addr));
      if (nicename)
         *nicename = (char *)strdup (hostname);
      return 1;
    }

  pthread_mutex_lock (&gethostbyname_mutex);
  he = (struct hostent*)gethostbyname(hostname);
  if (he && he->h_addrtype==AF_INET && he->h_addr_list[0])
    {
     if (sa)
        {
           sa->sin_family = he->h_addrtype;
           memcpy(&sa->sin_addr, he->h_addr_list[0], he->h_length);
        }

      if (nicename && he->h_name)
         *nicename = (char *)strdup(he->h_name);

      rv = 1;
    }
  pthread_mutex_unlock(&gethostbyname_mutex);

  return rv;
}

int g_getaddrinfo(const char* hostname, const char* service, g_inet6_addr* ia, char** nicename)
{
  int rv = 0;
  struct addrinfo hints;
  struct addrinfo *result;
  int s;
  
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol = 0;          /* Any protocol */
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;
  
  s = getaddrinfo(hostname, service, &hints, &result);
  if (s != 0) {
    return s;
  }
  else {
    ia->name = (char *)strdup(hostname);
    ia->ai = result;
    ia->ref_count = 1;
  }
  return s;
}

/* Need to free return value later */
char*
g_gethostbyaddr(const char* addr, size_t length, int type)
{
  char* rv = NULL;
  struct hostent* he;

  pthread_mutex_lock (&gethostbyname_mutex);
  he = gethostbyaddr(addr, length, type);
  if (he != NULL && he->h_name != NULL)
     rv = (char *)strdup(he->h_name);
  pthread_mutex_unlock(&gethostbyname_mutex);
  return rv;
}

g_inet_addr*
g_inetaddr_new (const char* name, int port)
{
  struct sockaddr_in* sa_in;
  struct in_addr inaddr;
  g_inet_addr* ia = NULL;

  if(name==NULL)
     return NULL;

  ia = (g_inet_addr *)malloc(sizeof(g_inet_addr));
  if(ia==NULL)
     return NULL;
  memset(ia, 0, sizeof(g_inet_addr));

  ia->name = (char *)strdup(name);
  ia->ref_count = 1;

  /* Try to read the name as if were dotted decimal */
  if (inet_aton(name, &inaddr) != 0)
    {
      sa_in = (struct sockaddr_in*) &ia->sa;
      sa_in->sin_family = AF_INET;
      sa_in->sin_port = htons(port);
      memcpy(&sa_in->sin_addr, (char*) &inaddr, sizeof(struct in_addr));
    }
  else
    {
      struct sockaddr_in sa;

      /* Try to get the host by name (ie, DNS) */
      if (g_gethostbyname(name, &sa, NULL))
        {
          sa_in = (struct sockaddr_in*) &ia->sa;
          sa_in->sin_family = AF_INET;
          sa_in->sin_port = htons(port);
          memcpy(&sa_in->sin_addr, &sa.sin_addr, 4);
        }
    }

  return ia;
}

void
g_inetaddr_ref (g_inet_addr* ia)
{
  if(ia==NULL)
     return;

  ++ia->ref_count;
}

void
g_inetaddr_unref (g_inet_addr* ia)
{
  if(ia==NULL)
     return;

  --ia->ref_count;

  if (ia->ref_count == 0)
    {
      if (ia->name != NULL)
        free (ia->name);
      free (ia);
    }
}

void
g_inetaddr_delete (g_inet_addr* ia)
{
  if (ia != NULL)
    g_inetaddr_unref(ia);
}

int
g_inetaddr_get_port(const g_inet_addr* ia)
{
  if(ia==NULL)
     return -1;

  return (int) ntohs(G_SOCKADDR_IN(ia->sa).sin_port);
}

void
g_inetaddr_set_port(const g_inet_addr* ia, unsigned int port)
{
  if(ia == NULL)
     return;

  G_SOCKADDR_IN(ia->sa).sin_port = htons(port);
}


int
g_inetaddr_is_multicast (const g_inet_addr* inetaddr)
{
  unsigned int addr;

  if( inetaddr == NULL )
     return 0;

  addr = G_SOCKADDR_IN(inetaddr->sa).sin_addr.s_addr;
  addr = htonl(addr);

  if ((addr & 0xF0000000) == 0xE0000000)
    return 1;

  return 0;
}

g_inet_addr*
g_inetaddr_get_interface_to (const g_inet_addr* addr)
{
  int sockfd;
  struct sockaddr_in myaddr;
  socklen_t len;
  g_inet_addr* iface;

  if(!addr)
     return NULL;

  sockfd = socket (AF_INET, SOCK_DGRAM, 0);
  if (sockfd == -1)
    return NULL;

  if (connect (sockfd, &addr->sa, sizeof(addr->sa)) == -1)
    {
      close(sockfd);
      return NULL;
    }

  len = sizeof (myaddr);
  if (getsockname (sockfd, (struct sockaddr*) &myaddr, &len) != 0)
    {
      close(sockfd);
      return NULL;
    }

  iface = (g_inet_addr *)malloc(sizeof(g_inet_addr));
  if(!iface)
     {
       close(sockfd);
       return NULL;
     }
  iface->ref_count = 1;
  memcpy (&iface->sa, (char*) &myaddr, sizeof (struct sockaddr_in));

  return iface;
}

llist_entry *
g_inetaddr_list_interfaces (void)
{
  llist_entry *list = NULL;
  llist_entry *e;
  int len, lastlen;
  char* buf;
  char* ptr;
  int sockfd;
  struct ifconf ifc;

  /* Create a dummy socket */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd == -1) return NULL;

  len = 8 * sizeof(struct ifreq);
  lastlen = 0;

  /* Get the list of interfaces.  We might have to try multiple times
     if there are a lot of interfaces. */
  while(1)
    {
      buf = (char *) malloc (len);
      memset( buf, 0, len );

      ifc.ifc_len = len;
      ifc.ifc_buf = buf;
      if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0)
        {
          /* Might have failed because our buffer was too small */
          if (errno != EINVAL || lastlen != 0)
            {
              free(buf);
              return NULL;
            }
        }
      else
        {
          /* Break if we got all the interfaces */
          if (ifc.ifc_len == lastlen)
            break;

          lastlen = ifc.ifc_len;
        }

      /* Did not allocate big enough buffer - try again */
      len += 8 * sizeof(struct ifreq);
      free(buf);
    }


  /* Create the list.  Stevens has a much more complex way of doing
     this, but his is probably much more correct portable.  */
  for (ptr = buf; ptr < (buf + ifc.ifc_len); )
    {
      struct ifreq* ifr = (struct ifreq*) ptr;
      struct sockaddr addr;
      g_inet_addr* ia;

#ifdef HAVE_SOCKADDR_SA_LEN
      ptr += sizeof(ifr->ifr_name) + ifr->ifr_addr.sa_len;
#else
      ptr += sizeof(struct ifreq);
#endif

      /* Ignore non-AF_INET */
      if (ifr->ifr_addr.sa_family != AF_INET)
        continue;

      /* FIX: Skip colons in name?  Can happen if aliases, maybe. */

      /* Save the address - the next call will clobber it */
      memcpy(&addr, &ifr->ifr_addr, sizeof(addr));

      /* Get the flags */
      ioctl(sockfd, SIOCGIFFLAGS, ifr);

      /* Ignore entries that aren't up or loopback.  Someday we'll
         write an interface structure and include this stuff. */
      if (!(ifr->ifr_flags & IFF_UP) || (ifr->ifr_flags & IFF_LOOPBACK))
         continue;

      /* Create an InetAddr for this one and add it to our list */
      ia = malloc( sizeof(g_inet_addr) );
      memset( ia, 0, sizeof(g_inet_addr) );
      ia->ref_count = 1;
      memcpy(&ia->sa, &addr, sizeof(addr));

      e = malloc(sizeof(llist_entry));
      if(!e)
         {
            free(ia);
            free(buf);
            return NULL;
         } 
   
      e->val = ia;
 
      llist_add( &list, e);
    }

  free (buf);
  return list;
}




