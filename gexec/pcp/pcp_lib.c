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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <pthread.h>
#include <semaphore.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <auth.h>
#include <e/bytes.h>
#include <e/e_error.h>
#include <e/io.h>
#include <e/net.h>
#include <e/tree.h>
#include <e/xmalloc.h>
#include "msg.h"
#include "pcp_lib.h"

static void pcp_state_create(pcp_state **s)
{
    (*s) = (pcp_state *)xmalloc(sizeof(pcp_state));

    (*s)->send_tid = -1;
    (*s)->recv_tid = -1;

    (*s)->tree        = NULL;
    (*s)->self        = -1;
    (*s)->nchildren   = 0;
    (*s)->child_nodes = NULL; 
    (*s)->child_ips   = NULL;

    (*s)->child_socks = NULL;

    (*s)->cmd_type   = PCP_NULL_CMD;
    (*s)->cmd        = NULL;
    (*s)->cmd_opts   = NULL;
    (*s)->cmd_rval   = PCP_CMD_PENDING;

    (*s)->ndone      = 0;       
    if (sem_init(&(*s)->cleanup_sem, 0, 0) != 0) {
        fatal_error("sem_init error\n");
        exit(1);
    }
    if (sem_init(&(*s)->killme_sem, 0, 0) != 0) {
        fatal_error("sem_init error\n");
        exit(1);
    }
}

