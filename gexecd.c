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
#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <getopt.h>
#include <grp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include <auth.h>
#include <e/barrier.h>
#include <e/bitmask.h>
#include <e/bytes.h>
#include <e/e_error.h>
#include <e/io.h>
#include <e/net.h>
#include <e/tree.h>
#include <e/xmalloc.h>
#include "gexec_lib.h"
#include "gexecd.h"
#include "gexecd_options.h"
#include "msg.h"
#include "request.h"

static void gexecd_state_create(gexecd_state **s)
{
    (*s) = (gexecd_state *)xmalloc(sizeof(gexecd_state));
    (*s)->pid            = -1;
    (*s)->pgid           = -1;
    (*s)->job_tid        = -1;
    (*s)->route_down_tid = -1;
    (*s)->route_up_tid   = -1;
    (*s)->stdout_tid     = -1;
    (*s)->stderr_tid     = -1;
    (*s)->heartbeat_tid  = -1; 

    (*s)->uid   = -1;
    (*s)->gid   = -1;

    (*s)->tree        = NULL;
    (*s)->self        = -1;
    (*s)->parent      = -1;
    (*s)->nchildren   = 0;
    (*s)->child_nodes = NULL;
    (*s)->child_ips   = NULL;

    (*s)->req         = NULL;  
    (*s)->rval        = GEXEC_OK;

    (*s)->w_stdin   = -1;
    (*s)->r_stdout  = -1;
    (*s)->r_stderr  = -1;

    if (pthread_mutex_init(&(*s)->up_sock_lock, NULL) != 0) {
        fatal_error("pthread_mutex_init error\n");
        exit(1);
    }
    (*s)->up_sock     = -1;
    (*s)->child_socks = NULL; 

    if (sem_init(&(*s)->jobstart_sem, 0, 0) != 0) {
        fatal_error("sem_init error\n");
        exit(1);
    }
    if (sem_init(&(*s)->cleanup_sem, 0, 0) != 0) {
        fatal_error("sem_init error\n");
        exit(1);
    }
    if (sem_init(&(*s)->stdout_sem, 0, 0) != 0) {
        fatal_error("sem_init error\n");
        exit(1);
    }
    if (sem_init(&(*s)->stderr_sem, 0, 0) != 0) {
        fatal_error("sem_init error\n");
        exit(1);
    }
    if (sem_init(&(*s)->killme_sem, 0, 0) != 0) {
        fatal_error("sem_init error\n");
        exit(1);
    }
}

static void gexecd_state_destroy(gexecd_state *s)
{
    int i;

    if (s->pgid != -1)
        killpg(s->pgid, SIGKILL);
    if (s->job_tid != -1) {
        e_assert(pthread_cancel(s->job_tid) == 0);
        e_assert(pthread_join(s->job_tid, NULL) == 0);
    }
    if (s->route_down_tid != -1) {
        e_assert(pthread_cancel(s->route_down_tid) == 0);
        e_assert(pthread_join(s->route_down_tid, NULL) == 0);
    }
    if (s->route_up_tid != -1) {
        e_assert(pthread_cancel(s->route_up_tid) == 0);
        e_assert(pthread_join(s->route_up_tid, NULL) == 0);
    }
    if (s->stdout_tid != -1) {
        e_assert(pthread_cancel(s->stdout_tid) == 0);
        e_assert(pthread_join(s->stdout_tid, NULL) == 0);
    }
    if (s->stderr_tid != -1) {
        e_assert(pthread_cancel(s->stderr_tid) == 0);
        e_assert(pthread_join(s->stderr_tid, NULL) == 0);
    }
    if (s->heartbeat_tid != -1) {
        e_assert(pthread_cancel(s->heartbeat_tid) == 0);
        e_assert(pthread_join(s->heartbeat_tid, NULL) == 0);
    }

    if (s->tree != NULL)
        tree_destroy(s->tree);
    if (s->child_nodes != NULL)
        xfree(s->child_nodes);
    if (s->child_ips != NULL)
        xfree(s->child_ips);

    if (s->req != NULL)
        request_destroy(s->req);

    if (s->w_stdin != -1)
        close(s->w_stdin);
    if (s->r_stdout != -1)
        close(s->r_stdout);
    if (s->r_stderr != -1)
        close(s->r_stderr);

    e_assert(pthread_mutex_destroy(&s->up_sock_lock) == 0);
    if (s->up_sock != -1)
        close(s->up_sock);
    if (s->child_socks != NULL) {
        for (i = 0; i < s->nchildren; i++)
            if (s->child_socks[i] != -1)
                close(s->child_socks[i]);
        xfree(s->child_socks);
    }

    sem_destroy(&s->jobstart_sem);
    sem_destroy(&s->cleanup_sem);
    sem_destroy(&s->stdout_sem);
    sem_destroy(&s->stderr_sem);
    sem_destroy(&s->killme_sem);
    xfree(s);
}

/* Called for thread cancellation or pthread_exit if on stack */
static void mutex_cleanup(void *arg)
{
    pthread_mutex_t *lock = (pthread_mutex_t *)arg;
    e_assert(pthread_mutex_unlock(lock) == 0);
}

static int gexecd_authenticate(credentials *creds, signature *creds_sig,
                             uid_t *uid, gid_t *gid)
{
    if (auth_verify_signature(creds, creds_sig) != AUTH_OK)
        return GEXEC_AUTH_ERROR;
    *uid = creds->uid;
    *gid = creds->gid;
    return GEXEC_OK;
}

