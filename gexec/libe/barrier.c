/*
 * "Copyright (c) 1999 by Brent N. Chun and The Regents of the University 
 * of California.  All rights reserved."
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include "barrier.h"
#include "e_error.h"
#include "xmalloc.h"

/* Called in response to thread cancellation or pthread_exit if on stack */
static void mutex_cleanup(void *arg)
{
    pthread_mutex_t *lock = (pthread_mutex_t *)arg;
    e_assert(pthread_mutex_unlock(lock) == 0);
}

void barrier_create(barrier **b, int nthrs)
{
    (*b) = (barrier *)xmalloc(sizeof(barrier));
    (*b)->nthrs   = nthrs;
    (*b)->waiting = 0;
    (*b)->phase   = 0;
    pthread_mutex_init(&(*b)->lock, NULL);
    pthread_cond_init(&(*b)->wait_cv, NULL);

}

void barrier_destroy(barrier *b)
{
    pthread_cond_destroy(&b->wait_cv);
    pthread_mutex_destroy(&b->lock);
    xfree(b);
}

void barrier_barrier(barrier *b)
{
    int my_phase;

    pthread_mutex_lock(&b->lock);
    pthread_cleanup_push(mutex_cleanup, (void *)&b->lock);
    my_phase = b->phase;
    b->waiting++;
    if (b->waiting == b->nthrs) {
        b->waiting = 0;
        b->phase = (my_phase == 0) ? 1 : 0; 
        pthread_cond_broadcast(&b->wait_cv);
    }
    while (b->phase == my_phase) 
        pthread_cond_wait(&b->wait_cv, &b->lock);
    pthread_cleanup_pop(0);
    pthread_mutex_unlock(&b->lock);
}
