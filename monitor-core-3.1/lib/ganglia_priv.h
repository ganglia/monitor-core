#ifndef GANGLIA_PRIV_H
#define GANGLIA_PRIV_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#ifdef TIME_WITH_SYS_TIME
#include <time.h>
#endif
#endif

#include <gm_msg.h>

#include <errno.h>
#ifndef SYS_CALL
#define SYS_CALL(RC,SYSCALL) \
   do {                      \
       RC = SYSCALL;         \
   } while (RC < 0 && errno == EINTR);
#endif

#define GANGLIA_DEFAULT_MCAST_CHANNEL "239.2.11.71"
#define GANGLIA_DEFAULT_MCAST_PORT    8649
#define GANGLIA_DEFAULT_XML_PORT      8649

#ifndef SYNAPSE_FAILURE
#define SYNAPSE_FAILURE -1
#endif
#ifndef SYNAPSE_SUCCESS
#define SYNAPSE_SUCCESS 0
#endif

char *Ganglia_default_collection_groups(void);

#endif