static int locked_msg_send(int sock, pthread_mutex_t *lock, 
                           gexec_hdr *hdr, void *bdy)
{
    int rval1, rval2;

    pthread_mutex_lock(lock);
    pthread_cleanup_push(mutex_cleanup, (void *)lock);
    rval1 = net_send_bytes(sock, (void *)hdr, sizeof(gexec_hdr));
    rval2 = net_send_bytes(sock, (void *)bdy, hdr->bdy_len);
    pthread_cleanup_pop(0);
    pthread_mutex_unlock(lock);

    if (rval1 != E_OK || rval2 != E_OK)
        return GEXEC_NET_ERROR;
    return GEXEC_OK;
}

static int send_tree_create(int sock, gexec_hdr *hdr, credentials *creds,
                            signature *creds_sig, void **tbuf, int tbuf_len)
{ 
    e_assert(sock != -1);
    if (net_send_bytes(sock, (void *)hdr, sizeof(gexec_hdr)) != E_OK)
        return GEXEC_NET_ERROR;
    if (net_send_bytes(sock, (void *)creds, sizeof(credentials)) != E_OK)
        return GEXEC_NET_ERROR;
    if (net_send_bytes(sock, (void *)creds_sig, sizeof(signature)) != E_OK)
        return GEXEC_NET_ERROR;
    if (net_send_bytes(sock, tbuf, tbuf_len) != E_OK)
        return GEXEC_NET_ERROR;
    return GEXEC_OK;
}

static int send_tc_reply(int sock, pthread_mutex_t *lock, int src, 
                         int dst, int rval)
{
    gexec_hdr           hdr;
    gexec_tc_reply_bdy  bdy;
 
    GEXEC_HDR_BUILD(&hdr, GEXEC_TREE_CREATE_REPLY_MSG, src, dst, 
                    sizeof(gexec_tc_reply_bdy));
    bdy.rval     = rval;
    return locked_msg_send(sock, lock, &hdr, &bdy);
}

static void send_tc_error(int sock, pthread_mutex_t *lock, int src, 
                          int dst, int rval, int *bad_nodes)
{
    gexec_hdr           hdr;
    gexec_tc_error_bdy  bdy;
 
    /* "Best effort" send */
    GEXEC_HDR_BUILD(&hdr, GEXEC_TREE_CREATE_ERROR_MSG, src, dst, 
                    sizeof(gexec_tc_error_bdy));
    bdy.rval     = rval;
    bitmask_reset(bdy.bad_nodes, GEXEC_MAX_NPROCS);
    bitmask_or(bdy.bad_nodes, bad_nodes, GEXEC_MAX_NPROCS);
    locked_msg_send(sock, lock, &hdr, &bdy);
}

/* Send error to parent and wait for parent to close socket to this node */
static void send_tc_error_and_wait(int up_sock, pthread_mutex_t *up_sock_lock,
                                   int src, int dst, int rval, int *bad_nodes)
{
    char bit_bucket[8192];

    send_tc_error(up_sock, up_sock_lock, src, dst, rval, bad_nodes);
    for (;;)
        if (net_recv_bytes(up_sock, bit_bucket, 8192) != E_OK)
            break;
}

static int send_job_reply(int up_sock, pthread_mutex_t *up_sock_lock,
                          int src, int dst, int rval)
{
    gexec_hdr           hdr;
    gexec_job_reply_bdy bdy;

    GEXEC_HDR_BUILD(&hdr, GEXEC_JOB_REPLY_MSG, src, dst, sizeof(bdy));
    bdy.rval = rval;
    return locked_msg_send(up_sock, up_sock_lock, &hdr, &bdy);
}

static int forward_request(int up_sock, int *child_socks, int nchildren, 
                           gexec_hdr *hdr, uid_t uid, gid_t gid, request **req)
{
    void *buf;
    int  rval, i; 

    buf = xmalloc(hdr->bdy_len);

    /* Recv request buf */
    if (net_recv_bytes(up_sock, buf, hdr->bdy_len) != E_OK) {
        rval = GEXEC_NET_ERROR;
        goto cleanup;
    }

    /* Send hdr, request buf name to children */
    for (i = 0; i < nchildren; i++) {
        if (net_send_bytes(child_socks[i], hdr, sizeof(gexec_hdr)) != E_OK) {
            rval = GEXEC_NET_ERROR;
            goto cleanup;
        }
        if (net_send_bytes(child_socks[i], (void *)buf, hdr->bdy_len) != E_OK) {
            rval = GEXEC_NET_ERROR;
            goto cleanup;
        }
    }

    if (request_create_from_buf(req, buf, hdr->bdy_len) != E_OK) {
        rval = GEXEC_INTERNAL_ERROR;
        goto cleanup;
    }
/*      request_print(*req); */

    xfree(buf);
    return GEXEC_OK;

 cleanup:   
    xfree(buf);
    return rval;
}

static int recv_tree_create(int up_sock, gexec_hdr *hdr, credentials *creds,
                            signature *creds_sig, void **tbuf, int *tbuf_len)
{
    if (net_recv_bytes(up_sock, (void *)hdr, sizeof(gexec_hdr)) != E_OK)
        return GEXEC_NET_ERROR;
    if (net_recv_bytes(up_sock, (void *)creds, sizeof(credentials)) != E_OK)
        return GEXEC_NET_ERROR;
    if (net_recv_bytes(up_sock, (void *)creds_sig, sizeof(signature)) != E_OK)
        return GEXEC_NET_ERROR;
    *tbuf_len = hdr->bdy_len - sizeof(credentials) - sizeof(signature);
    *tbuf     = xmalloc(*tbuf_len);
    if (net_recv_bytes(up_sock, (void *)(*tbuf), *tbuf_len) != E_OK) {
        xfree(*tbuf);
        return GEXEC_NET_ERROR;
    }
    return GEXEC_OK;
}

