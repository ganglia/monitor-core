/********************************************************
 * An example source module to accompany...
 *
 * "Using POSIX Threads: Programming with Pthreads"
 *     by Brad nichols, Dick Buttlar, Jackie Farrell
 *     O'Reilly & Associates, Inc.
 *
 ********************************************************
 * rdwr.h --
 * 
 * Modified by Matt Massie April 2001
 *
 * Include file for reader/writer locks
 *
 */
#ifndef __RDWR_H
#define __RDWR_H

#include "pthread.h"

typedef struct rdwr_var
{
  int readers_reading;
  int writer_writing;
  pthread_mutex_t mutex;
  pthread_cond_t lock_free;
}
pthread_rdwr_t;

typedef void *pthread_rdwrattr_t;

int pthread_rdwr_init_np (pthread_rdwr_t * rdwrp);
int pthread_rdwr_rlock_np (pthread_rdwr_t * rdwrp);
int pthread_rdwr_runlock_np (pthread_rdwr_t * rdwrp);
int pthread_rdwr_wlock_np (pthread_rdwr_t * rdwrp);
int pthread_rdwr_wunlock_np (pthread_rdwr_t * rdwrp);

#endif /* __RDWR_H */