static void pcp_state_destroy(pcp_state *s)
{
    int i;
    
    if (s->send_tid != -1) {
        e_assert(pthread_cancel(s->send_tid) == 0);
        e_assert(pthread_join(s->send_tid, NULL) == 0);
    }
    if (s->recv_tid != -1) {
        e_assert(pthread_cancel(s->recv_tid) == 0);
        e_assert(pthread_join(s->recv_tid, NULL) == 0);
    }
    
    if (s->tree != NULL)
        tree_destroy(s->tree);
    if (s->child_nodes != NULL)
        xfree(s->child_nodes);
    if (s->child_ips != NULL)
        xfree(s->child_ips);

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

static void pcp_write_cmd_create(pcp_write_cmd **cmd,
                                 char *localfile,
                                 char *remotefile)
{
    int len;

    (*cmd) = (pcp_write_cmd *)xmalloc(sizeof(pcp_write_cmd));
    len = strlen(localfile) + NULLBYTE_SIZE;
    (*cmd)->localfile = (char *)xmalloc(len);
    strcpy((*cmd)->localfile, localfile);
    len = strlen(remotefile) + NULLBYTE_SIZE;
    (*cmd)->remotefile = (char *)xmalloc(len);
    strcpy((*cmd)->remotefile, remotefile);
}

static void pcp_write_cmd_destroy(pcp_write_cmd *cmd)
{
    xfree(cmd->localfile);
    xfree(cmd->remotefile);
    xfree(cmd);
}

static void pcp_delete_cmd_create(pcp_delete_cmd **cmd,
                                  char *remotefile)
{
    int len;

    (*cmd) = (pcp_delete_cmd *)xmalloc(sizeof(pcp_delete_cmd));
    len = strlen(remotefile) + NULLBYTE_SIZE;
    (*cmd)->remotefile = (char *)xmalloc(len);
    strcpy((*cmd)->remotefile, remotefile);
}

static void pcp_delete_cmd_destroy(pcp_delete_cmd *cmd)
{
    xfree(cmd->remotefile);
    xfree(cmd);
}

static void pcp_checksum_cmd_create(pcp_checksum_cmd **cmd, 
                                    char *remotefile)
{
    int len;

    (*cmd) = (pcp_checksum_cmd *)xmalloc(sizeof(pcp_checksum_cmd));
    len = strlen(remotefile) + NULLBYTE_SIZE;
    (*cmd)->remotefile = (char *)xmalloc(len);
    strcpy((*cmd)->remotefile, remotefile);
}

static void pcp_checksum_cmd_destroy(pcp_checksum_cmd *cmd)
{
    xfree(cmd->remotefile);
    xfree(cmd);
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

static int pcp_tree_init(int nhosts, char **ips, int fanout, tree **t, int *self, 
                         int *nchildren, int **child_nodes, char ***child_ips)
{
    int  i, rval;
    char ip[16];

    if (net_gethostip(ip) != E_OK)
        return PCP_NET_ERROR;

    if ((rval = tree_create(t, nhosts, ips, fanout, 0)) != E_OK) {
        fprintf(stderr, "Each host IP address in list must be unique\n");
        return rval;
    }

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
    return PCP_OK;
}

static int open_child_socks(int **child_socks, int nchildren, char **child_ips,
                            int port)
{
    int  i, j;
    char host[1024];

    if (nchildren == 0)
        return PCP_OK;
    (*child_socks) = xmalloc(nchildren * sizeof(int));
    for (i = 0; i < nchildren; i++) 
        (*child_socks)[i] = -1;
    for (i = 0; i < nchildren; i++) 
        if (net_cli_sock_create(&(*child_socks)[i], child_ips[i], 
                                port) != E_OK) {
            net_iptohost(child_ips[i], host, 1024);
            fprintf(stderr, "Could not connect to %s:%d (%s:%d)\n", host, 
                    port, child_ips[i], port);
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

static int send_tree_create_msg(int sock, pcp_hdr *hdr, void *tbuf,
                                int tbuf_len, credentials *creds, 
                                signature *creds_sig)
{
    if (net_send_bytes(sock, (void *)hdr, sizeof(pcp_hdr)) != E_OK)
        return PCP_NET_ERROR;
    if (net_send_bytes(sock, (void *)creds, sizeof(credentials)) != E_OK)
        return PCP_NET_ERROR;
    if (net_send_bytes(sock, (void *)creds_sig, sizeof(signature)) != E_OK)
        return PCP_NET_ERROR;
    if (net_send_bytes(sock, tbuf, tbuf_len) != E_OK)
        return PCP_NET_ERROR;
    return PCP_OK;
}

static int send_tree_create(int *child_socks, char **child_ips, int *child_nodes,
                            int nchildren, tree *t, credentials *creds, 
                            signature *creds_sig)
{
    pcp_hdr hdr;
    int     tbuf_len, i, rval;
    void    *tbuf;
    char    host[1024];

    tbuf = tree_malloc_buf(t, &tbuf_len);
    
    hdr.type    = PCP_TREE_CREATE_MSG;
    hdr.src     = ROOT;

    hdr.bdy_len  = 0;
    hdr.bdy_len += sizeof(credentials);
    hdr.bdy_len += sizeof(signature);
    hdr.bdy_len += tbuf_len;

    for (i = 0; i < nchildren; i++) {
        hdr.dst = child_nodes[i];
        if ((rval = send_tree_create_msg(child_socks[i], &hdr, tbuf, tbuf_len,
                                         creds, creds_sig)) != PCP_OK) {
            net_iptohost(child_ips[i], host, 1024);
            fprintf(stderr, "Error sending to %s (%s)\n", host, child_ips[i]);
            xfree(tbuf);
            return rval;
        }
    }
    xfree(tbuf);
    return PCP_OK;
}

/* Create tree (including root) and open child sockets */
static int pcp_tree_root_init(pcp_state *s, int nhosts, char **ips, int port,
                              int fanout)
{
    int         rval;
    credentials creds;
    signature   creds_sig;

    /* Create tree locally */
    if ((rval = pcp_tree_init(nhosts, ips, fanout, &s->tree, &s->self, 
                              &s->nchildren, &s->child_nodes,
                              &s->child_ips)) != PCP_OK) {
        return rval;
    }

    /* Open child sockets */
    if ((rval = open_child_socks(&s->child_socks, s->nchildren, 
                                 s->child_ips, port)) != PCP_OK) {
        return rval;
    }

    /* Obtain credentials from authd on localhost */
    auth_init_credentials(&creds, PCP_CRED_LIFETIME);
    if (auth_get_signature(&creds, &creds_sig) != AUTH_OK) {
        fprintf(stderr, "Could not obtain signature from authd on localhost\n");
        return PCP_AUTH_ERROR;
    }

    /* Send tree create msg to children (including credentials) */
    if ((rval = send_tree_create(s->child_socks, s->child_ips, s->child_nodes,
                                 s->nchildren, s->tree, &creds, 
                                 &creds_sig)) != PCP_OK)
        return rval;

    return PCP_OK;
}

static void print_tick(int bytes_read, int tick_size, int *old_num_ticks)
{
    int new_num_ticks;
    int i, num_ticks;

    new_num_ticks = bytes_read / tick_size;
    if (new_num_ticks > *old_num_ticks) {
        num_ticks = (new_num_ticks - *old_num_ticks);
        for (i = 0; i < num_ticks; i++)
            printf("#");
        *old_num_ticks = new_num_ticks;
    }
    fflush(stdout);
}

static int send_pcp_write_chunk1(int sock, pcp_hdr *hdr, int *frag_size,
                                 int *name_len, char *name, int *file_len,
                                 mode_t *file_mode)
{
    if (net_send_bytes(sock, (void *)hdr, sizeof(pcp_hdr)) != E_OK)
        return PCP_NET_ERROR;
    if (net_send_bytes(sock, (void *)frag_size, sizeof(*frag_size)) != E_OK)
        return PCP_NET_ERROR;
    if (net_send_bytes(sock, (void *)name_len, sizeof(*name_len)) != E_OK)
        return PCP_NET_ERROR;
    if (net_send_bytes(sock, (void *)name, *name_len) != E_OK)
        return PCP_NET_ERROR;
    if (net_send_bytes(sock, (void *)file_len, sizeof(*file_len)) != E_OK)
        return PCP_NET_ERROR;
    if (net_send_bytes(sock, (void *)file_mode, sizeof(*file_mode)) != E_OK)
        return PCP_NET_ERROR;
    return PCP_OK;
}

static int send_pcp_write_chunk2(int *child_socks, char **child_ips, 
                                 int nchildren, char *localfile, int file_len, 
                                 pcp_cmd_options *opts)
{
    int  i, fd, bytes_read, nbytes;
    int  num_ticks = 0;
    int  tick_size = file_len / PCP_NUM_TICKS;
    void *buf;
    char host[1024];

    if ((fd = open(localfile, O_RDONLY)) < 0) {
        fprintf(stderr, "Could not open localfile %s\n", localfile);
        return PCP_LOCALFILE_ERROR;
    }

    bytes_read = 0;
    buf = xmalloc(opts->frag_size);
    while ((file_len - bytes_read) != 0) {
        if ((file_len - bytes_read) >= opts->frag_size)
            nbytes = opts->frag_size;
        else
            nbytes = file_len - bytes_read;
        if (io_read_bytes(fd, buf, nbytes) != E_OK) {
            fprintf(stderr, "Error reading from localfile %s\n", localfile);
            xfree(buf);
            close(fd);
            return PCP_IO_ERROR;            
        }
        for (i = 0; i < nchildren; i++)
            if (net_send_bytes(child_socks[i], buf, nbytes) != E_OK) {
                net_iptohost(child_ips[i], host, 1024);
                fprintf(stderr, "Error sending to %s (%s)\n", host, child_ips[i]);
                xfree(buf);
                close(fd);
                return PCP_NET_ERROR;
            }
        bytes_read += nbytes;
        if (opts->verbose)
            print_tick(bytes_read, tick_size, &num_ticks);
    }
    printf("\n"); fflush(stdout);
    xfree(buf);
    close(fd);
    return PCP_OK;
}

static int send_pcp_write(int *child_socks, char **child_ips, int nchildren, 
                          int src, int dst, char *localfile, char *remotefile, 
                          pcp_cmd_options *opts)
{
    pcp_hdr     hdr;
    int         i, frag_size, name_len, file_len, rval;
    char        host[1024];
    struct stat stat_buf;

    if (stat(localfile, &stat_buf) < 0) {
        fprintf(stderr, "Could not open localfile %s\n", localfile);
        return PCP_LOCALFILE_ERROR;
    }
    frag_size = opts->frag_size;
    name_len  = strlen(remotefile) + NULLBYTE_SIZE;
    file_len  = stat_buf.st_size;

    hdr.type    = PCP_WRITE_MSG;
    hdr.src     = src;
    hdr.dst     = dst;

    hdr.bdy_len  = 0;
    hdr.bdy_len += sizeof(frag_size);
    hdr.bdy_len += sizeof(name_len);
    hdr.bdy_len += name_len;
    hdr.bdy_len += sizeof(file_len);
    hdr.bdy_len += file_len;

    for (i = 0; i < nchildren; i++) 
        if ((rval = send_pcp_write_chunk1(child_socks[i], &hdr, &frag_size,
                                          &name_len, remotefile, &file_len, 
                                          &stat_buf.st_mode)) != PCP_OK) {
            net_iptohost(child_ips[i], host, 1024);
            fprintf(stderr, "Error sending to %s (%s)\n", host, child_ips[i]);
            return rval;
        }
    if ((rval = send_pcp_write_chunk2(child_socks, child_ips, nchildren, 
                                      localfile, file_len, opts)) != PCP_OK)
        return rval;

    return PCP_OK;
}

static int send_pcp_remotefile_msg(int sock, pcp_hdr *hdr, 
                                   int *name_len, char *name)

{
    if (net_send_bytes(sock, (void *)hdr, sizeof(pcp_hdr)) != E_OK)
        return PCP_NET_ERROR;
    if (net_send_bytes(sock, (void *)name_len, sizeof(*name_len)) != E_OK)
        return PCP_NET_ERROR;
    if (net_send_bytes(sock, (void *)name, *name_len) != E_OK)
        return PCP_NET_ERROR;
    return PCP_OK;
}

static int send_pcp_delete(int *child_socks, char **child_ips, int nchildren, 
                           int src, int dst, char *remotefile)
{
    pcp_hdr hdr;
    int     i, name_len, rval;
    char    host[1024];

    name_len = strlen(remotefile) + NULLBYTE_SIZE;

    hdr.type    = PCP_DELETE_MSG;
    hdr.src     = src;
    hdr.dst     = dst;

    hdr.bdy_len  = 0;
    hdr.bdy_len += sizeof(name_len);
    hdr.bdy_len += name_len;

    for (i = 0; i < nchildren; i++)
        if ((rval = send_pcp_remotefile_msg(child_socks[i], &hdr, &name_len, 
                                            remotefile)) != PCP_OK) {
            net_iptohost(child_ips[i], host, 1024);
            fprintf(stderr, "Error sending to %s (%s)\n", host, child_ips[i]);
            return rval;
        }
    return PCP_OK;
}

static int send_pcp_checksum(int *child_socks, char **child_ips, int nchildren, 
                             int src, int dst, char *remotefile)
{
    pcp_hdr hdr;
    int     i, name_len, rval;
    char    host[1024];

    name_len = strlen(remotefile) + NULLBYTE_SIZE;

    hdr.type    = PCP_CHECKSUM_MSG;
    hdr.src     = src;
    hdr.dst     = dst;

    hdr.bdy_len  = 0;
    hdr.bdy_len += sizeof(name_len);
    hdr.bdy_len += name_len;

    for (i = 0; i < nchildren; i++)
        if ((rval = send_pcp_remotefile_msg(child_socks[i], &hdr, &name_len, 
                                            remotefile)) != PCP_OK) {
            net_iptohost(child_ips[i], host, 1024);
            fprintf(stderr, "Error sending to %s (%s)\n", host, child_ips[i]);
            return rval;
        }
    return PCP_OK;
}

static int handle_pcp_write_reply(int child_sock, char **ips, pcp_hdr *hdr)
{
    pcp_w_reply_bdy bdy;
    char            *ip;
    char            host[1024];
    
    if (net_recv_bytes(child_sock, (void *)&bdy, sizeof(bdy)) != E_OK)
        return PCP_NET_ERROR;

    ip = ips[hdr->src];
    net_iptohost(ip, host, 1024);
    if (bdy.rval == PCP_OK)
        printf("Write succeeded on %s (%s)\n", host, ip);
    else
        printf("Write failed on %s (%s)\n", host, ip);
    return PCP_OK;
}

static int handle_pcp_delete_reply(int child_sock, char **ips, pcp_hdr *hdr)
{
    pcp_d_reply_bdy bdy;
    char            *ip;
    char            host[1024];
    
    if (net_recv_bytes(child_sock, (void *)&bdy, sizeof(bdy)) != E_OK)
        return PCP_NET_ERROR;

    ip = ips[hdr->src];
    net_iptohost(ip, host, 1024);

    switch (bdy.rval) {
        
    case PCP_OK:
        printf("Delete succeeded on %s (%s)\n", host, ip);
        break;

    case PCP_REMOTEFILE_ERROR:
        printf("Delete failed on %s (%s) (file does not exist)\n", 
               host, ip);
        break;

    case PCP_REMOTEFILE_PERM_ERROR:
        printf("Delete failed on %s (%s) (permissions error)\n", 
               host, ip);
        break;

    default:
        printf("Delete failed on %s (%s)\n", host, ip);
        break;
    }

    return PCP_OK;
}

static int handle_pcp_checksum_reply(int child_sock, char **ips, pcp_hdr *hdr)
{
    pcp_c_reply_bdy bdy;
    int             i;
    char            *ip;
    char            host[1024];
    
    if (net_recv_bytes(child_sock, (void *)&bdy, sizeof(bdy)) != E_OK)
        return PCP_NET_ERROR;

    ip = ips[hdr->src];
    net_iptohost(ip, host, 1024);

    switch (bdy.rval) {

    case PCP_OK:
        printf("Checksum succeeded on %s (%s)\n", host, ip);
        printf("   SHA-1 = ");
        for (i = 0; i < SHA_DIGEST_LENGTH; i++)
            printf("%02x", bdy.checksum[i]);
        printf("\n");
        break;

    case PCP_REMOTEFILE_ERROR:
        printf("Checksum failed on %s (%s) (file does not exist)\n",
               host, ip);
        break;

    default:
        printf("Checksum failed on %s (%s)\n", host, ip);
        break;
    }

    return PCP_OK;
}

static int handle_pcp_tc_error(int child_sock, char **ips, pcp_hdr *hdr)
{
    pcp_tc_error_bdy bdy;
    char            *ip;
    char             host[1024];

    if (net_recv_bytes(child_sock, (void *)&bdy, sizeof(bdy)) != E_OK)
        return PCP_NET_ERROR;

    ip = ips[hdr->src];

    switch (bdy.rval) {

    case PCP_AUTH_ERROR:
        net_iptohost(ip, host, 1024);
        fprintf(stderr, "RSA authentication error on %s (%s)\n", host, ip);
        break;

    case PCP_NET_ERROR:
        net_iptohost(ips[bdy.bad_node], host, 1024);
        fprintf(stderr, "Could not connect to %s (%s)\n", host, ips[bdy.bad_node]);
        break;

    }
    return bdy.rval;
}

static int handle_pcp_write_error(int sock, char **ips, pcp_hdr *hdr)
{
    pcp_w_error_bdy bdy;
    
    if (net_recv_bytes(sock, (void *)&bdy, sizeof(bdy)) != E_OK)
        return PCP_NET_ERROR;

    /* NOTE: add messages */
    return bdy.rval;
}

static int handle_pcp_delete_error(int sock, char **ips, pcp_hdr *hdr)
{
    pcp_d_error_bdy bdy;
    
    if (net_recv_bytes(sock, (void *)&bdy, sizeof(bdy)) != E_OK)
        return PCP_NET_ERROR;

    /* NOTE: add messages */
    return bdy.rval;
}

static int handle_pcp_checksum_error(int sock, char **ips, pcp_hdr *hdr)
{
    pcp_c_error_bdy bdy;
    
    if (net_recv_bytes(sock, (void *)&bdy, sizeof(bdy)) != E_OK)
        return PCP_NET_ERROR;

    /* NOTE: add messages */
    return bdy.rval;
}

static int handle_pcp_heartbeat(int sock, char **ips, pcp_hdr *hdr)
{
    pcp_hb_bdy bdy;
    
    if (net_recv_bytes(sock, (void *)&bdy, sizeof(bdy)) != E_OK)
        return PCP_NET_ERROR;

    /* NOTE: add messages */
    return PCP_OK;
}

static int recv_handler(int child_sock, char *child_ip, char **ips, int *ndone)
{
    pcp_hdr hdr;
    
    if (net_recv_bytes(child_sock, (void *)&hdr, sizeof(pcp_hdr)) != E_OK)
        return PCP_NET_ERROR;

    switch (hdr.type) {

    case PCP_WRITE_REPLY_MSG:
        (*ndone)++;
        return handle_pcp_write_reply(child_sock, ips, &hdr);

    case PCP_DELETE_REPLY_MSG:
        (*ndone)++;
        return handle_pcp_delete_reply(child_sock, ips, &hdr);

    case PCP_CHECKSUM_REPLY_MSG:
        (*ndone)++;
        return handle_pcp_checksum_reply(child_sock, ips, &hdr);

    case PCP_TREE_CREATE_ERROR_MSG:
        return handle_pcp_tc_error(child_sock, ips, &hdr);

    case PCP_WRITE_ERROR_MSG:
        return handle_pcp_write_error(child_sock, ips, &hdr);

    case PCP_DELETE_ERROR_MSG:
        return handle_pcp_delete_error(child_sock, ips, &hdr);

    case PCP_CHECKSUM_ERROR_MSG:
        return handle_pcp_checksum_error(child_sock, ips, &hdr);

    case PCP_HEARTBEAT_MSG:
        return handle_pcp_heartbeat(child_sock, ips, &hdr);

    default:
        return PCP_INTERNAL_ERROR;
    }
}

static void *send_thr(void *arg)
{
    pcp_state         *state = (pcp_state *)arg;
    pcp_write_cmd     *w_cmd;
    pcp_delete_cmd    *d_cmd;
    pcp_checksum_cmd  *c_cmd;

    /* Do the command by pushing data down the tree */
    switch (state->cmd_type) {

    case PCP_WRITE_CMD:
        w_cmd = (pcp_write_cmd *)state->cmd;
        state->cmd_rval = send_pcp_write(state->child_socks, state->child_ips, 
                                         state->nchildren, state->self, PCP_DST_ALL, 
                                         w_cmd->localfile, w_cmd->remotefile, 
                                         state->cmd_opts);
        break;

    case PCP_DELETE_CMD:
        d_cmd = (pcp_delete_cmd *)state->cmd;
        state->cmd_rval = send_pcp_delete(state->child_socks, state->child_ips, 
                                          state->nchildren, state->self, PCP_DST_ALL, 
                                          d_cmd->remotefile);
        break;

    case PCP_CHECKSUM_CMD:
        c_cmd = (pcp_checksum_cmd *)state->cmd;
        state->cmd_rval = send_pcp_checksum(state->child_socks, state->child_ips,
                                            state->nchildren, state->self, PCP_DST_ALL, 
                                            c_cmd->remotefile);
        break;
    }
    if (state->cmd_rval != PCP_OK)
        goto cleanup;

    sem_wait(&state->killme_sem);
    return NULL;

 cleanup:
    sem_post(&state->cleanup_sem);
    sem_wait(&state->killme_sem);
    return NULL; 
}

static void *recv_thr(void *arg)
{
    pcp_state       *state = (pcp_state *)arg;
    fd_set          readfds, readfds_orig;
    int             max_sock = 0, i, n;
    struct timeval  timeout;

    /* Receive/handle any messages sent from down the tree */
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
            goto done;
        for (i = 0; i < state->nchildren; i++)
            if (FD_ISSET(state->child_socks[i], &readfds)) {
                if (recv_handler(state->child_socks[i], state->child_ips[i], 
                                 state->tree->ips, &state->ndone) != PCP_OK)
                    goto done;
                if (state->ndone == (state->tree->nhosts - 1))
                    goto done;
            }
        if (n == 0) pthread_testcancel();
    }
 done:
    sem_post(&state->cleanup_sem);
    sem_wait(&state->killme_sem);
    return NULL; 
}

static int pcp_cmd(int cmd_type, void *cmd, pcp_cmd_options *opts, 
                   int nhosts, char **ips)
{
    int             rval, nhosts_wr;
    char            **ips_wr;
    pcp_state       *state;
    pthread_attr_t  attr;

    pcp_state_create(&state);
    pthread_attr_init(&attr);
    state->cmd_type = cmd_type;
    state->cmd      = cmd;
    state->cmd_opts = opts;

    ips_with_root_create(nhosts, ips, &nhosts_wr, &ips_wr);
    if ((rval = pcp_tree_root_init(state, nhosts_wr, ips_wr, opts->port, 
                                   opts->fanout)) != PCP_OK)
        goto cleanup;

    if (pthread_create(&state->send_tid, &attr, send_thr, (void *)state) != 0) {
        rval = PCP_PTHREADS_ERROR;
        goto cleanup;
    }
    if (pthread_create(&state->recv_tid, &attr, recv_thr, (void *)state) != 0) {
        rval = PCP_PTHREADS_ERROR;
        goto cleanup;
    }

    sem_wait(&state->cleanup_sem);
    rval = state->cmd_rval;

    ips_with_root_destroy(nhosts_wr, ips_wr);
    pthread_attr_destroy(&attr);
    pcp_state_destroy(state);
    return rval;

 cleanup:
    ips_with_root_destroy(nhosts_wr, ips_wr);
    pthread_attr_destroy(&attr);
    pcp_state_destroy(state);
    return rval;
}

int pcp_write(int nhosts, char **ips, char *localfile, char *remotefile, 
              pcp_cmd_options *opts)
{
    int             rval;  
    pcp_write_cmd   *cmd = NULL;

    pcp_write_cmd_create(&cmd, localfile, remotefile);
    if ((rval = pcp_cmd(PCP_WRITE_CMD, cmd, opts, nhosts, ips)) != PCP_OK) {
        pcp_write_cmd_destroy(cmd);
        return rval;
    }
    pcp_write_cmd_destroy(cmd);
    return PCP_OK;
}

int pcp_delete(int nhosts, char **ips, char *remotefile, pcp_cmd_options *opts)
{
    int             rval;  
    pcp_delete_cmd  *cmd = NULL;

    pcp_delete_cmd_create(&cmd, remotefile);
    if ((rval = pcp_cmd(PCP_DELETE_CMD, cmd, opts, nhosts, ips)) != PCP_OK) {
        pcp_delete_cmd_destroy(cmd);
        return rval;
    }
    pcp_delete_cmd_destroy(cmd);
    return PCP_OK;
}

int pcp_checksum(int nhosts, char **ips, char *remotefile, pcp_cmd_options *opts)
{
    int               rval;  
    pcp_checksum_cmd  *cmd = NULL;

    pcp_checksum_cmd_create(&cmd, remotefile);
    if ((rval = pcp_cmd(PCP_CHECKSUM_CMD, cmd, opts, nhosts, ips)) != PCP_OK) {
        pcp_checksum_cmd_destroy(cmd);
        return rval;
    }
    pcp_checksum_cmd_destroy(cmd);
    return PCP_OK;
}
