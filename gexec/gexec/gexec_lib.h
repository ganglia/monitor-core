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
#ifndef __GEXEC_LIB_H
#define __GEXEC_LIB_H

#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pwd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <e/bitmask.h>
#include <e/tree.h>

typedef struct {
    int verbose;        /* Boolean (1 or 0) */
    int detached;       /* Boolean (1 or 0) */
    int port;           /* Integer (>= 1)   */
    int tree_fanout;    /* Integer (>= 1)   */
    int prefix_type;    /* See values below */
} gexec_spawn_opts;

/* Values for prefix_type */
#define GEXEC_NO_PREFIX             0
#define GEXEC_VNN_PREFIX            1
#define GEXEC_HOST_PREFIX           2
#define GEXEC_IP_PREFIX             3

/* Default values */
#define GEXEC_DEF_VERBOSE           0
#define GEXEC_DEF_PORT              2875
#define GEXEC_DEF_DETACHED          0
#define GEXEC_DEF_TREE_FANOUT       2
#define GEXEC_DEF_PREFIX_TYPE       GEXEC_VNN_PREFIX

/* Maximum number of proceses, multiple of BITMASK_CHUNK (currently 32 bits) */
#define GEXEC_MAX_NPROCS            (512 * BITMASK_CHUNK)
/* GEXEC bitmask in number of 32 bit chunks */
#define GEXEC_BITMASK_SIZE          (GEXEC_MAX_NPROCS / BITMASK_CHUNK)

/* Internal GPID (converted to string for user) */ 
typedef struct {
    struct in_addr addr;
    long           sec;
    long           usec;
} _gexec_gpid;

typedef struct {
    pthread_t tid;
} gexec_handle;

/* Client API functions */
void gexec_init();
int gexec_spawn(int nprocs, char **ips, int argc, char **argv, char **envp, 
                gexec_spawn_opts *opts, int *bad_nodes);
int gexec_spawn_async(int nprocs, char **ips, int argc, char **argv, char **envp, 
                      gexec_spawn_opts *opts, gexec_handle *h);
int gexec_wait(gexec_handle *h, int *bad_nodes);
void gexec_shutdown();

/* Remote Job API functions */
int   gexec_my_vnn();
int   gexec_nprocs();
char *gexec_gpid();
int   gexec_max_nprocs();

/* Return / error values */

#define GEXEC_OK                         0

#define GEXEC_NO_IMPL_ERROR             -1
#define GEXEC_INTERNAL_ERROR            -2
#define GEXEC_AUTH_ERROR                -3
#define GEXEC_NET_ERROR                 -4
#define GEXEC_IO_ERROR                  -5

#define GEXEC_PTHREADS_ERROR            -6
#define GEXEC_SETUID_ERROR              -7
#define GEXEC_SETGID_ERROR              -8
#define GEXEC_DUP2_ERROR                -9
#define GEXEC_PUTENV_ERROR             -10

#define GEXEC_PIPE_ERROR               -11 
#define GEXEC_WAITPID_ERROR            -12 
#define GEXEC_READ_ERROR               -13 
#define GEXEC_INITGROUPS_ERROR         -14 
#define GEXEC_CHDIR_ERROR              -15 

#define GEXEC_NO_HOSTS_ERROR           -16 

/* Miscellaneous constants */

#define GEXEC_CRED_LIFETIME             30

/* Miscellaneous macros */

#define VNN(__tree_index)   (__tree_index - 1)

#endif /* __GEXEC_LIB_H */
