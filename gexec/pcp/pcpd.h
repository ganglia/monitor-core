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
#ifndef __PCPD_H
#define __PCPD_H

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <e/tree.h>
#include "pcpd_options.h"

typedef struct {
    pthread_t       route_down_tid;
    pthread_t       route_up_tid;
    pthread_t       heartbeat_tid;

    uid_t           uid;
    gid_t           gid;

    tree            *tree;
    int             self;
    int             parent;
    int             nchildren;
    int             *child_nodes;
    char            **child_ips;

    pthread_mutex_t up_sock_lock;
    int             up_sock;
    int             *child_socks;

    sem_t           cleanup_sem;
    sem_t           killme_sem;

} pcpd_state;

typedef struct {
    int          up_sock;
    pcpd_options *opts;
} pcpd_thr_args;

#define PCPD_HEARTBEAT_INT  10

#endif /* __PCPD_H */
