/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */
#ifndef __GEXEC_STATE_H
#define __GEXEC_STATE_H

#include <pthread.h>
#include <semaphore.h>
#include <e/barrier.h>
#include <e/tree.h>
#include "gexec_lib.h"
#include "gexec_options.h"
#include "request.h"

typedef struct {
    gexec_spawn_opts *opts;         /* Pointer to user argument */

    pthread_t        stdin_tid;
    pthread_t        signals_tid;
    pthread_t        recv_tid;

    tree             *tree;
    int              self;
    int              nchildren;
    int              *child_nodes;
    char             **child_ips;   /* Holds pointers to IPs in tree */
    char             **hosts;       /* Holds malloc'ed hostnames     */
    int              *bad_nodes;    /* Pointer to user argument      */

    int              *child_socks;
    pthread_mutex_t  *child_locks;
    
    request          *req;
    int              rval;

    int              ndone;
    sem_t            cleanup_sem;
    sem_t            killme_sem;
} gexec_state;

#endif /* __GEXEC_STATE_H */
