#ifndef INTERFACE_H
#define INTERFACE_H 1
#include "unpifi.h"
struct ifi_info * get_first_multicast_interface ( void );
struct ifi_info * get_first_interface ( void );
struct ifi_info * get_interface ( char *name );
void get_all_interfaces( void );
int get_min_mtu( void );
#endif