/* Send stdin to child (close stdin on EOF) */
static int route_down_stdin_handler(int up_sock, int *child_socks, int nchildren, 
                                    int w_stdin, int stdin_len, char *stdin_buf)
{
    e_assert(w_stdin != -1);
    e_assert(stdin_len <= GEXEC_IO_CHUNK);

    bzero(stdin_buf, GEXEC_IO_CHUNK);
    if (net_recv_bytes(up_sock, stdin_buf, stdin_len) != GEXEC_OK)
        return GEXEC_NET_ERROR;
    if (stdin_len == 1 && stdin_buf[0] == EOF) {
        close(w_stdin);
        w_stdin = -1;
    }
    else {
        if (io_write_bytes(w_stdin, stdin_buf, stdin_len) != E_OK)
            return GEXEC_IO_ERROR;
    }
    return GEXEC_OK;
}

/* Send signal to user's process (group) */
static int route_down_signal_handler(int up_sock, int *child_socks, int nchildren,
                                     pid_t pgid, gexec_signal_bdy *sig_bdy)
                                     
{
    e_assert(pgid != -1);

    if (net_recv_bytes(up_sock, sig_bdy, sizeof(gexec_signal_bdy)) != GEXEC_OK)
        return GEXEC_NET_ERROR;
    killpg(pgid, sig_bdy->sig);
    return GEXEC_OK;
}

static int route_down_handler(int up_sock, int *child_socks, int nchildren, gexec_hdr *hdr, 
                              int self, pid_t pgid, int w_stdin)
{
    char                stdin_buf[GEXEC_IO_CHUNK];
    int                 i, rval;
    gexec_signal_bdy    sig_bdy;
    void                *bdy;

    switch (hdr->type) {

    case GEXEC_STDIN_MSG:
        if ((rval = route_down_stdin_handler(up_sock, child_socks, nchildren,   
                                             w_stdin, hdr->bdy_len, 
                                             stdin_buf)) != GEXEC_OK)
            return rval;
        bdy = stdin_buf;
        break;

    case GEXEC_SIGNAL_MSG:
        e_assert(hdr->bdy_len == sizeof(gexec_signal_bdy));
        if ((rval = route_down_signal_handler(up_sock, child_socks, nchildren,   
                                              pgid, &sig_bdy)) != GEXEC_OK)
            return rval;
        bdy = &sig_bdy;
        break;

    default:
        e_assert(0);
        return GEXEC_INTERNAL_ERROR;

    }

    if (hdr->dst != self) {
        for (i = 0; i < nchildren; i++) {
            if (net_send_bytes(child_socks[i], hdr, sizeof(gexec_hdr)) != GEXEC_OK)
                return GEXEC_NET_ERROR;            
            if (net_send_bytes(child_socks[i], bdy, hdr->bdy_len) != GEXEC_OK)
                return GEXEC_NET_ERROR;            
        }
    }
    return GEXEC_OK;
}

static int route_up_handler(int sock, int up_sock, pthread_mutex_t *lock, int self)
{
    gexec_hdr           hdr;
    void                *bdy;
    char                chunk[GEXEC_IO_CHUNK];
    gexec_hb_bdy        hb_bdy;
    gexec_tc_reply_bdy  tcr_bdy;
    gexec_job_reply_bdy jr_bdy;
    gexec_tc_error_bdy  tce_bdy;

    if (net_recv_bytes(sock, (void *)&hdr, sizeof(gexec_hdr)) != E_OK)
        return GEXEC_NET_ERROR;

    switch (hdr.type) {

    case GEXEC_HEARTBEAT_MSG:
        if (net_recv_bytes(sock, (void *)&hb_bdy, sizeof(hb_bdy)) != E_OK)
            return GEXEC_NET_ERROR;
        bdy = (void *)&hb_bdy;
        break;

    case GEXEC_STDOUT_MSG:
        if (net_recv_bytes(sock, (void *)chunk, hdr.bdy_len) != E_OK)
            return GEXEC_NET_ERROR;
        bdy = (void *)chunk;
        break;

    case GEXEC_STDERR_MSG:
        if (net_recv_bytes(sock, (void *)chunk, hdr.bdy_len) != E_OK)
            return GEXEC_NET_ERROR;
        bdy = (void *)chunk;
        break;

    case GEXEC_TREE_CREATE_REPLY_MSG:
        if (net_recv_bytes(sock, (void *)&tcr_bdy, sizeof(tcr_bdy)) != E_OK)
            return GEXEC_NET_ERROR;
        bdy = (void *)&tcr_bdy;
        break;

    case GEXEC_JOB_REPLY_MSG:
        if (net_recv_bytes(sock, (void *)&jr_bdy, sizeof(jr_bdy)) != E_OK)
            return GEXEC_NET_ERROR;
        bdy = (void *)&jr_bdy;
        break;

    case GEXEC_TREE_CREATE_ERROR_MSG:
        if (net_recv_bytes(sock, (void *)&tce_bdy, sizeof(tce_bdy)) != E_OK)
            return GEXEC_NET_ERROR;
        bdy = (void *)&tce_bdy;
        break;

    /* NOTE: fill me in (and matching code to create) */
/*      case GEXEC_JOB_ERROR_MSG: */
/*          break; */

    default:
        return GEXEC_INTERNAL_ERROR;
    }

    /* Route up only if this node isn't packet final destination */
    if (hdr.dst != self) 
        if (locked_msg_send(up_sock, lock, &hdr, bdy) != GEXEC_OK)
            return GEXEC_NET_ERROR;

    return GEXEC_OK;
}

