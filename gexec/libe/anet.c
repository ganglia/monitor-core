#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "e_error.h"
#include "anet.h"
#include "net.h"

int anet_cli_tcpsock_create(int *sock)
{
    int opt;

    if (((*sock) = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return E_SOCKET_ERROR;
    opt = 1;
    setsockopt((*sock), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
    opt = 1;
    setsockopt((*sock), SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(int));
    opt = 0; /* Explicitly turn no delay off (i.e., Nagle algorithm on) */
    setsockopt((*sock), IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(int));
    fcntl((*sock), F_SETFL, O_NONBLOCK);
    return E_OK;
}

void anet_cli_tcpsock_destroy(int sock)
{
    close(sock);
}

int anet_svr_tcpsock_create(int *sock, int svr_port)
{
    int rval;

    if ((rval = net_svr_sock_create(sock, svr_port)) != E_OK)
        return rval;
    fcntl((*sock), F_SETFL, O_NONBLOCK);
    return E_OK;
}

void anet_svr_tcpsock_destroy(int sock)
{
    close(sock);
}

