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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <auth.h>
#include <e/barrier.h>
#include <e/bitmask.h>
#include <e/e_error.h>
#include <e/net.h>
#include <e/tree.h>
#include <e/xmalloc.h>
#include "gexec_lib.h"
#include "gexec_state.h"
#include "msg.h"
#include "request.h"

/* Need this global to catch SIGCONT only by signals threads */
static pthread_key_t g_sigkey;

static void gexec_state_create(gexec_state **s)
{
    (*s) = (gexec_state *)xmalloc(sizeof(gexec_state));

    (*s)->stdin_tid   = -1;
    (*s)->signals_tid = -1;
    (*s)->recv_tid    = -1;

    (*s)->tree        = NULL;
    (*s)->self        = -1;
    (*s)->nchildren   = 0;
    (*s)->child_nodes = NULL; 
    (*s)->child_ips   = NULL;
    (*s)->hosts       = NULL;
    (*s)->bad_nodes   = NULL;

    (*s)->child_socks = NULL;
    (*s)->child_locks = NULL;

    (*s)->req         = NULL;  
    (*s)->rval        = GEXEC_OK;  

    (*s)->ndone       = 0;       
    if (sem_init(&(*s)->cleanup_sem, 0, 0) != 0) {
        fatal_error("sem_init error\n");
        exit(1);
    }
    if (sem_init(&(*s)->killme_sem, 0, 0) != 0) {
        fatal_error("sem_init error\n");
        exit(1);
    }
}

static void gexec_state_destroy(gexec_state *s)
{
    int i, nhosts = 0;

    if (s->stdin_tid != -1) {
        e_assert(pthread_cancel(s->stdin_tid) == 0);
        e_assert(pthread_join(s->stdin_tid, NULL) == 0);
    }
    if (s->signals_tid != -1) {
        e_assert(pthread_cancel(s->signals_tid) == 0);
        e_assert(pthread_join(s->signals_tid, NULL) == 0);
    }
    if (s->recv_tid != -1) {
        e_assert(pthread_cancel(s->recv_tid) == 0);
        e_assert(pthread_join(s->recv_tid, NULL) == 0);
    }

    if (s->tree != NULL) {
        nhosts = s->tree->nhosts;
        tree_destroy(s->tree);
    }
    if (s->child_nodes != NULL)
        xfree(s->child_nodes);
    if (s->child_ips != NULL)
        xfree(s->child_ips);
    if (s->hosts != NULL) {
        for (i = 0; i < nhosts; i++)
            xfree(s->hosts[i]);
        xfree(s->hosts);
    }

    if (s->child_socks != NULL) {
        for (i = 0; i < s->nchildren; i++)
            if (s->child_socks[i] != -1)
                close(s->child_socks[i]);
        xfree(s->child_socks);
    }
    if (s->child_locks != NULL) {
        for (i = 0; i < s->nchildren; i++)
            e_assert(pthread_mutex_destroy(&s->child_locks[i]) == 0);
        xfree(s->child_locks);
    }

    if (s->req != NULL)
        request_destroy(s->req);

    sem_destroy(&s->cleanup_sem);
    sem_destroy(&s->killme_sem);
    xfree(s);
}

/* Called for thread cancellation or pthread_exit if on stack */
static void mutex_cleanup(void *arg)
{
    pthread_mutex_t *lock = (pthread_mutex_t *)arg;
    e_assert(pthread_mutex_unlock(lock) == 0);
}

/* All threads, except signal thread, block all signals */
static void block_signals(sigset_t *oldmask)
{
    sigset_t newmask;

    sigfillset(&newmask);
    sigdelset(&newmask, SIGINT); /* C-c (all threads exit) */
    pthread_sigmask(SIG_BLOCK, &newmask, oldmask);
}

/* Restore signal mask to what it was before call into gexec library */
static void unblock_signals(sigset_t *oldmask)
{
    pthread_sigmask(SIG_BLOCK, oldmask, NULL);
}

