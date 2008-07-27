#include <stdlib.h>
#include <string.h>

#include "interface.h"

int
get_min_mtu( void )
{
  struct ifi_info *info, *n;
  unsigned min_mtu_set = 0;
  unsigned min_mtu = 0;

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