static void gexecd_tree_init(void *tbuf, int tbuf_len, int self, tree **t, 
                             int *parent, int *nchildren, int **child_nodes, 
                             char ***child_ips)
{
    int  i;
    char ip[16];

    net_gethostip(ip);

    tree_create_from_buf(t, tbuf, tbuf_len, 1);
    *parent    = tree_parent(*t, self);
    *nchildren = tree_num_children(*t, self);
    if (*nchildren > 0) {
        *child_nodes = (int *)xmalloc(*nchildren * sizeof(int));
        *child_ips   = (char **)xmalloc(*nchildren * sizeof(char *));

        for (i = 0; i < *nchildren; i++)
            (*child_nodes)[i] = tree_child(*t, self, i);
        for (i = 0; i < *nchildren; i++)
            (*child_ips)[i] = (*t)->ips[(*child_nodes)[i]];
    }
}

/* Open sockets to children and record any bad nodes */
static int open_child_socks(int **child_socks, int nchildren, int *child_nodes,
                            char **child_ips, int port, int *bad_nodes)
{
    int i, sock_error = 0;

    if (nchildren == 0)
        return GEXEC_OK;

    (*child_socks) = xmalloc(nchildren * sizeof(int));
    for (i = 0; i < nchildren; i++) {
        (*child_socks)[i] = -1;
        if (net_cli_sock_create(&(*child_socks)[i], child_ips[i], 
                                port) != E_OK) {
            bitmask_set(bad_nodes, GEXEC_MAX_NPROCS, child_nodes[i]);
            sock_error = 1;
        }
    }
    if (sock_error)
        goto cleanup;
    return GEXEC_OK;

 cleanup:
    for (i = 0; i < nchildren; i++)
        if ((*child_socks)[i] != -1)
            close((*child_socks)[i]);
    xfree(*child_socks);
    *child_socks = NULL;
    return GEXEC_NET_ERROR;
}

/* Recv tree create msg and create the tree locally */
static int gexec_tni_recv_tc(gexecd_state *s, gexec_hdr *tc_hdr, 
                             credentials *tc_creds, signature *tc_creds_sig, 
                             void **tc_tbuf, int *tc_tbuf_len)
{
    int rval;

    if ((rval = recv_tree_create(s->up_sock, tc_hdr, tc_creds, tc_creds_sig,
                                 tc_tbuf, tc_tbuf_len)) != GEXEC_OK)
        return rval;
    s->self = tc_hdr->dst;
    gexecd_tree_init(*tc_tbuf, *tc_tbuf_len, s->self, &s->tree, &s->parent, 
                     &s->nchildren, &s->child_nodes, &s->child_ips);
    return GEXEC_OK;
}

/* Authenticate (send error on failure) */
static int gexec_tni_auth(gexecd_state *s, credentials *tc_creds, 
                          signature *tc_creds_sig)
{
    int rval;
    int bad_nodes[GEXEC_BITMASK_SIZE];

    rval = gexecd_authenticate(tc_creds, tc_creds_sig, &s->uid, &s->gid);
    if (rval != GEXEC_OK) {
        bitmask_reset(bad_nodes, GEXEC_MAX_NPROCS);
        bitmask_set(bad_nodes, GEXEC_MAX_NPROCS, s->self);
        send_tc_error_and_wait(s->up_sock, &s->up_sock_lock, s->self, 
                               s->parent, rval, bad_nodes);
        return rval;
    }
    return GEXEC_OK;
}

/* Send tree create msg to children (send error on failure) */
static int gexec_tni_send_tc(gexecd_state *s, gexecd_options *opts, 
                             gexec_hdr *tc_hdr, credentials *tc_creds, 
                             signature *tc_creds_sig, void *tc_tbuf, 
                             int tc_tbuf_len, int *bad_nodes)
{
    int i, rval = GEXEC_OK;

    if (open_child_socks(&s->child_socks, s->nchildren, s->child_nodes, 
                         s->child_ips, opts->port, bad_nodes) != GEXEC_OK) {
        send_tc_error_and_wait(s->up_sock, &s->up_sock_lock, s->self, s->parent,
                               GEXEC_NET_ERROR, bad_nodes);
        return GEXEC_NET_ERROR;
    }

    for (i = 0; i < s->nchildren; i++) {
        tc_hdr->dst = s->child_nodes[i];
        if (send_tree_create(s->child_socks[i], tc_hdr, tc_creds, tc_creds_sig,
                             tc_tbuf, tc_tbuf_len) != GEXEC_OK) {
            bitmask_set(bad_nodes, GEXEC_MAX_NPROCS, s->child_nodes[i]);
            rval = GEXEC_NET_ERROR;
        }
    }

    if (rval != GEXEC_OK)
        send_tc_error_and_wait(s->up_sock, &s->up_sock_lock, s->self, 
                               s->parent, GEXEC_NET_ERROR, bad_nodes);
    return rval;
}

/* Recv tree create replies/errors from children (send error on failure) */
static int gexec_tni_recv_tc_replies(gexecd_state *s, int *bad_nodes)
{
    gexec_hdr           hdr;
    gexec_tc_reply_bdy  tcr_bdy; 
    gexec_tc_error_bdy  tce_bdy;
    int                 i, sock, rval = GEXEC_OK;
    
    for (i = 0; i < s->nchildren; i++) {
        sock = s->child_socks[i];
        if (net_recv_bytes(sock, &hdr, sizeof(hdr)) != GEXEC_OK) {
            bitmask_set(bad_nodes, GEXEC_MAX_NPROCS, s->child_nodes[i]);
            rval = GEXEC_NET_ERROR;
            continue;
        }
        switch (hdr.type) {
        case GEXEC_TREE_CREATE_REPLY_MSG:
            if (net_recv_bytes(sock, &tcr_bdy, sizeof(tcr_bdy)) != GEXEC_OK) {
                bitmask_set(bad_nodes, GEXEC_MAX_NPROCS, s->child_nodes[i]);
                rval = GEXEC_NET_ERROR;                
            }
            break;
        case GEXEC_TREE_CREATE_ERROR_MSG: /* Aggregate errors from children */
            rval = GEXEC_NET_ERROR;
            if (net_recv_bytes(sock, &tce_bdy, sizeof(tce_bdy)) == GEXEC_OK)
                bitmask_or(bad_nodes, tce_bdy.bad_nodes, GEXEC_MAX_NPROCS);
            else
                bitmask_set(bad_nodes, GEXEC_MAX_NPROCS, s->child_nodes[i]);
            break;
        }
    }

    if (rval != GEXEC_OK)
        send_tc_error_and_wait(s->up_sock, &s->up_sock_lock, s->self,
                               s->parent, rval, bad_nodes);
    return rval;
}