static void ips_with_root_create(int nhosts, char **ips,
                                 int *nhosts_wr, char ***ips_wr)
{
    char ip[IP_STRLEN];
    int  i;
    
    net_gethostip(ip);
    *nhosts_wr = nhosts + 1;
    *ips_wr    = (char **)xmalloc(*nhosts_wr * sizeof(char *));
    for (i = 0; i < *nhosts_wr; i++)
        (*ips_wr)[i] = (char *)xmalloc(IP_STRLEN);
    strcpy((*ips_wr)[0], ip);
    for (i = 1; i < *nhosts_wr; i++)
        strcpy((*ips_wr)[i], ips[i-1]);
}

static void ips_with_root_destroy(int nhosts_wr, char **ips_wr)
{
    int i;

    for (i = 0; i < nhosts_wr; i++)
        xfree(ips_wr[i]);
    xfree(ips_wr);
}

static int open_child_socks(int **child_socks, pthread_mutex_t **child_locks, int nchildren, 
                            int *child_nodes, char **child_ips, int port, int *bad_nodes)
{
    int  i, sock_error = 0;
    char host[1024];

    if (nchildren == 0)
        return GEXEC_OK;

    (*child_socks) = xmalloc(nchildren * sizeof(int));
    (*child_locks) = (pthread_mutex_t *)xmalloc(nchildren * sizeof(pthread_mutex_t));
    for (i = 0; i < nchildren; i++) { 
        (*child_socks)[i] = -1;
        if (net_cli_sock_create(&(*child_socks)[i], child_ips[i], port) != E_OK) {
            net_iptohost(child_ips[i], host, 1024);
            bitmask_set(bad_nodes, GEXEC_MAX_NPROCS, child_nodes[i]);
            sock_error = 1;
        }
        if (pthread_mutex_init(&(*child_locks)[i], NULL) != 0) {
            fprintf(stderr, "pthread_mutex_init error\n");
            exit(1);
        }
    }
    if (sock_error)
        goto cleanup;
    return GEXEC_OK;

 cleanup:
    for (i = 0; i < nchildren; i++) {
        pthread_mutex_destroy(&(*child_locks)[i]);
        if ((*child_socks)[i] != -1)
            close((*child_socks)[i]);
    }
    xfree(*child_locks);
    xfree(*child_socks);
    *child_locks = NULL;
    *child_socks = NULL;
    return GEXEC_NET_ERROR;
}

static int ips_to_hosts(char ***hosts, char **ips, int nhosts)
{
    int i;

    *hosts = (char **)xmalloc(nhosts * sizeof(char *));
    for (i = 0; i < nhosts; i++)
        (*hosts)[i] = xmalloc(1024 * sizeof(char));
    for (i = 0; i < nhosts; i++)
        if (net_iptohost(ips[i], (*hosts)[i], 1024) != E_OK) {
            fprintf(stderr, "Could not find hostname for %s\n", ips[i]);
            return GEXEC_NET_ERROR;
        }
    return GEXEC_OK;
}

static int compute_envc(char **envp)
{
    int envc;

    for (envc = 0; envp[envc] != NULL; envc++) {}
    return envc;
}

static int send_msg(int sock, gexec_hdr *hdr, void *buf)
{
    if (net_send_bytes(sock, (void *)hdr, sizeof(gexec_hdr)) != E_OK)
        return GEXEC_NET_ERROR;
    if (net_send_bytes(sock, (void *)buf, hdr->bdy_len) != E_OK)
        return GEXEC_NET_ERROR;
    return GEXEC_OK;
}

