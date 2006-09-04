#include <stdlib.h>

#include "interface.h"
#include "unpifi.h"


int
get_min_mtu( void )
{
  struct ifi_info *info, *n;
  int min_mtu_set = 0;
  int min_mtu = 0;

  info = Get_ifi_info(AF_INET, 0);

  for(n = info; n; n = n->ifi_next)
    {
      if(!min_mtu_set) 
        {
          min_mtu = n->ifi_mtu;
          min_mtu_set = 1;
        }
      else if( n->ifi_mtu < min_mtu )
        {
          min_mtu = n->ifi_mtu;
        }
    }

  free_ifi_info(info);
  return min_mtu;
}

struct ifi_info *
get_first_multicast_interface ( void )
{
   struct ifi_info *info, *n;

   info =  Get_ifi_info(AF_INET, 0);

   for(n = info; n; n = n->ifi_next)
    {
      /* The interface must be UP, not loopback and multicast enabled */
      if(! (n->ifi_flags & IFF_UP) )
        {
          continue;
        }

      if( n->ifi_flags & IFF_LOOPBACK )
        {
          continue;
        } 

      if(! (n->ifi_flags & IFF_MULTICAST) )
        {
          continue;
        }

      return n;
    }
   return NULL;
}

struct ifi_info *
get_first_interface ( void )
{
   return Get_ifi_info(AF_INET, 0); 
}

struct ifi_info *
get_interface ( char *name )
{
   struct ifi_info *info, *n;

   info =  Get_ifi_info(AF_INET, 0);

   for(n = info; n; n = n->ifi_next)
    {
       if(!strcmp( name, n->ifi_name))
         {
           return n;
         }
    }

   return NULL;
}
