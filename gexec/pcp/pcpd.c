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
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include <openssl/sha.h>
#include <auth.h>
#include <e/e_error.h>
#include <e/io.h>
#include <e/net.h>
#include <e/tree.h>
#include <e/xmalloc.h>
#include "msg.h"
#include "pcp_lib.h"
#include "pcpd.h"
#include "pcpd_options.h"

static void pcpd_state_create(pcpd_state **s)
{
    (*s) = (pcpd_state *)xmalloc(sizeof(pcpd_state));
    (*s)->route_down_tid = -1;
    (*s)->route_up_tid   = -1;
    (*s)->heartbeat_tid  = -1;

    (*s)->uid = -1;
    (*s)->gid = -1;

    (*s)->tree        = NULL;
    (*s)->self        = -1;
    (*s)->parent      = -1;
    (*s)->nchildren   = 0;
    (*s)->child_nodes = NULL;
    (*s)->child_ips   = NULL;

    if (pthread_mutex_init(&(*s)->up_sock_lock, NULL) != 0) {
        fatal_error("pthread_mutex_init error\n");
        exit(1);
    }
    (*s)->up_sock     = -1;
    (*s)->child_socks = NULL; 

    if (sem_init(&(*s)->cleanup_sem, 0, 0) != 0) {
        fatal_error("sem_init error\n");
        exit(1);
    }
    if (sem_init(&(*s)->killme_sem, 0, 0) != 0) {
        fatal_error("sem_init error\n");
        exit(1);
    }
}