static int locked_send_msg(int sock, pthread_mutex_t *lock, 
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

static int send_tree_create_msg(int sock, gexec_hdr *hdr, void *tbuf,
                                int tbuf_len, credentials *creds, 
                                signature *creds_sig)
{
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

static int send_tree_create(int *child_socks, char **child_ips, int *child_nodes,
                            int nchildren, tree *t, credentials *creds, 
                            signature *creds_sig)
{
    gexec_hdr   hdr;
    int         tbuf_len, i, rval;
    void        *tbuf;
    char        host[1024];

    tbuf = tree_malloc_buf(t, &tbuf_len);
    
    hdr.type    = GEXEC_TREE_CREATE_MSG;
    hdr.src     = ROOT;

    hdr.bdy_len  = 0;
    hdr.bdy_len += sizeof(credentials);
    hdr.bdy_len += sizeof(signature);
    hdr.bdy_len += tbuf_len;

    for (i = 0; i < nchildren; i++) {
        hdr.dst = child_nodes[i];
        if ((rval = send_tree_create_msg(child_socks[i], &hdr, tbuf, tbuf_len,
                                         creds, creds_sig)) != GEXEC_OK) {
            net_iptohost(child_ips[i], host, 1024);
            fprintf(stderr, "Error sending to %s (%s)\n", host, child_ips[i]);
            xfree(tbuf);
            return rval;
        }
    }
    xfree(tbuf);
    return GEXEC_OK;
}

static int send_signal_msg(int *child_socks, pthread_mutex_t *child_locks, int nchildren, 
                           int self, int sig)
{
    int               i, sock;
    pthread_mutex_t   *lock;  
    gexec_hdr         hdr;  
    gexec_signal_bdy  sig_bdy;

    GEXEC_HDR_BUILD(&hdr, GEXEC_SIGNAL_MSG, self, GEXEC_DST_ALL, sizeof(sig_bdy));
    sig_bdy.sig = sig;
    for (i = 0; i < nchildren; i++) {
        sock = child_socks[i];
        lock = &child_locks[i];
        if (locked_send_msg(sock, lock, &hdr, &sig_bdy) != GEXEC_OK)
            return GEXEC_NET_ERROR;
    }
    return GEXEC_OK;
}

static int recv_tc_replies(int *child_socks, int *child_nodes, char **ips,
                           int *bad_nodes, int nchildren)
{
    int                 i, sock;
    int                 rval = GEXEC_OK;
    gexec_hdr           hdr;
    gexec_tc_reply_bdy  tcr_bdy;
    gexec_tc_error_bdy  tce_bdy;

    for (i = 0; i < nchildren; i++) {
        sock = child_socks[i];
        if (net_recv_bytes(sock, &hdr, sizeof(hdr)) != GEXEC_OK) {
            bitmask_set(bad_nodes, GEXEC_MAX_NPROCS, child_nodes[i]);
            rval = GEXEC_NET_ERROR;
            continue;
        }
        switch (hdr.type) {
        case GEXEC_TREE_CREATE_REPLY_MSG:
            if (net_recv_bytes(sock, &tcr_bdy, sizeof(tcr_bdy)) != GEXEC_OK) {
                bitmask_set(bad_nodes, GEXEC_MAX_NPROCS, child_nodes[i]);
                rval = GEXEC_NET_ERROR;                
            }
            break;
        case GEXEC_TREE_CREATE_ERROR_MSG: /* Aggregate errors from children */
            rval = GEXEC_NET_ERROR;
            if (net_recv_bytes(sock, &tce_bdy, sizeof(tce_bdy)) == GEXEC_OK)
                bitmask_or(bad_nodes, tce_bdy.bad_nodes, GEXEC_MAX_NPROCS);
            else
                bitmask_set(bad_nodes, GEXEC_MAX_NPROCS, child_nodes[i]);
        }
    }
    return rval;
}

static int gexec_tree_init(int nhosts, char **ips, int fanout, tree **t, int *self, 
                           int *nchildren, int **child_nodes, char ***child_ips)
{
    int  i, rval;
    char ip[16];

    if (net_gethostip(ip) != E_OK)
        return GEXEC_NET_ERROR;

    rval = tree_create(t, nhosts, ips, fanout, 1);
    e_assert(rval == E_OK);

    *self        = ROOT;
    *nchildren   = tree_num_children((*t), ROOT);
    if (*nchildren > 0) {
        *child_nodes = (int *)xmalloc(*nchildren * sizeof(int));
        *child_ips   = (char **)xmalloc(*nchildren * sizeof(char *));

        for (i = 0; i < *nchildren; i++)
            (*child_nodes)[i] = tree_child((*t), ROOT, i);
        for (i = 0; i < *nchildren; i++)
            (*child_ips)[i] = (*t)->ips[(*child_nodes)[i]];
    }
    return GEXEC_OK;
}

/* Create tree (including root) and open child sockets */
static int gexec_tree_root_init(gexec_state *s, int nhosts_wr, char **ips_wr, 
                                int fanout, int port)
{
    int rval;
    credentials       creds;
    signature         creds_sig;

    /* Create tree locally */
    if ((rval = gexec_tree_init(nhosts_wr, ips_wr, fanout, &s->tree, &s->self, 
                                &s->nchildren, &s->child_nodes,
                                &s->child_ips)) != GEXEC_OK)
        return rval;

    /* Open child sockets */
    if ((rval = open_child_socks(&s->child_socks, &s->child_locks, s->nchildren,
                                 s->child_nodes, s->child_ips, 
                                 port, s->bad_nodes)) != GEXEC_OK)
        return rval;

    /* Lookup and cache list of of hostnames if printing w/ that prefix */
    if (s->opts->prefix_type == GEXEC_HOST_PREFIX &&
        (rval = ips_to_hosts(&s->hosts, s->tree->ips, s->tree->nhosts)) != GEXEC_OK)
        return rval;

    /* Obtain credentials from authd on localhost */
    auth_init_credentials(&creds, GEXEC_CRED_LIFETIME);
    if (auth_get_signature(&creds, &creds_sig) != AUTH_OK) {
        fprintf(stderr, "Could not obtain signature from authd on localhost\n");
        return GEXEC_AUTH_ERROR;
    }

    /* Send tree create msg to children (including credentials) */
    if ((rval = send_tree_create(s->child_socks, s->child_ips, s->child_nodes, s->nchildren, 
                                 s->tree, &creds, &creds_sig)) != GEXEC_OK)
        return rval;
        
    /* Recv tree create replies/errors from children */
    if ((rval = recv_tc_replies(s->child_socks, s->child_nodes, s->tree->ips, 
                                s->bad_nodes, s->nchildren)) != GEXEC_OK)
        return rval;

    return GEXEC_OK;
}

static int gexec_job_init(int *child_socks, char **child_ips, int nchildren, 
                          int src, int dst, request *req)
{
    gexec_hdr   hdr;
    int         i, rval, len;
    char        host[1024];
    void        *buf;

    buf = request_malloc_buf(req, &len);
    GEXEC_HDR_BUILD(&hdr, GEXEC_JOB_MSG, src, dst, len);
    for (i = 0; i < nchildren; i++)
        if ((rval = send_msg(child_socks[i], &hdr, buf)) != GEXEC_OK) {
            net_iptohost(child_ips[i], host, 1024);
            fprintf(stderr, "Error sending to %s (%s)\n", host, child_ips[i]);
            xfree(buf);
            return rval;
        }
    xfree(buf);
    return GEXEC_OK;
}

static void prefixed_fprintf(FILE *stream, char *line, char *host, char *ip, int vnn, 
                             int prefix_type)
{
    switch (prefix_type) {

    case GEXEC_NO_PREFIX:
        fprintf(stream, "%s", line);
        break;

    case GEXEC_VNN_PREFIX:
        fprintf(stream, "%d %s", vnn, line);
        break;

    case GEXEC_HOST_PREFIX:
        fprintf(stream, "%s %s", host, line);
        break;

    case GEXEC_IP_PREFIX:
        fprintf(stream, "%s %s", ip, line);
        break;

    }
    fflush(stream);
}

/* Prefix every line (end-of-line = '\n') of output to stream with VNN */
static void prefixed_stdxxx(FILE *stream, char *buf, int len, char *host, char *ip, int vnn, 
                            int prefix_type)
{
    char *start, *newline, *end;
    char c;

    start = end = buf;
    while ((newline = strchr(start, '\n')) != NULL) {
        /* Put NULL after '\n' for printing (save original char) */
        end  = newline + 1;
        c    = *end;
        *end = '\0';
        /* Print prefix (if beginning of line) and line */
        prefixed_fprintf(stream, start, host, ip, vnn, prefix_type);
        /* Replace NULL with original character */
        *end  = c;
        start = end;
    }
    /* 
     * GEXEC_IO_CHUNK bytes w/out a newline. This could be bad for
     * interleaving output (with nonempty prefix) from multiple
     * nodes. This occurs when the user's program writes really long
     * output to stdout/stderr w/out newlines.
     */
    if (end != (buf + len - 1))
        prefixed_fprintf(stream, start, host, ip, vnn, prefix_type);
}

static int recv_heartbeat_handler(int child_sock, gexec_hdr *hdr)
{
    gexec_hb_bdy hb_bdy;

    if (net_recv_bytes(child_sock, &hb_bdy, hdr->bdy_len) != E_OK)
        return GEXEC_NET_ERROR;
    return GEXEC_OK;
}

static int recv_stdout_handler(int child_sock, gexec_hdr *hdr, char *host, char *ip, 
                               int prefix_type)
{
    char bdy[GEXEC_IO_CHUNK];

    if (net_recv_bytes(child_sock, bdy, hdr->bdy_len) != E_OK)
        return GEXEC_NET_ERROR;

    prefixed_stdxxx(stdout, (char *)bdy, hdr->bdy_len, host, ip, VNN(hdr->src), 
                    prefix_type);
    return GEXEC_OK;
}

static int recv_stderr_handler(int child_sock, gexec_hdr *hdr, char *host, char *ip, 
                               int prefix_type)
{
    char bdy[GEXEC_IO_CHUNK];

    if (net_recv_bytes(child_sock, bdy, hdr->bdy_len) != E_OK)
        return GEXEC_NET_ERROR;

    prefixed_stdxxx(stderr, (char *)bdy, hdr->bdy_len, host, ip, VNN(hdr->src), 
                    prefix_type);
    return GEXEC_OK;
}

static int recv_tc_reply_handler(int child_sock, gexec_hdr *hdr)
{
    gexec_tc_reply_bdy tcr_bdy;

    if (net_recv_bytes(child_sock, &tcr_bdy, sizeof(tcr_bdy)) != E_OK)
        return GEXEC_NET_ERROR;
    return GEXEC_OK;
}

static int recv_job_reply_handler(int child_sock, gexec_hdr *hdr)
{
    gexec_job_reply_bdy jr_bdy;

    if (net_recv_bytes(child_sock, &jr_bdy, sizeof(jr_bdy)) != E_OK)
        return GEXEC_NET_ERROR;
    return GEXEC_OK;
}

static int recv_handler(int child_sock, char *child_ip, char **hosts, char **ips, 
                        int *ndone, gexec_spawn_opts *opts)
{
    gexec_hdr hdr;
    char      *host;  

    if (net_recv_bytes(child_sock, (void *)&hdr, sizeof(gexec_hdr)) != E_OK)
        return GEXEC_NET_ERROR;

    switch (hdr.type) {

    case GEXEC_HEARTBEAT_MSG:
        return recv_heartbeat_handler(child_sock, &hdr);

    case GEXEC_STDOUT_MSG:
        host = (hosts != NULL) ? hosts[hdr.src] : NULL;
        return recv_stdout_handler(child_sock, &hdr, host, ips[hdr.src], 
                                   opts->prefix_type);

    case GEXEC_STDERR_MSG:
        host = (hosts != NULL) ? hosts[hdr.src] : NULL;
        return recv_stderr_handler(child_sock, &hdr, host, ips[hdr.src], 
                                   opts->prefix_type);

    case GEXEC_TREE_CREATE_REPLY_MSG:
        return recv_tc_reply_handler(child_sock, &hdr);

    case GEXEC_JOB_REPLY_MSG:
        (*ndone)++;
        return recv_job_reply_handler(child_sock, &hdr);

    default:
        return GEXEC_INTERNAL_ERROR;
    }
}

static void sigcont_handler(int sig)
{
    gexec_state *s;

    s = (gexec_state *)pthread_getspecific(g_sigkey);

    e_assert(s != NULL);
    e_assert(sig == SIGCONT);

    send_signal_msg(s->child_socks, s->child_locks, s->nchildren, s->self, sig);
}

static void *stdin_thr(void *arg)
{
    gexec_state     *s = (gexec_state *)arg;
    gexec_hdr       hdr;
    char            eof = EOF;
    char            buf[GEXEC_IO_CHUNK];
    fd_set          readfds, readfds_orig;
    int             n, i, sock;
    pthread_mutex_t *lock;
    struct timeval  timeout;

    FD_ZERO(&readfds_orig);
    FD_SET(STDIN_FILENO, &readfds_orig);

    for (;;) {
        bzero(buf, GEXEC_IO_CHUNK);
        n = read(STDIN_FILENO, buf, GEXEC_IO_CHUNK);
        if (n < 0 && errno == EIO) { /* bg case */
            readfds = readfds_orig;
            timeout.tv_sec  = 0;
            timeout.tv_usec = 250000; /* For cancellation point (read) */
            if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout) < 0)
                goto cleanup;
        }
        else if (n == 0) { /* EOF case (stdin closed) */
            GEXEC_HDR_BUILD(&hdr, GEXEC_STDIN_MSG, s->self, GEXEC_DST_ALL, 1);
            for (i = 0; i < s->nchildren; i++) {
                sock = s->child_socks[i];
                lock = &s->child_locks[i];
                if (locked_send_msg(sock, lock, &hdr, &eof) != GEXEC_OK)
                    goto cleanup;
            }
            goto wait;
        }
        else {
            GEXEC_HDR_BUILD(&hdr, GEXEC_STDIN_MSG, s->self, GEXEC_DST_ALL, n);  
            for (i = 0; i < s->nchildren; i++) {
                sock = s->child_socks[i];
                lock = &s->child_locks[i];
                if (locked_send_msg(sock, lock, &hdr, buf) != GEXEC_OK)
                    goto cleanup;
            }
        }
    }
 cleanup:
    sem_post(&s->cleanup_sem);
 wait:
    sem_wait(&s->killme_sem);
    return NULL; 
}

