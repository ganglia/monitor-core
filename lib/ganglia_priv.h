#ifndef GANGLIA_PRIV_H
#define GANGLIA_PRIV_H 1

#include <errno.h>
#ifndef SYS_CALL
#define SYS_CALL(RC,SYSCALL) \
   do {                      \
       RC = SYSCALL;         \
   } while (RC < 0 && errno == EINTR);
#endif

#endif
