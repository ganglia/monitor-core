/*
 * "Copyright (c) 1999 by Matt Massie and The Regents of the University 
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
#include <ganglia/barrier.h>

/*
 * barrier_init: allocates resources for a barrier for nthrs
 * threads. Returns REXEC_OK on success, returns error code on
 * error. If error occurs, no resources are allocated and all heap
 * allocated variables are set to NULL.  
 */
int barrier_init(barrier **b, int nthrs)
{
    if (((*b) = (barrier *)malloc(sizeof(barrier))) == NULL) {
        goto malloc_error;
    }
    if (pthread_mutex_init(&(*b)->lock, NULL) != 0) {
        goto mutex_init_error;
    }
    if (pthread_cond_init(&(*b)->wait_cv, NULL) != 0) {
        goto cond_init_error;
    }
    (*b)->nthrs = nthrs;
    (*b)->waiting = 0;
    (*b)->phase = 0;
    return 0;

 cond_init_error:
    pthread_mutex_destroy(&(*b)->lock);
 mutex_init_error:
    free(*b);
    (*b) = NULL;
 malloc_error:
    return 1;
}

/*
 * barrier_destroy: frees resources for a barrier previously allocated
 * with barrier_init.  
 */
void barrier_destroy(barrier *b)
{
    pthread_cond_destroy(&b->wait_cv);
    pthread_mutex_destroy(&b->lock);
    free(b);
}

/*
 * barrier_barrier: performs a barrier. Each thread blocks until
 * b->nthrs threads have reached the barrier.  
*/
void barrier_barrier(barrier *b)
{
    int my_phase;

    pthread_mutex_lock(&b->lock);
    my_phase = b->phase;
    b->waiting++;
    if (b->waiting == b->nthrs) {
        b->waiting = 0;
        b->phase = (my_phase == 0) ? 1 : 0; 
        pthread_cond_broadcast(&b->wait_cv);
    }
    while (b->phase == my_phase) 
        pthread_cond_wait(&b->wait_cv, &b->lock);
    pthread_mutex_unlock(&b->lock);
}