static void *signals_thr(void *arg)
{
    gexec_state *s = (gexec_state *)arg;
    sigset_t    mask;
    int         sig;

    pthread_setspecific(g_sigkey, s);

    sigfillset(&mask);
    pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
    signal(SIGCONT, sigcont_handler);

    for (;;) {
        sigwait(&mask, &sig);
        if (send_signal_msg(s->child_socks, s->child_locks, s->nchildren, 
                            s->self, sig) != GEXEC_OK)
            goto cleanup;

        switch (sig) {

        case SIGTSTP: /* C-z, bg */
            killpg(getpgrp(), SIGSTOP); /* Assumes same process group */
            break;
        }
    }
 cleanup:
    sem_post(&s->cleanup_sem);
    sem_wait(&s->killme_sem);
    return NULL; 
}

static void *recv_thr(void *arg) 
{
    gexec_state     *s = (gexec_state *)arg;
    fd_set          readfds, readfds_orig;
    int             max_sock = 0, i, n;
    struct timeval  timeout;

    /* Receive/handle any messages sent from down the tree */
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
        for (i = 0; i < s->nchildren; i++)
            if (FD_ISSET(s->child_socks[i], &readfds)) {
                if (recv_handler(s->child_socks[i], s->child_ips[i], s->hosts,
                                 s->tree->ips, &s->ndone, s->opts) != GEXEC_OK)
                    goto cleanup;
                if (s->ndone == (s->tree->nhosts - 1))
                    goto cleanup;
            }
        if (n == 0) pthread_testcancel();
    }
 cleanup:
    sem_post(&s->cleanup_sem);
    sem_wait(&s->killme_sem);
    return NULL; 
}

