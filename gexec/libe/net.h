/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#ifndef __NET_H
#define __NET_H

/* Max backlog length for incoming connections */
#define NET_DEFAULT_BACKLOG   16

/* Taken from /usr/include/sys/un.h, where it is hardcoded */
#define UNIX_PATH_MAX        108

/* Max length of www.xxx.yyy.zzz */
#define IP_STRLEN             16  

int  net_cli_sock_create(int *sock, char *svr_host, int svr_port);
void net_cli_sock_destroy(int sock);
int  net_svr_sock_create(int *sock, int svr_port);
void net_svr_sock_destroy(int sock);
int  net_cli_unixsock_create(int *sock, char *cli_path, char *svr_path);
void net_cli_unixsock_destroy(int sock);
int  net_svr_unixsock_create(int *sock, char *svr_path);
void net_svr_unixsock_destroy(int sock);
int  net_send_bytes(int sock, void *send_buf, int nbytes);
int  net_recv_bytes(int sock, void *recv_buf, int nbytes);
int  net_hosttoip(char *host, char *ip);
int  net_iptohost(char *ip, char *host, int len);
int  net_gethostip(char *ip); 

#endif /* __NET_H */
