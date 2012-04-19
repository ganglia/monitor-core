#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef MINGW

#ifdef HAVE_SYS_SOCKIO_H
/* For older versions of Solaris... */
#include <sys/sockio.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <gm_msg.h>

#include "unpifi.h"

/* HP-UX, Solaris */
#if !defined(ifr_mtu) && defined(ifr_metric)
# define ifr_mtu        ifr_metric
#endif

#define Calloc calloc
#define Malloc malloc

int
Socket(int family, int type, int protocol)
{
  int             n;

  if ( (n = socket(family, type, protocol)) < 0)
    err_sys("socket error");
  return(n);
}

int
Ioctl(int fd, unsigned long request, void *arg)
{
  int             n;

  if ( (n = ioctl(fd, request, arg)) == -1)
    err_sys("ioctl error");
  return(n);      /* streamio of I_LIST returns value */
}

struct ifi_info *
get_ifi_info(int family, int doaliases)
{
	struct ifi_info		*ifi, *ifihead, **ifipnext;
	int					sockfd, len, lastlen, flags, myflags;
	char				*ptr, *buf, lastname[IFNAMSIZ], *cptr;
	struct ifconf		ifc;
	struct ifreq		*ifr, ifrcopy;
	struct sockaddr_in	*sinptr;
        struct ifreq            mtu;
#ifdef SOLARIS
	int _c_virt = 0;
#endif /* SOLARIS */
	int _all_virt = 0;

	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1) {
		err_ret("get_ifi_info error: socket returns -1");
		return NULL;
	}

	lastlen = 0;
	len = 100 * sizeof(struct ifreq);	/* initial buffer size guess */
	for ( ; ; ) {
		buf = Malloc(len);
		ifc.ifc_len = len;
		ifc.ifc_buf = buf;
		if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) {
			if (errno != EINVAL || lastlen != 0)
				err_sys("ioctl error");
		} else {
			if (ifc.ifc_len == lastlen)
				break;		/* success, len has not changed */
			lastlen = ifc.ifc_len;
		}
		len += 10 * sizeof(struct ifreq);	/* increment */
		free(buf);
	}
	ifihead = NULL;
	ifipnext = &ifihead;
	lastname[0] = 0;
/* end get_ifi_info1 */

#ifdef SOLARIS
	/* On a Solaris zone/container (non-global zone), all
	   the interfaces are virtual interfaces.  This code attempts
	   to detect such cases and handle them differently.
	   Without this, Ganglia refuses to start in a Solaris 10 zone.
	   http://bugzilla.ganglia.info/cgi-bin/bugzilla/show_bug.cgi?id=100

           This code ONLY attempts to change the way NICs are evaluated
	   if and only if:
	    a) it is Solaris
            b) ALL interfaces appear to be virtual (with a colon in the names)
	*/
	for (ptr = buf; ptr < buf + ifc.ifc_len; ) {
		ifr = (struct ifreq *) ptr;
#ifdef  HAVE_SOCKADDR_SA_LEN
                len = max(sizeof(struct sockaddr), ifr->ifr_addr.sa_len);
#else
                switch (ifr->ifr_addr.sa_family) {
#ifdef  IPV6
                case AF_INET6:
                        len = sizeof(struct sockaddr_in6);
                        break;
#endif /* IPV6 */
                case AF_INET:
                default:
                        len = sizeof(struct sockaddr);
                        break;
                }
#endif  /* HAVE_SOCKADDR_SA_LEN */
                ptr += sizeof(ifr->ifr_name) + len;     /* for next one in buffer */
		if ( (cptr = strchr(ifr->ifr_name, ':')) != NULL)
			_c_virt ++;
	}
	if(_c_virt == ifc.ifc_len)
		_all_virt = 1;
#endif /* SOLARIS */
		