static int gexec_tni_send_tc_reply(gexecd_state *s)
{
    if (send_tc_reply(s->up_sock, &s->up_sock_lock, s->self, s->parent,
                      GEXEC_OK) != GEXEC_OK)
        return GEXEC_NET_ERROR;
    return GEXEC_OK;
}

/* Create tree, open child sockets, and forward tree create request. */
static int gexec_tree_node_init(gexecd_state *s, gexecd_options *opts)
{
    gexec_hdr    hdr;
    credentials  creds;
    signature    creds_sig;
    void         *tbuf = NULL;
    int          tbuf_len, rval;
    int          bad_nodes[GEXEC_BITMASK_SIZE];

    e_assert(GEXEC_MAX_NPROCS ==  GEXEC_BITMASK_SIZE * BITMASK_CHUNK);
    bitmask_reset(bad_nodes, GEXEC_MAX_NPROCS);

    rval = gexec_tni_recv_tc(s, &hdr, &creds, &creds_sig, &tbuf, &tbuf_len);
    if (rval != GEXEC_OK)
        goto cleanup;

    rval = gexec_tni_auth(s, &creds, &creds_sig);
    if (rval != GEXEC_OK)
         goto cleanup;

    rval = gexec_tni_send_tc(s, opts, &hdr, &creds, &creds_sig, tbuf, 
                             tbuf_len, bad_nodes);
    if (rval != GEXEC_OK)
         goto cleanup;

    rval = gexec_tni_recv_tc_replies(s, bad_nodes);
    if (rval != GEXEC_OK)
         goto cleanup;

    rval = gexec_tni_send_tc_reply(s);
    if (rval != GEXEC_OK)
         goto cleanup;

    xfree(tbuf);
    return GEXEC_OK;

 cleanup:
    if (tbuf != NULL) xfree(tbuf);
    return rval;
}

static int gexec_job_init(gexecd_state *s, gexecd_options *opts)
{
    gexec_hdr       hdr;
    int             rval;

    if (net_recv_bytes(s->up_sock, (void *)&hdr, sizeof(gexec_hdr)) != E_OK)
        return GEXEC_NET_ERROR;
    if (hdr.type != GEXEC_JOB_MSG)
        return GEXEC_INTERNAL_ERROR;
    if ((rval = forward_request(s->up_sock, s->child_socks, s->nchildren, &hdr, 
                                s->uid, s->gid, &s->req)) != GEXEC_OK)
        return rval;
    return GEXEC_OK;
}

static int create_pipes(int stdin_pipe[2], int stdout_pipe[2], int stderr_pipe[2])
{
    /* Create pipes for stdin, stdout, stderr forwarding */
    if (pipe(stdin_pipe) < 0)
        goto cleanup0;
    if (pipe(stdout_pipe) < 0)
        goto cleanup1;
    if (pipe(stderr_pipe) < 0)
        goto cleanup2;
    return GEXEC_OK;

 cleanup2:
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);
 cleanup1:
    close(stdin_pipe[0]);
    close(stdin_pipe[1]);
 cleanup0:
    return GEXEC_PIPE_ERROR;
}

/* 
 * child_setup_pipes: make pipes correspond to stdin (w), stdout (r),
 * stderr (r) fds and close unneeded sides of pipes.  On error, may
 * close stdin, stdout, stderr.  This is acceptable since function is
 * only used after a fork and before an exec.  If this fails, the exec
 * will never be reached.
 */
static int child_setup_pipes(int *stdin_pipe, int *stdout_pipe, int *stderr_pipe)
{
    int rval;

    /* Make pipes correspond to stdin (w), stdout (r), stderr (r) fds */
    if (stdin_pipe[0] != STDIN_FILENO) {
        if (dup2(stdin_pipe[0], STDIN_FILENO) < 0) {
            rval = GEXEC_DUP2_ERROR;
            goto cleanup0;
        }
        close(stdin_pipe[0]);
    }
    if (stdout_pipe[1] != STDOUT_FILENO) {
        if (dup2(stdout_pipe[1], STDOUT_FILENO) < 0) {
            rval = GEXEC_DUP2_ERROR;
            goto cleanup1;
        }
        close(stdout_pipe[1]);
    }
    if (stderr_pipe[1] != STDERR_FILENO) {
        if (dup2(stderr_pipe[1], STDERR_FILENO) < 0) {
            rval = GEXEC_DUP2_ERROR;
            goto cleanup2;
        }
        close(stderr_pipe[1]);
    }

    /* Close unneeded sides of pipes */
    close(stdin_pipe[1]);
    close(stdout_pipe[0]);
    close(stderr_pipe[0]);
    return GEXEC_OK;

 cleanup2:
    close(STDOUT_FILENO);
 cleanup1:
    close(STDIN_FILENO);
 cleanup0:
    return rval;
}

/* 
 * parent_setup_pipes: close unneeded file descriptors. Leave stdin
 * writer stdout reader, stderr reader open.
 */