static void pcpd_state_destroy(pcpd_state *s)
{
    int i;

    if (s->route_down_tid != -1) {
        e_assert(pthread_cancel(s->route_down_tid) == 0);
        e_assert(pthread_join(s->route_down_tid, NULL) == 0);
    }
    if (s->route_up_tid != -1) {
        e_assert(pthread_cancel(s->route_up_tid) == 0);
        e_assert(pthread_join(s->route_up_tid, NULL) == 0);
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

    e_assert(pthread_mutex_destroy(&s->up_sock_lock) == 0);
    if (s->up_sock != -1)
        close(s->up_sock);
    if (s->child_socks != NULL) {
        for (i = 0; i < s->nchildren; i++)
            if (s->child_socks[i] != -1)
                close(s->child_socks[i]);
        xfree(s->child_socks);
    }

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

static int recv_tree_create(int up_sock, pcp_hdr *hdr, credentials *creds,
                            signature *creds_sig, void **tbuf, int *tbuf_len)
{
    if (net_recv_bytes(up_sock, (void *)hdr, sizeof(pcp_hdr)) != E_OK)
        return PCP_NET_ERROR;
    if (net_recv_bytes(up_sock, (void *)creds, sizeof(credentials)) != E_OK)
        return PCP_NET_ERROR;
    if (net_recv_bytes(up_sock, (void *)creds_sig, sizeof(signature)) != E_OK)
        return PCP_NET_ERROR;
    *tbuf_len = hdr->bdy_len - sizeof(credentials) - sizeof(signature);
    *tbuf     = xmalloc(*tbuf_len);
    if (net_recv_bytes(up_sock, (void *)(*tbuf), *tbuf_len) != E_OK) {
        xfree(*tbuf);
        return PCP_NET_ERROR;
    }
    return PCP_OK;
}

static int pcpd_authenticate(credentials *creds, signature *creds_sig,
                             uid_t *uid, gid_t *gid)
{
    if (auth_verify_signature(creds, creds_sig) != AUTH_OK)
        return PCP_AUTH_ERROR;
    *uid = creds->uid;
    *gid = creds->gid;
    return PCP_OK;
}

static void pcpd_tree_init(void *tbuf, int tbuf_len, int self, tree **t, 
                           int *parent, int *nchildren, int **child_nodes, 
                           char ***child_ips)
{
    int  i;
    char ip[16];

    net_gethostip(ip);

    tree_create_from_buf(t, tbuf, tbuf_len, 0);
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

static int open_child_socks(int **child_socks, int nchildren, char **child_ips,
                            int port, int *bad_child)
{
    int i, j;

    if (nchildren == 0)
        return PCP_OK;
    (*child_socks) = xmalloc(nchildren * sizeof(int));
    for (i = 0; i < nchildren; i++) 
        (*child_socks)[i] = -1;
    for (i = 0; i < nchildren; i++) 
        if (net_cli_sock_create(&(*child_socks)[i], child_ips[i], 
                                port) != E_OK) {
            *bad_child = i;
            goto cleanup;
        }
    return PCP_OK;

 cleanup:
    for (j = 0; j < i; j++)
        close((*child_socks)[j]);
    xfree(*child_socks);
    *child_socks = NULL;
    return PCP_NET_ERROR;
}

static int send_tree_create(int sock, pcp_hdr *hdr, credentials *creds,
                            signature *creds_sig, void **tbuf, int tbuf_len)
{ 
    if (sock != -1) {
        if (net_send_bytes(sock, (void *)hdr, sizeof(pcp_hdr)) != E_OK)
            return PCP_NET_ERROR;
        if (net_send_bytes(sock, (void *)creds, sizeof(credentials)) != E_OK)
            return PCP_NET_ERROR;
        if (net_send_bytes(sock, (void *)creds_sig, sizeof(signature)) != E_OK)
            return PCP_NET_ERROR;
        if (net_send_bytes(sock, tbuf, tbuf_len) != E_OK)
            return PCP_NET_ERROR;
    }
    return PCP_OK;
}

static int locked_msg_send(int sock, pthread_mutex_t *lock, 
                           pcp_hdr *hdr, void *bdy)
{
    int rval1, rval2;

    pthread_mutex_lock(lock);
    pthread_cleanup_push(mutex_cleanup, (void *)lock);
    rval1 = net_send_bytes(sock, (void *)hdr, sizeof(pcp_hdr));
    rval2 = net_send_bytes(sock, (void *)bdy, hdr->bdy_len);
    pthread_cleanup_pop(0);
    pthread_mutex_unlock(lock);

    if (rval1 != E_OK || rval2 != E_OK)
        return PCP_NET_ERROR;
    return PCP_OK;
}

static int send_pcp_write_reply(int sock, pthread_mutex_t *lock, int src, 
                                int dst, int rval)
{
    pcp_hdr          hdr;
    pcp_w_reply_bdy  bdy;

    PCP_HDR_BUILD(&hdr, PCP_WRITE_REPLY_MSG, src, dst, sizeof(pcp_w_reply_bdy));
    bdy.rval = rval;
    if (locked_msg_send(sock, lock, &hdr, &bdy) != PCP_OK)
        return PCP_NET_ERROR;

    return PCP_OK;
}

static int send_pcp_delete_reply(int sock, pthread_mutex_t *lock, int src, 
                                 int dst, int rval)
{
    pcp_hdr          hdr;
    pcp_d_reply_bdy  bdy;

    PCP_HDR_BUILD(&hdr, PCP_DELETE_REPLY_MSG, src, dst, sizeof(pcp_d_reply_bdy));
    bdy.rval = rval;
    if (locked_msg_send(sock, lock, &hdr, &bdy) != PCP_OK)
        return PCP_NET_ERROR;

    return PCP_OK;
}

static int send_pcp_checksum_reply(int sock, pthread_mutex_t *lock, int src, 
                                   int dst, int rval, unsigned char *checksum)
{
    pcp_hdr          hdr;
    pcp_c_reply_bdy  bdy;
    
    PCP_HDR_BUILD(&hdr, PCP_CHECKSUM_REPLY_MSG, src, dst, sizeof(pcp_c_reply_bdy));
    bdy.rval = rval;
    memcpy(bdy.checksum, checksum, PCP_CHECKSUM_SIZE);
    if (locked_msg_send(sock, lock, &hdr, &bdy) != PCP_OK)
        return PCP_NET_ERROR;

    return PCP_OK;
}

static void send_pcp_tc_error(int sock, pthread_mutex_t *lock, int src, 
                              int dst, int rval, int bad_node)
{
    pcp_hdr           hdr;
    pcp_tc_error_bdy  bdy;
 
    /* "Best effort" send */
    PCP_HDR_BUILD(&hdr, PCP_TREE_CREATE_ERROR_MSG, src, dst, 
                  sizeof(pcp_tc_error_bdy));
    bdy.rval     = rval;
    bdy.bad_node = bad_node;
    locked_msg_send(sock, lock, &hdr, &bdy);
}

static void send_pcp_write_error(int sock, pthread_mutex_t *lock, int src, 
                                 int dst, int rval)
{
    pcp_hdr          hdr;
    pcp_w_error_bdy  bdy;
 
    /* "Best effort" send */
    PCP_HDR_BUILD(&hdr, PCP_WRITE_ERROR_MSG, src, dst, sizeof(pcp_w_error_bdy));
    bdy.rval = rval;
    locked_msg_send(sock, lock, &hdr, &bdy);
}

/* Send error to parent and wait for parent to close socket to this node */
static void send_pcp_tc_error_and_wait(int up_sock, pthread_mutex_t *up_sock_lock, 
                                       int src, int dst, int rval, int bad_node)
{
    char bit_bucket[8192];

    send_pcp_tc_error(up_sock, up_sock_lock, src, dst, rval, bad_node);
    for (;;)
        if (net_recv_bytes(up_sock, bit_bucket, 8192) != E_OK)
            break;
}

/* Create tree, open child sockets, and forward tree create request. */
static int pcp_tree_node_init(pcpd_state *s, pcpd_options *opts)
{
    pcp_hdr      hdr;
    credentials  creds;
    signature    creds_sig;
    void         *tbuf;
    int          tbuf_len, i, rval;
    int          bad_child;

    if ((rval = recv_tree_create(s->up_sock, &hdr, &creds, &creds_sig,
                                 &tbuf, &tbuf_len)) != PCP_OK)
        return rval;
    s->self = hdr.dst;
    pcpd_tree_init(tbuf, tbuf_len, s->self, &s->tree, &s->parent, 
                   &s->nchildren, &s->child_nodes, &s->child_ips);
    if ((rval = pcpd_authenticate(&creds, &creds_sig, &s->uid, 
                                  &s->gid)) != PCP_OK) {
        send_pcp_tc_error_and_wait(s->up_sock, &s->up_sock_lock, s->self, 
                                   ROOT, rval, -1);
        goto cleanup;
    }
    if ((rval = open_child_socks(&s->child_socks, s->nchildren, s->child_ips, 
                                 opts->port, &bad_child)) != PCP_OK) {
        send_pcp_tc_error_and_wait(s->up_sock, &s->up_sock_lock, s->self,   
                                   ROOT, rval, s->child_nodes[bad_child]);
        goto cleanup;
    }
    for (i = 0; i < s->nchildren; i++) {
        hdr.dst = s->child_nodes[i];
        if (send_tree_create(s->child_socks[i], &hdr, &creds, &creds_sig,
                             tbuf, tbuf_len) != PCP_OK) {
            send_pcp_tc_error_and_wait(s->up_sock, &s->up_sock_lock, s->self, 
                                       ROOT, rval, s->child_nodes[i]);
            goto cleanup;
        }
    }
    xfree(tbuf);
    return PCP_OK;

 cleanup:
    xfree(tbuf);
    return rval;
}

/* Compute SHA-1 digest of filename and return in digest */ 
static int sha1(char *filename, unsigned char *digest)
{
    int         fd, file_len, bytes_read, nbytes;
    struct stat stat_buf;
    void        *buf;
    SHA_CTX     ctx;

    if (stat(filename, &stat_buf) < 0)
        return PCP_REMOTEFILE_ERROR;
    if ((fd = open(filename, O_RDONLY)) < 0)
        return PCP_REMOTEFILE_ERROR;
    file_len = stat_buf.st_size;

    SHA_Init(&ctx);
    bytes_read = 0;
    buf = xmalloc(PCP_DEF_FRAG_SIZE);
    while ((file_len - bytes_read) != 0) {
        if ((file_len - bytes_read) >= PCP_DEF_FRAG_SIZE)
            nbytes = PCP_DEF_FRAG_SIZE;
        else
            nbytes = file_len - bytes_read;
        if (io_read_bytes(fd, buf, nbytes) != E_OK) {
            xfree(buf);
            close(fd);
            return PCP_IO_ERROR;            
        }
        SHA_Update(&ctx, buf, nbytes);
        bytes_read += nbytes;
    }
    SHA_Final(digest, &ctx);
    xfree(buf);
    close(fd);
    return PCP_OK;
}

static int handle_pcp_write_chunk1(int up_sock, int *child_socks, int nchildren,
                                   pcp_hdr *hdr, uid_t uid, gid_t gid, int *fd, 
                                   int *frag_size, int *file_len)
{
    int         name_len, i, rval;
    char        *name = NULL;
    mode_t      file_mode;
    struct stat stat_buf;

    /* Recv frag_size, name_len, name, file_len from parent */
    if (net_recv_bytes(up_sock, frag_size, sizeof(*frag_size)) != E_OK) {
        rval = PCP_NET_ERROR;
        goto cleanup;
    }
    if (net_recv_bytes(up_sock, &name_len, sizeof(name_len)) != E_OK) {
        rval = PCP_NET_ERROR;
        goto cleanup;
    }
    name = (char *)xmalloc(name_len);
    if (net_recv_bytes(up_sock, (void *)name, name_len) != E_OK) {
        rval = PCP_NET_ERROR;
        goto cleanup;
    }
    if (net_recv_bytes(up_sock, file_len, sizeof(*file_len)) != E_OK) {
        rval = PCP_NET_ERROR;
        goto cleanup;
    }
    if (net_recv_bytes(up_sock, &file_mode, sizeof(file_mode)) != E_OK) {
        rval = PCP_NET_ERROR;
        goto cleanup;
    }

    /* Send hdr, frag_size, name_len, name, file_len to children */
    for (i = 0; i < nchildren; i++) {
        if (net_send_bytes(child_socks[i], hdr, sizeof(pcp_hdr)) != E_OK) {
            rval = PCP_NET_ERROR;
            goto cleanup;
        }
        if (net_send_bytes(child_socks[i], frag_size, sizeof(*frag_size)) != E_OK) {
            rval = PCP_NET_ERROR;
            goto cleanup;
        }
        if (net_send_bytes(child_socks[i], &name_len, sizeof(name_len)) != E_OK) {
            rval = PCP_NET_ERROR;
            goto cleanup;
        }
        if (net_send_bytes(child_socks[i], (void *)name, name_len) != E_OK) {
            rval = PCP_NET_ERROR;
            goto cleanup;
        }
        if (net_send_bytes(child_socks[i], file_len, sizeof(*file_len)) != E_OK) {
            rval = PCP_NET_ERROR;
            goto cleanup;
        }
        if (net_send_bytes(child_socks[i], &file_mode, sizeof(file_mode)) != E_OK) {
            rval = PCP_NET_ERROR;
            goto cleanup;
        }
    }

    /* Delete file if file already exists */
    if (stat(name, &stat_buf) == 0 && unlink(name) < 0) {
        rval = PCP_REMOTEFILE_PERM_ERROR;
        goto cleanup;
    }
    /* Open file for writing (file_mode always applies since file is new) */
    if (((*fd) = open(name, O_WRONLY | O_TRUNC | O_CREAT, file_mode)) < 0) {
        rval = PCP_REMOTEFILE_ERROR;
        goto cleanup;
    }
    xfree(name);

    return PCP_OK;

 cleanup:
    if (*fd != -1) 
        close(*fd);
    if (name != NULL) 
        xfree(name);
    return rval;
}

static int handle_pcp_write_chunk2(int up_sock, int *child_socks, int nchildren,
                                   int frag_size, int fd, int file_len)
{
    int     bytes_read, nbytes, i;
    void    *buf;

    bytes_read = 0;
    buf = xmalloc(frag_size);
    while ((file_len - bytes_read) != 0) {
        if ((file_len - bytes_read) >= frag_size)
            nbytes = frag_size;
        else
            nbytes = file_len - bytes_read;
        if (net_recv_bytes(up_sock, buf, nbytes) != E_OK) {
            xfree(buf);
            return PCP_NET_ERROR;
        }
        for (i = 0; i < nchildren; i++)
            if (net_send_bytes(child_socks[i], buf, nbytes) != E_OK) {
                xfree(buf);
                return PCP_NET_ERROR;
            }
        if (io_write_bytes(fd, buf, nbytes) != E_OK) {
            xfree(buf);
            return PCP_IO_ERROR;
        }
        bytes_read += nbytes;
    }
    xfree(buf);
    return PCP_OK;
}

static int handle_pcp_write(int up_sock, int *child_socks, int nchildren,
                            pcp_hdr *hdr, uid_t uid, gid_t gid)
{
    int rval;
    int fd = -1, frag_size, file_len;

    if ((rval = handle_pcp_write_chunk1(up_sock, child_socks, nchildren, hdr,
                                        uid, gid, &fd, &frag_size, 
                                        &file_len)) != PCP_OK)
        return rval;
    if ((rval = handle_pcp_write_chunk2(up_sock, child_socks, nchildren,
                                        frag_size, fd, file_len)) != PCP_OK) {
        close(fd);
        return rval;
    }
    close(fd);
    return PCP_OK;
}

static int handle_pcp_delete(int up_sock, int *child_socks, int nchildren,
                             pcp_hdr *hdr, uid_t uid, gid_t gid)
{
    int     name_len, i, rval;
    char    *name = NULL;

    /* Recv name_len, name from parent */
    if (net_recv_bytes(up_sock, &name_len, sizeof(name_len)) != E_OK) {
        rval = PCP_NET_ERROR;
        goto cleanup;
    }
    name = (char *)xmalloc(name_len);
    if (net_recv_bytes(up_sock, (void *)name, name_len) != E_OK) {
        rval = PCP_NET_ERROR;
        goto cleanup;
    }

    /* Send hdr, name_len, name to children */
    for (i = 0; i < nchildren; i++) {
        if (net_send_bytes(child_socks[i], hdr, sizeof(pcp_hdr)) != E_OK) {
            rval = PCP_NET_ERROR;
            goto cleanup;
        }
        if (net_send_bytes(child_socks[i], &name_len, sizeof(name_len)) != E_OK) {
            rval = PCP_NET_ERROR;
            goto cleanup;
        }
        if (net_send_bytes(child_socks[i], (void *)name, name_len) != E_OK) {
            rval = PCP_NET_ERROR;
            goto cleanup;
        }
    }

    /* Delete file */
    if (unlink(name) < 0) {
        if (errno == ENOENT)
            return PCP_REMOTEFILE_ERROR;
        else
            return PCP_REMOTEFILE_PERM_ERROR;
    }

    return PCP_OK;

 cleanup:
    if (name != NULL) 
        xfree(name);
    return rval;
}

static int handle_pcp_checksum(int up_sock, int *child_socks, int nchildren,
                               pcp_hdr *hdr, uid_t uid, gid_t gid, 
                               unsigned char *checksum)
{
    int         name_len, i, rval;
    char        *name = NULL;

    /* Recv name_len, name from parent */
    if (net_recv_bytes(up_sock, &name_len, sizeof(name_len)) != E_OK) {
        rval = PCP_NET_ERROR;
        goto cleanup;
    }
    name = (char *)xmalloc(name_len);
    if (net_recv_bytes(up_sock, (void *)name, name_len) != E_OK) {
        rval = PCP_NET_ERROR;
        goto cleanup;
    }

    /* Send hdr, name_len, name to children */
    for (i = 0; i < nchildren; i++) {
        if (net_send_bytes(child_socks[i], hdr, sizeof(pcp_hdr)) != E_OK) {
            rval = PCP_NET_ERROR;
            goto cleanup;
        }
        if (net_send_bytes(child_socks[i], &name_len, sizeof(name_len)) != E_OK) {
            rval = PCP_NET_ERROR;
            goto cleanup;
        }
        if (net_send_bytes(child_socks[i], (void *)name, name_len) != E_OK) {
            rval = PCP_NET_ERROR;
            goto cleanup;
        }
    }

    /* Compute SHA-1 digest */
    if ((rval = sha1(name, checksum)) != PCP_OK)
        goto cleanup;

    return PCP_OK;

 cleanup:   
    if (name != NULL) 
        xfree(name);
    return rval;
}

static int route_up_handler(int sock, int up_sock, pthread_mutex_t *lock, int self)
{
    pcp_hdr             hdr;
    void                *bdy;
    pcp_w_reply_bdy     w_bdy;
    pcp_d_reply_bdy     d_bdy;
    pcp_c_reply_bdy     c_bdy;
    pcp_tc_error_bdy    tce_bdy;
    pcp_w_error_bdy     we_bdy;
    pcp_d_error_bdy     de_bdy;
    pcp_c_error_bdy     ce_bdy;
    pcp_hb_bdy          hb_bdy;

    if (net_recv_bytes(sock, (void *)&hdr, sizeof(pcp_hdr)) != E_OK)
        return PCP_NET_ERROR;

    switch (hdr.type) {

    case PCP_WRITE_REPLY_MSG:
        if (net_recv_bytes(sock, (void *)&w_bdy, sizeof(w_bdy)) != E_OK)
            return PCP_NET_ERROR;
        bdy = (void *)&w_bdy;
        break;

    case PCP_DELETE_REPLY_MSG:
        e_assert(hdr.bdy_len == sizeof(d_bdy));
        if (net_recv_bytes(sock, (void *)&d_bdy, sizeof(d_bdy)) != E_OK)
            return PCP_NET_ERROR;
        bdy = (void *)&d_bdy;
        break;

    case PCP_CHECKSUM_REPLY_MSG:
        e_assert(hdr.bdy_len == sizeof(c_bdy));
        if (net_recv_bytes(sock, (void *)&c_bdy, sizeof(c_bdy)) != E_OK)
            return PCP_NET_ERROR;
        bdy = (void *)&c_bdy;
        break;

    case PCP_TREE_CREATE_ERROR_MSG:
        e_assert(hdr.bdy_len == sizeof(tce_bdy));
        if (net_recv_bytes(sock, (void *)&tce_bdy, sizeof(tce_bdy)) != E_OK)
            return PCP_NET_ERROR;
        bdy = (void *)&tce_bdy;
        break;

    case PCP_WRITE_ERROR_MSG:
        e_assert(hdr.bdy_len == sizeof(we_bdy));
        if (net_recv_bytes(sock, (void *)&we_bdy, sizeof(we_bdy)) != E_OK)
            return PCP_NET_ERROR;
        bdy = (void *)&we_bdy;
        break;

    case PCP_DELETE_ERROR_MSG:
        e_assert(hdr.bdy_len == sizeof(de_bdy));
        if (net_recv_bytes(sock, (void *)&de_bdy, sizeof(de_bdy)) != E_OK)
            return PCP_NET_ERROR;
        bdy = (void *)&de_bdy;
        break;

    case PCP_CHECKSUM_ERROR_MSG:
        e_assert(hdr.bdy_len == sizeof(ce_bdy));
        if (net_recv_bytes(sock, (void *)&ce_bdy, sizeof(ce_bdy)) != E_OK)
            return PCP_NET_ERROR;
        bdy = (void *)&ce_bdy;
        break;

    case PCP_HEARTBEAT_MSG:
        if (net_recv_bytes(sock, (void *)&hb_bdy, sizeof(hb_bdy)) != E_OK)
            return PCP_NET_ERROR;
        bdy = (void *)&hb_bdy;
        break;

    default:
        return PCP_INTERNAL_ERROR;
    }

    /* Route up only if this node isn't packet final destination */
    if (hdr.dst != self) 
        if (locked_msg_send(up_sock, lock, &hdr, bdy) != PCP_OK)
            return PCP_NET_ERROR;

    return PCP_OK;
}

static void *route_down_thr(void *arg)
{
    pcpd_state      *state = (pcpd_state *)arg;
    pcp_hdr         hdr;
    unsigned char   checksum[PCP_CHECKSUM_SIZE];
    int             rval;

    /* Run all commands with user's GID/UID */
    if (setegid(state->gid) < 0 || seteuid(state->uid) < 0)
        goto cleanup;

    for (;;) {
        if (net_recv_bytes(state->up_sock, (void *)&hdr, sizeof(pcp_hdr)) != E_OK)
            goto cleanup;

        switch (hdr.type) {

        case PCP_WRITE_MSG:
            rval = handle_pcp_write(state->up_sock, state->child_socks, 
                                    state->nchildren, &hdr, state->uid, state->gid);
            send_pcp_write_reply(state->up_sock, &state->up_sock_lock, state->self, 
                                 ROOT, rval);
            break;

        case PCP_DELETE_MSG:
            rval = handle_pcp_delete(state->up_sock, state->child_socks, 
                                     state->nchildren, &hdr, state->uid, state->gid);
            send_pcp_delete_reply(state->up_sock, &state->up_sock_lock, state->self, 
                                  ROOT, rval);
            break;

        case PCP_CHECKSUM_MSG:
            rval = handle_pcp_checksum(state->up_sock, state->child_socks,
                                       state->nchildren, &hdr, state->uid, state->gid,
                                       checksum);
            send_pcp_checksum_reply(state->up_sock, &state->up_sock_lock, state->self, 
                                    ROOT, rval, checksum);
            break;

        default:
            goto cleanup;
        }
    }
 cleanup:   
    e_assert(setegid(0) == 0); /* Back to root for pthread_join */
    e_assert(seteuid(0) == 0); /* Back to root for pthread_join */
    sem_post(&state->cleanup_sem);
    sem_wait(&state->killme_sem);
    return NULL;
}

static void *route_up_thr(void *arg)
{
    pcpd_state      *state = (pcpd_state *)arg;
    fd_set          readfds, readfds_orig;
    int             max_sock = 0, i, n;
    struct timeval  timeout;

    FD_ZERO(&readfds_orig);
    for (i = 0; i < state->nchildren; i++) {
        FD_SET(state->child_socks[i], &readfds_orig);
        if (state->child_socks[i] > max_sock)
            max_sock = state->child_socks[i];
    }

    for (;;) {
        readfds = readfds_orig;
        timeout.tv_sec  = 0;
        timeout.tv_usec = 250000; /* For cancellation check */
        if ((n = select(max_sock + 1, &readfds, NULL, NULL, &timeout)) < 0)
            goto cleanup;
        /* Route reply packets upstream (if available) */
        for (i = 0; i < state->nchildren; i++)
            if (FD_ISSET(state->child_socks[i], &readfds))
                if (route_up_handler(state->child_socks[i], state->up_sock,
                                     &state->up_sock_lock, state->self) != PCP_OK)
                    goto cleanup;
        if (n == 0) pthread_testcancel();
    }
 cleanup:
    sem_post(&state->cleanup_sem);
    sem_wait(&state->killme_sem);
    return NULL;
}

static void *heartbeat_thr(void *arg)
{
    pcpd_state      *s = (pcpd_state *)arg;
    pcp_hdr         hdr;
    pcp_hb_bdy      hb_bdy;
    pthread_cond_t  wait_cv   = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t wait_lock = PTHREAD_MUTEX_INITIALIZER;
    struct timespec timeout;

    PCP_HDR_BUILD(&hdr, PCP_HEARTBEAT_MSG, s->self, s->parent, sizeof(hb_bdy));

    for (;;) {
        timeout.tv_sec  = time(NULL) + PCPD_HEARTBEAT_INT;
        timeout.tv_nsec = 0;
        
        pthread_mutex_lock(&wait_lock);
        pthread_cleanup_push(mutex_cleanup, (void *)&wait_lock);
        pthread_cond_timedwait(&wait_cv, &wait_lock, &timeout);
        pthread_cleanup_pop(0);
        pthread_mutex_unlock(&wait_lock);

        if (locked_msg_send(s->up_sock, &s->up_sock_lock, &hdr, &hb_bdy) != PCP_OK)
            goto cleanup;
    }
 cleanup:
    sem_post(&s->cleanup_sem);
    sem_wait(&s->killme_sem);
    return NULL;
}

static void *pcpd_thr(void *arg)
{
    pcpd_thr_args   *args = (pcpd_thr_args *)arg;
    pcpd_options    *opts;
    pcpd_state      *state;
    pthread_attr_t  attr;

    pcpd_state_create(&state);
    pthread_attr_init(&attr);
    state->up_sock = args->up_sock;
    opts           = args->opts;
    xfree(args);

    if (pcp_tree_node_init(state, opts) != PCP_OK)
        goto cleanup;

    if (pthread_create(&state->route_down_tid, &attr, route_down_thr, 
                       (void *)state) != 0)
        goto cleanup;
    if (pthread_create(&state->route_up_tid, &attr, route_up_thr, 
                       (void *)state) != 0)
        goto cleanup;
    if (pthread_create(&state->heartbeat_tid, &attr, heartbeat_thr, 
                       (void *)state) != 0)
        goto cleanup;
    sem_wait(&state->cleanup_sem);

 cleanup:
    pthread_attr_destroy(&attr);
    pcpd_state_destroy(state);
    return NULL;
}

void pcpd_standalone(pcpd_options *opts)
{
    int                 svr_sock;
    int                 cli_sock;
    pthread_attr_t      attr;
    pthread_t           tid;
    socklen_t           sockaddr_len = sizeof(struct sockaddr_in);
    struct sockaddr_in  sockaddr; 
    pcpd_thr_args       *args;

    if (net_svr_sock_create(&svr_sock, opts->port) != E_OK) {
        fprintf(stderr, "Bind error on TCP socket on port %d\n", opts->port);
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
        args = (pcpd_thr_args *)xmalloc(sizeof(pcpd_thr_args));
        args->up_sock = cli_sock;
        args->opts    = opts;
        if (pthread_create(&tid, &attr, pcpd_thr, (void *)args) != 0)
            close(cli_sock);
    }
    pthread_attr_destroy(&attr);
    net_svr_sock_destroy(svr_sock);
}

int main(int argc, char **argv)
{
    pcpd_options  opts;
    pcpd_thr_args *args;

    pcpd_options_init(&opts);
    pcpd_options_parse(&opts, argc, argv);

    signal(SIGPIPE, SIG_IGN);

    if (opts.inetd) {
        args = (pcpd_thr_args *)xmalloc(sizeof(pcpd_thr_args));
        args->up_sock = STDIN_FILENO;
        args->opts    = &opts;
        pcpd_thr(args);
        xfree(args);
    } else {
	if (opts.daemon)
	    daemon(0, 0);
        pcpd_standalone(&opts);
    }

    return 0;
}