/* include get_ifi_info2 */
	for (ptr = buf; ptr < buf + ifc.ifc_len; ) {
		ifr = (struct ifreq *) ptr;

#ifdef	HAVE_SOCKADDR_SA_LEN
		len = max(sizeof(struct sockaddr), ifr->ifr_addr.sa_len);
#else
		switch (ifr->ifr_addr.sa_family) {
#ifdef	IPV6
		case AF_INET6:	
			len = sizeof(struct sockaddr_in6);
			break;
#endif
		case AF_INET:	
		default:	
			len = sizeof(struct sockaddr);
			break;
		}
#endif	/* HAVE_SOCKADDR_SA_LEN */
		ptr += sizeof(ifr->ifr_name) + len;	/* for next one in buffer */

		if (ifr->ifr_addr.sa_family != family)
			continue;	/* ignore if not desired address family */

		myflags = 0;
		if ( (cptr = strchr(ifr->ifr_name, ':')) != NULL &&
			(_all_virt == 0))
			*cptr = 0;		/* replace colon will null */
		if (strncmp(lastname, ifr->ifr_name, IFNAMSIZ) == 0) {
			if (doaliases == 0)
				continue;	/* already processed this interface */
			myflags = IFI_ALIAS;
		}
		memcpy(lastname, ifr->ifr_name, IFNAMSIZ);

		ifrcopy = *ifr;
		Ioctl(sockfd, SIOCGIFFLAGS, &ifrcopy);
		flags = ifrcopy.ifr_flags;
		if ((flags & IFF_UP) == 0)
			continue;	/* ignore if interface not up */

		ifi = Calloc(1, sizeof(struct ifi_info));
		*ifipnext = ifi;			/* prev points to this new one */
		ifipnext = &ifi->ifi_next;	/* pointer to next one goes here */

		ifi->ifi_flags = flags;		/* IFF_xxx values */
		ifi->ifi_myflags = myflags;	/* IFI_xxx values */
		memcpy(ifi->ifi_name, ifr->ifr_name, IFI_NAME);
		ifi->ifi_name[IFI_NAME-1] = '\0';

                /* Grab the MTU for this interface */
                memcpy( mtu.ifr_name , ifi->ifi_name, IFI_NAME);
#ifdef SIOCGIFMTU
                Ioctl(sockfd, SIOCGIFMTU, &mtu);
                ifi->ifi_mtu = mtu.ifr_mtu;
#else
                ifi->ifi_mtu = 1500;
#endif


/* end get_ifi_info2 */
/* include get_ifi_info3 */
		switch (ifr->ifr_addr.sa_family) {
		case AF_INET:
			sinptr = (struct sockaddr_in *) &ifr->ifr_addr;
			if (ifi->ifi_addr == NULL) {
				ifi->ifi_addr = Calloc(1, sizeof(struct sockaddr_in));
				memcpy(ifi->ifi_addr, sinptr, sizeof(struct sockaddr_in));

#ifdef	SIOCGIFBRDADDR
				if (flags & IFF_BROADCAST) {
					Ioctl(sockfd, SIOCGIFBRDADDR, &ifrcopy);
					sinptr = (struct sockaddr_in *) &ifrcopy.ifr_broadaddr;
					ifi->ifi_brdaddr = Calloc(1, sizeof(struct sockaddr_in));
					memcpy(ifi->ifi_brdaddr, sinptr, sizeof(struct sockaddr_in));
				}
#endif

#ifdef	SIOCGIFDSTADDR
				if (flags & IFF_POINTOPOINT) {
					Ioctl(sockfd, SIOCGIFDSTADDR, &ifrcopy);
					sinptr = (struct sockaddr_in *) &ifrcopy.ifr_dstaddr;
					ifi->ifi_dstaddr = Calloc(1, sizeof(struct sockaddr_in));
					memcpy(ifi->ifi_dstaddr, sinptr, sizeof(struct sockaddr_in));
				}
#endif
			}
			break;

		default:
			break;
		}
	}
	free(buf);
	close(sockfd);
	return(ifihead);	/* pointer to first structure in linked list */
}

void
free_ifi_info(struct ifi_info *ifihead)
{
	struct ifi_info	*ifi, *ifinext;

	for (ifi = ifihead; ifi != NULL; ifi = ifinext) {
		if (ifi->ifi_addr != NULL)
			free(ifi->ifi_addr);
		if (ifi->ifi_brdaddr != NULL)
			free(ifi->ifi_brdaddr);
		if (ifi->ifi_dstaddr != NULL)
			free(ifi->ifi_dstaddr);
		ifinext = ifi->ifi_next;	/* can't fetch ifi_next after free() */
		free(ifi);					/* the ifi_info{} itself */
	}
}

struct ifi_info *
Get_ifi_info(int family, int doaliases)
{
	struct ifi_info	*ifi;

	if ( (ifi = get_ifi_info(family, doaliases)) == NULL)
		err_quit("get_ifi_info error");
	return(ifi);
}

#endif