static void parent_setup_pipes(int *stdin_pipe, int *stdout_pipe, int *stderr_pipe)
{
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);
    stdin_pipe[0]  = -1;
    stdout_pipe[1] = -1;
    stderr_pipe[1] = -1;
}

static void child_close_descriptors()
{
    char            dirname[256];
    int             fd;
    pid_t           pid;
    DIR             *dir;
    struct dirent   *dirent;

    pid = getpid();
    bzero(dirname, 256);
    sprintf(dirname, "/proc/%d/fd", pid);
    dir = opendir(dirname);
    while ((dirent = readdir(dir)) != NULL) {
        fd = atoi(dirent->d_name);
        if ((fd != STDIN_FILENO) && (fd != STDOUT_FILENO) && (fd != STDERR_FILENO))
            close(fd);
    }
}

static int child_setup_env(request *r, uid_t uid, gid_t gid)
{
    int i;

    for (i = 0; i < r->envc; i++)
        if (putenv(r->envp[i]) < 0)
            return GEXEC_PUTENV_ERROR;
    if (initgroups(r->user, gid) < 0)
        return GEXEC_INITGROUPS_ERROR;
    if (setgid(gid) < 0)
        return GEXEC_SETGID_ERROR;
    if (setuid(uid) < 0)
        return GEXEC_SETUID_ERROR;
    if (chdir(r->cwd) < 0) {
        fprintf(stderr, "Could not chdir to %s\n", r->cwd);
        return GEXEC_CHDIR_ERROR;
    }
    return GEXEC_OK;
}

static int child_setup_gexec_env(int vnn, int nprocs, _gexec_gpid *gpid)
{
    char *my_vnn_env;
    char *nprocs_env;
    char *gpid_env;

    my_vnn_env = (char *)xmalloc(128 * sizeof(char));
    nprocs_env = (char *)xmalloc(128 * sizeof(char));
    gpid_env   = (char *)xmalloc(256 * sizeof(char));
    
    bzero(my_vnn_env, 128 * sizeof(char));
    bzero(nprocs_env, 128 * sizeof(char));
    bzero(gpid_env, 256 * sizeof(char));

    sprintf(my_vnn_env, "GEXEC_MY_VNN=%d", vnn);
    sprintf(nprocs_env, "GEXEC_NPROCS=%d", nprocs);
    sprintf(gpid_env, "GEXEC_GPID=%8x:%ld:%ld", gpid->addr.s_addr,
            gpid->sec, gpid->usec);

    if (putenv(my_vnn_env) < 0)
        return GEXEC_PUTENV_ERROR;
    if (putenv(nprocs_env) < 0)
        return GEXEC_PUTENV_ERROR;
    if (putenv(gpid_env) < 0)
        return GEXEC_PUTENV_ERROR;
    return GEXEC_OK;
}

/*
 * Read newline terminated data into n_buf and leave extra characters
 * after last newline in n_buf in x_buf.  Next iteration we'll copy
 * x_buf to beginning of n_buf and then look for a newline termination
 * again. In case where we can't find any newlines in n_buf, we just
 * return the buffer. (This shouldn't happen very often.)
 */
static int read_newline_term_buf(int fd,
                                 char *n_buf, int n_buflen, int *n_bytes_read, 
                                 char *x_buf, int x_buflen, int *x_bytes_read)
{
    int  has_newline, n;
    char *p;

    *n_bytes_read = 0;
    *x_bytes_read = 0;
    has_newline   = 0;

    while ((*n_bytes_read) != (n_buflen - NULLBYTE_SIZE) && !has_newline) {
        if ((n = read(fd, (void *)((long int)n_buf + (long int)*n_bytes_read), 
                      (n_buflen - NULLBYTE_SIZE) - *n_bytes_read)) <= 0) {
            if (n <= 0) /* EOF (i.e., fd closed -> job exited) */
                return GEXEC_READ_ERROR;
        }
        *n_bytes_read += n;
        n_buf[*n_bytes_read] = '\0'; /* For strrchr (we overwrite later) */
        /* 
         * If have newline in n_buf, move characters after last
         * newline into x_buf so that n_buf ends in a newline.  We'll
         * copy x_buf into next n_buf the next time around.
         */
        if ((p = strrchr(n_buf, '\n')) != NULL) {
            *x_bytes_read  = (*n_bytes_read - (p - n_buf + 1));
            *n_bytes_read -= (*n_bytes_read - (p - n_buf + 1));
            memcpy(x_buf, p + 1, *x_bytes_read);
            has_newline = 1;
        }
    }
    return GEXEC_OK;
}

