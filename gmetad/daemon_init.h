#ifndef DAEMON_INIT_H
#define DAEMON_INIT_H 1

#include <sys/types.h>
#include <sys/stat.h>

void daemon_init (const char *pname, int facility, mode_t rrd_umask);

#endif
