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
#ifndef __GEXECD_H
#define __GEXECD_H

#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>
#include "gexecd_options.h"
#include "request.h"

typedef struct {

    pid_t           pid;
    pid_t           pgid;
    pthread_t       job_tid;
    pthread_t       route_down_tid;
    pthread_t       route_up_tid;
    pthread_t       stdout_tid;
    pthread_t       stderr_tid;
    pthread_t       heartbeat_tid;

    uid_t           uid;
    gid_t           gid;

    tree            *tree;
    int             self;
    int             parent;
    int             nchildren;
    int             *child_nodes;
    char            **child_ips;

    request         *req;
    int             rval;

    int             w_stdin;
    int             r_stdout;
    int             r_stderr;

    pthread_mutex_t up_sock_lock;
    int             up_sock;
    int             *child_socks;

    sem_t           jobstart_sem;
    sem_t           cleanup_sem;
    sem_t           stdout_sem;
    sem_t           stderr_sem;
    sem_t           killme_sem;

} gexecd_state;

typedef struct {
    int             up_sock;
    gexecd_options  *opts;
} gexecd_thr_args;

#define GEXECD_HEARTBEAT_INT    10

#define GEXECD_OK                0
#define GEXECD_NET_ERROR        -1

#endif /* __GEXECD_H */
