#ifndef INTERFACE_H
#define INTERFACE_H 1

#include "config.h"

#ifdef MINGW
#include <windows.h>
#include <iphlpapi.h>
#else
#include "unpifi.h"
#endif

int get_min_mtu( void );
#endif