void gexec_init()
{
    pthread_key_create(&g_sigkey, NULL);
}

int gexec_spawn(int nprocs, char **ips, int argc, char **argv, char **envp, 
                gexec_spawn_opts *opts, int *bad_nodes)
{
    gexec_state     *state;
    int             rval, nprocs_wr;
    char            **ips_wr;
    pthread_attr_t  attr;
    sigset_t        oldmask;
  
    gexec_state_create(&state);
    block_signals(&oldmask);
    bitmask_reset(bad_nodes, GEXEC_MAX_NPROCS);
    state->opts      = opts;
    state->bad_nodes = bad_nodes;
    ips_with_root_create(nprocs, ips, &nprocs_wr, &ips_wr);
    pthread_attr_init(&attr);
    request_create(&state->req, argc, argv, compute_envc(envp), envp, opts);
    if ((rval = gexec_tree_root_init(state, nprocs_wr, ips_wr, opts->tree_fanout,
                                     opts->port)) != GEXEC_OK)
        goto cleanup;
    if ((rval = gexec_job_init(state->child_socks, state->child_ips, state->nchildren, 
                               state->self, GEXEC_DST_ALL, state->req)) != GEXEC_OK)
        goto cleanup;

        /* In detached mode, exit once request pushed down the tree */
    if (opts->detached)
        goto cleanup;

    if (pthread_create(&state->stdin_tid, &attr, stdin_thr, (void *)state) != 0) {
        rval = GEXEC_PTHREADS_ERROR;
        goto cleanup;
    }
    if (pthread_create(&state->recv_tid, &attr, recv_thr, (void *)state) != 0) {
        rval = GEXEC_PTHREADS_ERROR;
        goto cleanup;
    }
    if (pthread_create(&state->signals_tid, &attr, signals_thr, (void *)state) != 0) {
        rval = GEXEC_PTHREADS_ERROR;
        goto cleanup;
    }

    sem_wait(&state->cleanup_sem);
    rval = state->rval;

    pthread_attr_destroy(&attr);
    ips_with_root_destroy(nprocs_wr, ips_wr);
    bitmask_shift(bad_nodes, GEXEC_MAX_NPROCS, -1);
    unblock_signals(&oldmask);
    gexec_state_destroy(state);
    return rval;

 cleanup:
    pthread_attr_destroy(&attr);
    ips_with_root_destroy(nprocs_wr, ips_wr);
    bitmask_shift(bad_nodes, GEXEC_MAX_NPROCS, -1);
    unblock_signals(&oldmask);
    gexec_state_destroy(state);
    return GEXEC_OK;
}

int gexec_spawn_async(int nprocs, char **ips, int argc, char **argv, char **envp, 
                      gexec_spawn_opts *opts, gexec_handle *h)
{
    /* NOTE: not implemented yet */
    return GEXEC_NO_IMPL_ERROR;
}

int gexec_wait(gexec_handle *h, int *bad_nodes)
{
    /* NOTE: not implemented yet */
    return GEXEC_NO_IMPL_ERROR;
}

void gexec_shutdown()
{
    pthread_key_delete(g_sigkey);
}

int gexec_my_vnn()
{
    return atoi(getenv("GEXEC_MY_VNN"));
}

int gexec_nprocs()
{
    return atoi(getenv("GEXEC_NPROCS"));
}

char *gexec_gpid()
{
    return getenv("GEXEC_GPID");
}

int gexec_max_nprocs()
{
    return GEXEC_MAX_NPROCS;
}