static void read_send_stdxxx(int fd, int up_sock, pthread_mutex_t *up_sock_lock,
                             int src, int dst)
{
    gexec_hdr     hdr;
    char          n_buf[GEXEC_IO_CHUNK];  /* Newline termimated buf     */
    char          x_buf[GEXEC_IO_CHUNK];  /* Extra chars for next n_buf */
    int           n_buflen, x_buflen;
    int           n_nbytes, x_nbytes;
    int           n_bytes_read, x_bytes_read;

    n_nbytes = 0;
    x_nbytes = 0;

    for (;;) {
        bzero(n_buf, GEXEC_IO_CHUNK);

        /* Move x_buf (no newlines) into start of n_buf */
        memcpy(n_buf, x_buf, x_nbytes);
        bzero(x_buf, GEXEC_IO_CHUNK);
        n_nbytes = x_nbytes;
        x_nbytes = 0;

        n_buflen = GEXEC_IO_CHUNK - n_nbytes;
        x_buflen = GEXEC_IO_CHUNK;

        /* Read data into rest of n_buf, no newline data into xbuf */
        if (read_newline_term_buf(fd, &n_buf[n_nbytes], n_buflen,
                                  &n_bytes_read, x_buf, x_buflen, 
                                  &x_bytes_read) != GEXEC_OK) {
            n_nbytes += n_bytes_read;
            x_nbytes += x_bytes_read;
            break;
        }

        /* Null terminate the output and send up the tree towards root */
        n_nbytes += n_bytes_read;
        x_nbytes += x_bytes_read;

        e_assert(n_nbytes < GEXEC_IO_CHUNK);
        e_assert(x_nbytes < GEXEC_IO_CHUNK);

        n_buf[n_nbytes] = '\0';
        x_buf[x_nbytes] = '\0';

        GEXEC_HDR_BUILD(&hdr, GEXEC_STDOUT_MSG, src, dst,
                        n_nbytes + NULLBYTE_SIZE);
        if (locked_msg_send(up_sock, up_sock_lock, &hdr, n_buf) != GEXEC_OK) {
            n_nbytes = 0;
            break;
        }
    }
    if (n_nbytes != 0) {
        n_buf[n_nbytes] = '\0';
        GEXEC_HDR_BUILD(&hdr, GEXEC_STDOUT_MSG, src, dst, 
                        n_nbytes + NULLBYTE_SIZE);
        locked_msg_send(up_sock, up_sock_lock, &hdr, n_buf);
    }
}

static void *stdout_thr(void *arg)
{
    gexecd_state  *s = (gexecd_state *)arg;

    /* Read stdout and forward it towards the root of the tree */
    read_send_stdxxx(s->r_stdout, s->up_sock, &s->up_sock_lock, s->self, ROOT);

    sem_post(&s->stdout_sem);
    sem_wait(&s->killme_sem);
    return NULL;
}

static void *stderr_thr(void *arg)
{
    gexecd_state  *s = (gexecd_state *)arg;

    /* Read stderr and forward it towards the root of the tree */
    read_send_stdxxx(s->r_stderr, s->up_sock, &s->up_sock_lock, s->self, ROOT);

    sem_post(&s->stderr_sem);
    sem_wait(&s->killme_sem);
    return NULL;
}

static void *job_thr(void *arg)
{
    gexecd_state    *s = (gexecd_state *)arg;
    int             stdin_pipe[2], stdout_pipe[2], stderr_pipe[2];
    int             status;
    pthread_attr_t  attr;

    e_assert(s->req != NULL);
    if (create_pipes(stdin_pipe, stdout_pipe, stderr_pipe) != GEXEC_OK)
        goto cleanup;
	s->w_stdin  = stdin_pipe[1];
	s->r_stdout = stdout_pipe[0];
	s->r_stderr = stderr_pipe[0];

    if ((s->pid = fork()) < 0)
        goto cleanup;

    if (s->pid == 0) {
        if (setpgid(getpid(), getpid()) < 0)
            exit(1);
        if (child_setup_pipes(stdin_pipe, stdout_pipe, stderr_pipe) != GEXEC_OK)
            exit(2);
        child_close_descriptors();
        if (child_setup_env(s->req, s->uid, s->gid) != GEXEC_OK)
            exit(2);
        if (child_setup_gexec_env(VNN(s->self), s->tree->nhosts - 1, 
                                  &s->req->gpid) != GEXEC_OK)
            exit(2);
        if (execv(s->req->argv[0], s->req->argv) < 0) {
            fprintf(stderr, "Could not exec %s\n", s->req->argv[0]);
            exit(1);            
        }
        exit(0);
    }
    /* If detached, clean up as soon we fork/exec the process */
    if (s->req->opts.detached)
        goto cleanup;

    s->pgid = s->pid;
    parent_setup_pipes(stdin_pipe, stdout_pipe, stderr_pipe);
    sem_post(&s->jobstart_sem);

    pthread_attr_init(&attr);
    if (pthread_create(&s->stdout_tid, &attr, stdout_thr, (void *)s) != 0)
        goto cleanup;
    if (pthread_create(&s->stderr_tid, &attr, stderr_thr, (void *)s) != 0)
        goto cleanup;

 waitpid:
    /* NOTE: may want to send error msg up nicely, no cleanup - wait for cli exit */
    if (waitpid(s->pid, &status, 0) < 0) {
        if (errno == EINTR)
            goto waitpid;
        goto cleanup;
    }

    /* NOTE: change */
    s->rval = (WIFEXITED(status)) ? GEXEC_OK : GEXEC_WAITPID_ERROR;
    sem_wait(&s->stdout_sem);
    sem_wait(&s->stderr_sem);
    send_job_reply(s->up_sock, &s->up_sock_lock, s->self, ROOT, GEXEC_OK);
    sem_wait(&s->killme_sem);
    return NULL;
    
 cleanup:
    sem_post(&s->cleanup_sem);
    sem_wait(&s->killme_sem);
    return NULL;
}

/* For routing stdin and signal messages to jobs */
static void *route_down_thr(void *arg)
{
    gexecd_state    *s = (gexecd_state *)arg;
    gexec_hdr       hdr;
    pthread_attr_t  attr;

    /* Wait until pipes and pid/pgid valid */
    sem_wait(&s->jobstart_sem);
    e_assert(s->req != NULL);

    pthread_attr_init(&attr);
    for (;;) {
        if (net_recv_bytes(s->up_sock, (void *)&hdr, sizeof(gexec_hdr)) != E_OK)
            goto cleanup;
        if (route_down_handler(s->up_sock, s->child_socks, s->nchildren, &hdr, 
                               s->self, s->pgid, s->w_stdin) != GEXEC_OK)
            goto cleanup;
    }
 cleanup:
    sem_post(&s->cleanup_sem);
    sem_wait(&s->killme_sem);
    return NULL;
}

