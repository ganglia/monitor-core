#include <stdlib.h>
#include <string.h>

#include "interface.h"

#ifdef MINGW
int
get_min_mtu( void )
{
  DWORD ret, dwInterface, dwSize = 0;
  PMIB_IFTABLE ifTable;
  PMIB_IFROW ifRow;
  unsigned min_mtu = 0;

  dwSize = sizeof(MIB_IFTABLE);
  ifTable = (PMIB_IFTABLE)malloc(dwSize);
  while ((ret = GetIfTable(ifTable, &dwSize, 1)) == ERROR_INSUFFICIENT_BUFFER) {
    ifTable = (PMIB_IFTABLE)realloc(ifTable, dwSize);
  }

  if (ret = NO_ERROR) {
    for (dwInterface = 0; dwInterface < (ifTable -> dwNumEntries); dwInterface++) {
      ifRow = &(ifTable -> table[dwInterface]);

      if ((ifRow -> dwType != MIB_IF_TYPE_LOOPBACK) && (ifRow -> dwOperStatus ==MIB_IF_OPER_STATUS_OPERATIONAL)) {
        if (min_mtu) {
          if (ifRow -> dwMtu < min_mtu) {
            min_mtu = ifRow -> dwMtu;
          }
        } else {
          min_mtu = ifRow -> dwMtu;
        }
      }
    }
    free(ifTable);
  }
  return min_mtu;
}
#else
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
#endif
