/* Our own header for the programs that need interface configuration info.
   Include this file, instead of "unp.h". */

#ifndef	__unp_ifi_h
#define	__unp_ifi_h

#include <sys/types.h>
#include <sys/socket.h>

#ifdef AIX
#ifndef IP_MULTICAST
#define IP_MULTICAST
#endif
#endif
#include <net/if.h>
#ifdef AIX
#undef IP_MULTICAST
#endif

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define min(a,b)        ((a) < (b) ? (a) : (b))
#define max(a,b)        ((a) > (b) ? (a) : (b))

#define	IFI_NAME	16			/* same as IFNAMSIZ in <net/if.h> */
#define	IFI_HADDR	 8			/* allow for 64-bit EUI-64 in future */

struct ifi_info {
  char    ifi_name[IFI_NAME];	/* interface name, null terminated */
  u_char  ifi_haddr[IFI_HADDR];	/* hardware address */
  u_short ifi_hlen;				/* #bytes in hardware address: 0, 6, 8 */
  u_int   ifi_mtu;
  short   ifi_flags;			/* IFF_xxx constants from <net/if.h> */
  short   ifi_myflags;			/* our own IFI_xxx flags */
  struct sockaddr  *ifi_addr;	/* primary address */
  struct sockaddr  *ifi_brdaddr;/* broadcast address */
  struct sockaddr  *ifi_dstaddr;/* destination address */
  struct ifi_info  *ifi_next;	/* next of these structures */
};

#define	IFI_ALIAS	1			/* ifi_addr is an alias */

					/* function prototypes */
struct ifi_info	*get_ifi_info(int, int);
struct ifi_info	*Get_ifi_info(int, int);
void			 free_ifi_info(struct ifi_info *);

#endif	/* __unp_ifi_h */