static void *route_up_thr(void *arg)
{
    gexecd_state    *s = (gexecd_state *)arg;
    fd_set          readfds, readfds_orig;
    int             max_sock = 0, i, n;
    struct timeval  timeout;

    e_assert(s->req != NULL);
    FD_ZERO(&readfds_orig);
    for (i = 0; i < s->nchildren; i++) {
        FD_SET(s->child_socks[i], &readfds_orig);
        if (s->child_socks[i] > max_sock)
            max_sock = s->child_socks[i];
    }

    for (;;) {
        readfds = readfds_orig;
        timeout.tv_sec  = 0;
        timeout.tv_usec = 250000; /* For cancellation check */
        if ((n = select(max_sock + 1, &readfds, NULL, NULL, &timeout)) < 0)
            goto cleanup;
        /* Route reply packets upstream (if available) */
        for (i = 0; i < s->nchildren; i++)
            if (FD_ISSET(s->child_socks[i], &readfds))
                if (route_up_handler(s->child_socks[i], s->up_sock,
                                     &s->up_sock_lock, s->self) != GEXEC_OK)
                    goto cleanup;
        if (n == 0) pthread_testcancel();
    }
 cleanup:
    sem_post(&s->cleanup_sem);
    sem_wait(&s->killme_sem);
    return NULL;
}

static void *heartbeat_thr(void *arg)
{
    gexecd_state    *s = (gexecd_state *)arg;
    gexec_hdr       hdr;
    gexec_hb_bdy    hb_bdy;
    pthread_cond_t  wait_cv   = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t wait_lock = PTHREAD_MUTEX_INITIALIZER;
    struct timespec timeout;

    GEXEC_HDR_BUILD(&hdr, GEXEC_HEARTBEAT_MSG, s->self, s->parent, sizeof(hb_bdy));

    for (;;) {
        timeout.tv_sec  = time(NULL) + GEXECD_HEARTBEAT_INT;
        timeout.tv_nsec = 0;
        
        pthread_mutex_lock(&wait_lock);
        pthread_cleanup_push(mutex_cleanup, (void *)&wait_lock);
        pthread_cond_timedwait(&wait_cv, &wait_lock, &timeout);
        pthread_cleanup_pop(0);
        pthread_mutex_unlock(&wait_lock);

        if (locked_msg_send(s->up_sock, &s->up_sock_lock, &hdr, &hb_bdy) != GEXEC_OK)
            goto cleanup;
    }
 cleanup:
    sem_post(&s->cleanup_sem);
    sem_wait(&s->killme_sem);
    return NULL;
}

static void *gexecd_thr(void *arg)
{
    gexecd_thr_args *args = (gexecd_thr_args *)arg;
    gexecd_options  *opts;
    gexecd_state    *s;
    pthread_attr_t  attr;

    gexecd_state_create(&s);
    pthread_attr_init(&attr);
    s->up_sock = args->up_sock;
    opts       = args->opts;
    xfree(args);

    if (gexec_tree_node_init(s, opts) != GEXEC_OK)
        goto cleanup;
    if (gexec_job_init(s, opts) != GEXEC_OK)
        goto cleanup;

    e_assert(s->req != NULL);
    if (pthread_create(&s->job_tid, &attr, job_thr, 
                       (void *)s) != 0) 
        goto cleanup;
    if (!s->req->opts.detached) {
        if (pthread_create(&s->route_down_tid, &attr, route_down_thr, 
                           (void *)s) != 0)
            goto cleanup;
        if (pthread_create(&s->route_up_tid, &attr, route_up_thr, 
                           (void *)s) != 0)
            goto cleanup;
        if (pthread_create(&s->heartbeat_tid, &attr, heartbeat_thr, 
                           (void *)s) != 0)
            goto cleanup;
    }
    sem_wait(&s->cleanup_sem);

 cleanup:
    pthread_attr_destroy(&attr);
    gexecd_state_destroy(s);
    return NULL;
}

void gexecd_standalone(gexecd_options *opts)
{
    int                 svr_sock;
    int                 cli_sock;
    pthread_attr_t      attr;
    pthread_t           tid;
    socklen_t           sockaddr_len = sizeof(struct sockaddr_in);
    struct sockaddr_in  sockaddr; 
    gexecd_thr_args     *args;

    if (net_svr_sock_create(&svr_sock, opts->port) != E_OK) {
        fprintf(stderr, "Bind error on TCP socket on port %d\n",
                opts->port);
        exit(2);
    }
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    for (;;) {
        if ((cli_sock = accept(svr_sock, (struct sockaddr *)&sockaddr,
                               &sockaddr_len)) < 0) {
            if (errno == EINTR)
                continue;
            fatal_error("accept error\n");
            exit(1);
        }
        args = (gexecd_thr_args *)xmalloc(sizeof(gexecd_thr_args));
        args->up_sock = cli_sock;
        args->opts    = opts;
        if (pthread_create(&tid, &attr, gexecd_thr, (void *)args) != 0)
            close(cli_sock);
    }
    pthread_attr_destroy(&attr);
    net_svr_sock_destroy(svr_sock);
}

int main(int argc, char **argv)
{
    gexecd_options  opts;
    gexecd_thr_args *args;

    gexecd_options_init(&opts);
    gexecd_options_parse(&opts, argc, argv);

    signal(SIGPIPE, SIG_IGN);

    if (opts.inetd) {
        args = (gexecd_thr_args *)xmalloc(sizeof(gexecd_thr_args));
        args->up_sock = STDIN_FILENO;
        args->opts    = &opts;
        gexecd_thr(args);
    }
    else 
        gexecd_standalone(&opts);

    return 0;
}
