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
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "e_error.h"
#include "net.h"

int net_cli_sock_create(int *sock, char *svr_host, int svr_port)
{   
    int                 opt, error;
    struct hostent      *h;
    struct sockaddr_in  svr_sockaddr;

    if (((*sock) = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return E_SOCKET_ERROR;
    opt = 1;
    setsockopt((*sock), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
    opt = 1;
    setsockopt((*sock), SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(int));
    opt = 0; /* Explicitly turn no delay off (i.e., Nagle algorithm on) */
    setsockopt((*sock), IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(int));

    if ((h = gethostbyname(svr_host)) == NULL) {
        error = E_GETHOSTBYNAME_ERROR;
        goto cleanup;
    }
    e_assert(h->h_addr_list[0] != NULL);

    bzero((char *)&svr_sockaddr, sizeof(svr_sockaddr));
    svr_sockaddr.sin_family = AF_INET;
    svr_sockaddr.sin_port   = htons(svr_port);
    memcpy(&svr_sockaddr.sin_addr.s_addr, h->h_addr_list[0], 
           sizeof(struct in_addr));

    for (;;) {
        if (connect((*sock), (struct sockaddr *)&svr_sockaddr,
                    sizeof(svr_sockaddr)) < 0) {
            if (errno == EINTR)
                continue;
            error = E_CONNECT_ERROR;
            goto cleanup;
        }
        break;
    }
    return E_OK;

 cleanup:
    close(*sock);
    (*sock) = -1;
    return error;
}

void net_cli_sock_destroy(int sock)
{
    close(sock);
}

int net_svr_sock_create(int *sock, int svr_port)
{
    int                 opt, error;
    struct sockaddr_in  svr_sockaddr;
    unsigned int        sockaddr_len = sizeof(struct sockaddr_in);

    if (((*sock) = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return E_SOCKET_ERROR;
    opt = 1;
    setsockopt((*sock), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
    opt = 1;
    setsockopt((*sock), SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(int));

    bzero((char *)&svr_sockaddr, sockaddr_len);
    svr_sockaddr.sin_family      = AF_INET;
    svr_sockaddr.sin_port        = htons(svr_port);
    svr_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind((*sock), (struct sockaddr *)&svr_sockaddr, sockaddr_len) < 0) {
        error = E_BIND_ERROR;
        goto cleanup;
    }
    if (listen((*sock), NET_DEFAULT_BACKLOG) < 0) {
        error = E_LISTEN_ERROR;
        goto cleanup;
    }
    return E_OK;

 cleanup:
    close(*sock);
    (*sock) = -1;
    return error;
}

void net_svr_sock_destroy(int sock)
{
    close(sock);
}

int net_cli_unixsock_create(int *sock, char *cli_path, char *svr_path)
{
    int                 error;
    struct sockaddr_un  cli_sockaddr;
    struct sockaddr_un  svr_sockaddr;
    socklen_t           sockaddr_len = sizeof(struct sockaddr_un);

    if (((*sock) = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        return E_SOCKET_ERROR;

    cli_sockaddr.sun_family  = AF_UNIX;
    bzero(cli_sockaddr.sun_path, UNIX_PATH_MAX);
    cli_sockaddr.sun_path[0] = '\0';
    sprintf(&cli_sockaddr.sun_path[1], "%s", cli_path);
    if (bind((*sock), (struct sockaddr *)&cli_sockaddr, 
             sockaddr_len) < 0) {
        error = E_BIND_ERROR;
        goto cleanup;
    }
    svr_sockaddr.sun_family  = AF_UNIX;
    bzero(svr_sockaddr.sun_path, UNIX_PATH_MAX);
    svr_sockaddr.sun_path[0] = '\0';
    sprintf(&svr_sockaddr.sun_path[1], "%s", svr_path);
    if (connect((*sock), (struct sockaddr *)&svr_sockaddr, 
                sockaddr_len) < 0) {
        error = E_CONNECT_ERROR;
        goto cleanup;
    }
    return E_OK;

 cleanup:
    close(*sock);
    (*sock) = -1;
    return error;
}

void net_cli_unixsock_destroy(int sock)
{
    close(sock);
}

int net_svr_unixsock_create(int *sock, char *svr_path)
{
    int                 opt, error;
    struct sockaddr_un  sockaddr;
    socklen_t           sockaddr_len = sizeof(struct sockaddr_un);

    if (((*sock) = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        return E_SOCKET_ERROR;
    opt = 1;
    setsockopt((*sock), SOL_SOCKET, SO_PASSCRED, &opt, sizeof(opt));

    sockaddr.sun_family  = AF_UNIX;
    bzero(sockaddr.sun_path, UNIX_PATH_MAX);
    sockaddr.sun_path[0] = '\0';
    sprintf(&sockaddr.sun_path[1], "%s", svr_path);
    if (bind((*sock), (struct sockaddr *)&sockaddr, 
             sockaddr_len) < 0) {
        error = E_BIND_ERROR;
        goto cleanup;
    }
    if (listen((*sock), NET_DEFAULT_BACKLOG) < 0) {
        error = E_LISTEN_ERROR;
        goto cleanup;
    }
    return E_OK;

 cleanup:
    close(*sock);
    (*sock) = -1;
    return error;
}

void net_svr_unixsock_destroy(int sock)
{
    close(sock);
}

int net_send_bytes(int sock, void *send_buf, int nbytes)
{
    int n, bytes_sent;

    bytes_sent = 0;
    while (bytes_sent != nbytes) {
        if ((n = write(sock, (void *)((long int)send_buf + (long int)bytes_sent), 
                       nbytes - bytes_sent)) <= 0) {
            /* write is a "slow" syscall, must be manually restarted */
            if (errno == EINTR) 
                continue;
            return E_WRITE_ERROR;
        }
        bytes_sent += n;
    }
    return E_OK;
}

int net_recv_bytes(int sock, void *recv_buf, int nbytes)
{   
    int n, bytes_recv;

    bytes_recv = 0;
    while (bytes_recv != nbytes) {
        if ((n = read(sock, (void *)((long int)recv_buf + (long int)bytes_recv), 
                      nbytes - bytes_recv)) <= 0) {
            /* If n = 0 (errno=EITNR), means EOF, unless TCP_NODELAY set */
            return E_READ_ERROR;
        }
        bytes_recv += n;
    }
    return E_OK;
}

int net_hosttoip(char *host, char *ip)
{
    struct hostent  *h;
    struct in_addr  addr;

    if ((h = gethostbyname(host)) == NULL)
        return E_GETHOSTNAME_ERROR;
    memcpy(&addr.s_addr, h->h_addr_list[0], sizeof(struct in_addr));
    sprintf(ip, "%d.%d.%d.%d",
            (addr.s_addr & 0x000000FF) >> 0,
            (addr.s_addr & 0x0000FF00) >> 8,
            (addr.s_addr & 0x00FF0000) >> 16,
            (addr.s_addr & 0xFF000000) >> 24);
    return E_OK;
}

int net_iptohost(char *ip, char *host, int len)
{
    struct hostent  *h1, *h2;
    struct in_addr  addr;
    
    if ((h1 = gethostbyname(ip)) == NULL)
        return E_GETHOSTBYNAME_ERROR;
    memcpy(&addr.s_addr, h1->h_addr_list[0], sizeof(struct in_addr));
    if ((h2 = gethostbyaddr(&addr, sizeof(addr), AF_INET)) == NULL)
        return E_GETHOSTBYADDR_ERROR;
    sprintf(host, "%.*s", len - 1, h2->h_name);
    return E_OK;
}

int net_gethostip(char *ip)
{
    char hostname[4096];

    /* To avoid DNS lookup, make sure hostname in /etc/hosts */
    if (gethostname(hostname, 4096) < 0)
        return E_GETHOSTNAME_ERROR;
    return net_hosttoip(hostname, ip);
}
